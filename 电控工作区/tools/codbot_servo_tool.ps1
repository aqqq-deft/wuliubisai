param(
    [switch]$SelfTest,
    [ValidateSet('Both','C6','C7')]
    [string]$Mode = 'Both'
)

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[Windows.Forms.Application]::EnableVisualStyles()

$script:Port = $null
$script:HeartbeatEnabled = $false
$script:FeedbackSeen = $false
$script:FeedbackTimeoutLatched = $false
$script:LastFeedbackTime = [DateTime]::MinValue
$script:RxBuffer = New-Object 'System.Collections.Generic.List[byte]'
$script:RxByteCount = 0L
$script:RxValidFrameCount = 0L
$script:RxChecksumErrorCount = 0L
$script:RxLastCode = $null
$script:RxLastSample = '--'
$script:RxActivityLogged = $false
$script:CalibrationDirection = 0
$script:CalibrationHoldTicks = 0
$script:CalibrationSendTicks = 0
$script:CalibrationRepeated = $false
$script:CurrentPulse = $null
$script:CommandedPulse = $null
$script:AutoTargetPulse = $null
$script:AutoTargetName = ''
$script:C7FeedbackSeen = $false
$script:C7FeedbackTimeoutLatched = $false
$script:C7LastFeedbackTime = [DateTime]::MinValue
$script:C7CurrentPulse = $null
$script:C7TargetPulse = $null
$script:C7State = 0
$script:C7SessionEnabled = $false
$script:C7ContinuousDirection = 0
$script:C7ContinuousCommandPulse = $null
$script:C7ContinuousTickCount = 0
$script:C7NextQueryTime = [DateTime]::MinValue
$script:C7ConfigPath = Join-Path $PSScriptRoot 'c7_servo_positions.json'
$script:C7ConfigLoadMessage = ''
$script:C7StateNames = @{
    0 = '未使能（PC7无有效脉宽）'
    1 = '保持'
    2 = '移动中'
    255 = '急停锁定'
}
$script:StateNames = @{
    0 = '停止/保持'
    1 = '正在打开'
    2 = '已打开'
    3 = '正在合拢'
    4 = '已合拢'
    255 = '急停锁定'
}

function Format-Frame([byte[]]$Frame) {
    return (($Frame | ForEach-Object { $_.ToString('X2') }) -join ' ')
}

function New-ProtocolFrame([int]$Code, [byte[]]$Payload) {
    [int]$length = 1 + $Payload.Length
    [byte[]]$frame = New-Object byte[] ($length + 3)
    $frame[0] = 0xCD
    $frame[1] = [byte]$length
    $frame[2] = [byte]$Code
    [byte]$xorValue = [byte]$Code
    for ($i = 0; $i -lt $Payload.Length; $i++) {
        $frame[3 + $i] = $Payload[$i]
        $xorValue = $xorValue -bxor $Payload[$i]
    }
    $frame[$frame.Length - 1] = $xorValue
    return ,$frame
}

function New-BucketFrame([int]$Command) {
    return ,(New-ProtocolFrame 0x07 ([byte[]]@($Command)))
}

function New-CalibrationFrame([int]$Action) {
    return ,(New-ProtocolFrame 0x09 ([byte[]]@($Action)))
}

function Limit-C7Pulse([int]$Pulse) {
    return [Math]::Max(550, [Math]::Min(2450, $Pulse))
}

function Convert-C7PulseToAngle([int]$Pulse) {
    return [Math]::Round((($Pulse - 500) * 270.0 / 2000.0), 3)
}

function New-C7Frame([int]$Action, [int]$Pulse) {
    if ($Pulse -lt 0 -or $Pulse -gt 65535) {
        throw "C7脉宽字段超出16位范围：$Pulse"
    }
    [byte[]]$payload = @(
        [byte]$Action,
        [byte](($Pulse -shr 8) -band 0xFF),
        [byte]($Pulse -band 0xFF)
    )
    return ,(New-ProtocolFrame 0x0A $payload)
}

function New-C7DefaultConfig {
    return [ordered]@{
        last_pulse_us = 1500
        position_0_us = $null
        position_1_us = $null
        position_2_us = $null
        position_3_us = $null
    }
}

function Get-C7ConfigPulse($Object, [string]$Name, [bool]$AllowNull) {
    if (-not ($Object.PSObject.Properties.Name -contains $Name)) {
        throw "配置缺少字段：$Name"
    }
    $value = $Object.$Name
    if ($null -eq $value) {
        if ($AllowNull) { return $null }
        throw "配置字段不能为空：$Name"
    }
    [int]$pulse = $value
    if ($pulse -lt 550 -or $pulse -gt 2450) {
        throw "配置字段 $Name 超出550..2450us：$pulse"
    }
    return $pulse
}

function ConvertTo-C7Config($Object) {
    $config = New-C7DefaultConfig
    $config.last_pulse_us = Get-C7ConfigPulse $Object 'last_pulse_us' $false
    foreach ($index in 0..3) {
        $name = "position_${index}_us"
        $config[$name] = Get-C7ConfigPulse $Object $name $true
    }
    return $config
}

function Save-C7ConfigToPath([System.Collections.IDictionary]$Config, [string]$Path) {
    $Config | ConvertTo-Json | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Load-C7ConfigFromPath([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) {
        return (New-C7DefaultConfig)
    }
    $raw = Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
    return (ConvertTo-C7Config $raw)
}

function Save-C7Config {
    Save-C7ConfigToPath $script:C7Config $script:C7ConfigPath
}

function Test-C7JsonRoundTrip {
    $tempPath = [IO.Path]::GetTempFileName()
    try {
        $config = New-C7DefaultConfig
        $config.position_0_us = 1450
        $config.position_3_us = 1775
        Save-C7ConfigToPath $config $tempPath
        $loaded = Load-C7ConfigFromPath $tempPath
        if ($loaded.last_pulse_us -ne 1500 -or
            $loaded.position_0_us -ne 1450 -or
            $loaded.position_3_us -ne 1775 -or
            $null -ne $loaded.position_1_us) {
            throw 'C7 JSON读写自检失败'
        }
    } finally {
        Remove-Item -LiteralPath $tempPath -Force -ErrorAction SilentlyContinue
    }
}

function New-ZeroMotionFrame {
    [byte[]]$frame = @(0xCD,0x0A,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00)
    return ,$frame
}

function Test-ProtocolFrames {
    $tests = @(
        @('开爪', (New-BucketFrame 1), 'CD 02 07 01 06'),
        @('合爪', (New-BucketFrame 2), 'CD 02 07 02 05'),
        @('保持', (New-BucketFrame 0), 'CD 02 07 00 07'),
        @('复位', (New-BucketFrame 5), 'CD 02 07 05 02'),
        @('急停', (New-BucketFrame 255), 'CD 02 07 FF F8'),
        @('减小10us', (New-CalibrationFrame 1), 'CD 02 09 01 08'),
        @('增加10us', (New-CalibrationFrame 2), 'CD 02 09 02 0B')
    )
    $tests += ,@('C7预置1500us', (New-C7Frame 1 1500), 'CD 04 0A 01 05 DC D2')
    $tests += ,@('C7移动1505us', (New-C7Frame 2 1505), 'CD 04 0A 02 05 E1 EC')
    $tests += ,@('C7保持', (New-C7Frame 0 0), 'CD 04 0A 00 00 00 0A')
    $tests += ,@('C7查询', (New-C7Frame 7 0), 'CD 04 0A 07 00 00 0D')
    $tests += ,@('C7急停', (New-C7Frame 255 0), 'CD 04 0A FF 00 00 F5')
    foreach ($test in $tests) {
        $actual = Format-Frame ([byte[]]$test[1])
        if ($actual -ne $test[2]) {
            throw "协议自检失败：$($test[0])，实际=$actual，预期=$($test[2])"
        }
    }
    [byte[]]$feedback = @(0xFD,0x06,0x0B,0x01,0x05,0xDC,0x05,0xDC,0x0A)
    [byte]$feedbackXor = 0
    for ($i = 2; $i -lt 8; $i++) {
        $feedbackXor = $feedbackXor -bxor $feedback[$i]
    }
    if ($feedbackXor -ne $feedback[8]) {
        throw 'C7 0x0B feedback XOR self-test failed'
    }
    if ((Limit-C7Pulse 0) -ne 550 -or
        (Limit-C7Pulse 3000) -ne 2450 -or
        (Convert-C7PulseToAngle 1500) -ne 135 -or
        [Math]::Abs((Convert-C7PulseToAngle 1505) - (Convert-C7PulseToAngle 1500) - 0.675) -gt 0.0001) {
        throw 'C7 limit or angle conversion self-test failed'
    }
    Test-C7JsonRoundTrip
}

Test-ProtocolFrames
if ($SelfTest) {
    Write-Output 'SERVO_TOOL_SELF_TEST_OK'
    exit 0
}
try {
    $script:C7Config = Load-C7ConfigFromPath $script:C7ConfigPath
} catch {
    $script:C7Config = New-C7DefaultConfig
    $script:C7ConfigLoadMessage = "C7配置读取失败，已使用安全默认值：$($_.Exception.Message)"
}


$form = New-Object Windows.Forms.Form
$form.Text = switch ($Mode) {
    'C6' { 'CODBOT-RCU C6舵机调试工具' }
    'C7' { 'CODBOT-RCU C7 PD-20S调试工具' }
    default { 'CODBOT-RCU C6/C7双舵机调试工具' }
}
$form.Size = '1460,700'
$form.StartPosition = 'CenterScreen'
$form.FormBorderStyle = 'FixedDialog'
$form.MaximizeBox = $false

$wiring = New-Object Windows.Forms.Label
$wiring.Text = '接线：C6最左列；从“C6字样/板内侧”到板边依次为 橙色信号 - 红色+5V - 棕色GND。首次必须脱开爪子连杆。'
$wiring.Location = '20,15'
$wiring.Size = '810,42'
$wiring.ForeColor = 'DarkRed'
$wiring.Font = New-Object Drawing.Font('Microsoft YaHei UI',10,[Drawing.FontStyle]::Bold)
$form.Controls.Add($wiring)

$portBox = New-Object Windows.Forms.ComboBox
$portBox.Location = '20,70'
$portBox.Size = '275,28'
$portBox.DropDownStyle = 'DropDownList'
$form.Controls.Add($portBox)

$refreshButton = New-Object Windows.Forms.Button
$refreshButton.Text = '刷新串口'
$refreshButton.Location = '305,68'
$refreshButton.Size = '95,30'
$form.Controls.Add($refreshButton)

$connectButton = New-Object Windows.Forms.Button
$connectButton.Text = '连接'
$connectButton.Location = '410,68'
$connectButton.Size = '95,30'
$form.Controls.Add($connectButton)

$connectionLabel = New-Object Windows.Forms.Label
$connectionLabel.Text = '未连接（请选择CH340）'
$connectionLabel.Location = '520,74'
$connectionLabel.Size = '305,24'
$connectionLabel.ForeColor = 'DarkRed'
$form.Controls.Add($connectionLabel)

$feedbackGroup = New-Object Windows.Forms.GroupBox
$feedbackGroup.Text = 'STM32反馈'
$feedbackGroup.Location = '20,115'
$feedbackGroup.Size = '810,85'
$form.Controls.Add($feedbackGroup)

$stateTitle = New-Object Windows.Forms.Label
$stateTitle.Text = '状态：'
$stateTitle.Location = '20,35'
$stateTitle.AutoSize = $true
$feedbackGroup.Controls.Add($stateTitle)

$stateValue = New-Object Windows.Forms.Label
$stateValue.Text = '尚未收到0x08反馈'
$stateValue.Location = '75,33'
$stateValue.Size = '180,25'
$stateValue.ForeColor = 'DarkOrange'
$stateValue.Font = New-Object Drawing.Font('Microsoft YaHei UI',10,[Drawing.FontStyle]::Bold)
$feedbackGroup.Controls.Add($stateValue)

$pulseTitle = New-Object Windows.Forms.Label
$pulseTitle.Text = '当前脉宽：'
$pulseTitle.Location = '285,35'
$pulseTitle.AutoSize = $true
$feedbackGroup.Controls.Add($pulseTitle)

$pulseValue = New-Object Windows.Forms.Label
$pulseValue.Text = '---- us'
$pulseValue.Location = '370,33'
$pulseValue.Size = '110,25'
$pulseValue.ForeColor = 'Navy'
$pulseValue.Font = New-Object Drawing.Font('Consolas',12,[Drawing.FontStyle]::Bold)
$feedbackGroup.Controls.Add($pulseValue)

$feedbackAge = New-Object Windows.Forms.Label
$feedbackAge.Text = '等待反馈'
$feedbackAge.Location = '515,35'
$feedbackAge.Size = '260,25'
$feedbackAge.ForeColor = 'DarkOrange'
$feedbackGroup.Controls.Add($feedbackAge)

$rxDiagnostic = New-Object Windows.Forms.Label
$rxDiagnostic.Text = '原始串口：RX 0 B / 有效帧 0 / 校验错误 0 / 最后帧 --'
$rxDiagnostic.Location = '20,60'
$rxDiagnostic.Size = '760,20'
$rxDiagnostic.ForeColor = 'DarkRed'
$feedbackGroup.Controls.Add($rxDiagnostic)

$startButton = New-Object Windows.Forms.Button
$startButton.Text = '开始安全测试'
$startButton.Location = '20,215'
$startButton.Size = '180,42'
$startButton.BackColor = 'LightGreen'
$startButton.Enabled = $false
$form.Controls.Add($startButton)

$startHint = New-Object Windows.Forms.Label
$startHint.Text = '连接后先等待有效反馈；点击后才发送零速度心跳和复位命令。'
$startHint.Location = '220,228'
$startHint.Size = '600,25'
$startHint.ForeColor = 'DarkOrange'
$form.Controls.Add($startHint)

$actionGroup = New-Object Windows.Forms.GroupBox
$actionGroup.Text = '舵机命令'
$actionGroup.Location = '20,270'
$actionGroup.Size = '810,180'
$actionGroup.Enabled = $false
$form.Controls.Add($actionGroup)

$decreaseButton = New-Object Windows.Forms.Button
$decreaseButton.Text = '减小 -10 us'
$decreaseButton.Location = '20,35'
$decreaseButton.Size = '145,42'
$actionGroup.Controls.Add($decreaseButton)

$increaseButton = New-Object Windows.Forms.Button
$increaseButton.Text = '增加 +10 us'
$increaseButton.Location = '180,35'
$increaseButton.Size = '145,42'
$actionGroup.Controls.Add($increaseButton)

$holdButton = New-Object Windows.Forms.Button
$holdButton.Text = '保持当前位置'
$holdButton.Location = '340,35'
$holdButton.Size = '145,42'
$actionGroup.Controls.Add($holdButton)

$resetButton = New-Object Windows.Forms.Button
$resetButton.Text = '复位/回1500us'
$resetButton.Location = '500,35'
$resetButton.Size = '145,42'
$actionGroup.Controls.Add($resetButton)

$emergencyButton = New-Object Windows.Forms.Button
$emergencyButton.Text = '舵机急停'
$emergencyButton.Location = '660,35'
$emergencyButton.Size = '125,42'
$emergencyButton.BackColor = 'IndianRed'
$emergencyButton.ForeColor = 'White'
$actionGroup.Controls.Add($emergencyButton)

$openButton = New-Object Windows.Forms.Button
$openButton.Text = '一键开爪（550us）'
$openButton.Location = '180,100'
$openButton.Size = '180,45'
$actionGroup.Controls.Add($openButton)

$closeButton = New-Object Windows.Forms.Button
$closeButton.Text = '一键合爪（2450us）'
$closeButton.Location = '450,100'
$closeButton.Size = '180,45'
$actionGroup.Controls.Add($closeButton)

$speedLabel = New-Object Windows.Forms.Label
$speedLabel.Text = '连续速度'
$speedLabel.Location = '660,96'
$speedLabel.Size = '125,20'
$actionGroup.Controls.Add($speedLabel)
$speedBox = New-Object Windows.Forms.ComboBox
$speedBox.Location = '660,118'
$speedBox.Size = '125,28'
$speedBox.DropDownStyle = 'DropDownList'
[void]$speedBox.Items.Add('慢速')
[void]$speedBox.Items.Add('中速（推荐）')
[void]$speedBox.Items.Add('快速')
$speedBox.SelectedIndex = 1
$actionGroup.Controls.Add($speedBox)

$placeholderHint = New-Object Windows.Forms.Label
$placeholderHint.Text = '开爪=550us，闭爪=2450us；中速全程约7.6秒，快速约3.8秒。安装爪子后须重定安全位置。'
$placeholderHint.Location = '20,465'
$placeholderHint.Size = '810,28'
$placeholderHint.ForeColor = 'DarkOrange'
$form.Controls.Add($placeholderHint)

$logBox = New-Object Windows.Forms.TextBox
$logBox.Location = '20,500'
$logBox.Size = '810,145'
$logBox.Multiline = $true
$logBox.ScrollBars = 'Vertical'
$logBox.ReadOnly = $true
$logBox.Font = New-Object Drawing.Font('Consolas',9)
$form.Controls.Add($logBox)

$c7Group = New-Object Windows.Forms.GroupBox
$c7Group.Text = 'C7 PD-20S（显示值均为指令脉宽，不是真实位置反馈）'
$c7Group.Location = '850,15'
$c7Group.Size = '575,630'
$form.Controls.Add($c7Group)

$c7Warning = New-Object Windows.Forms.Label
$c7Warning.Text = 'C7接线：白线=板内侧PC7信号，红线=中间+5V，黑线=板边GND，三根线均接C7。' +
    [Environment]::NewLine +
    'Type-C不能单独带舵机；先接好主板12V电源，或在断开12V时从绿色+5V端子输入限流5.0V。异常立即切电。'
$c7Warning.Location = '15,25'
$c7Warning.Size = '545,70'
$c7Warning.ForeColor = 'DarkRed'
$c7Warning.Font = New-Object Drawing.Font('Microsoft YaHei UI',9,[Drawing.FontStyle]::Bold)
$c7Group.Controls.Add($c7Warning)

$c7StateTitle = New-Object Windows.Forms.Label
$c7StateTitle.Text = '状态：'
$c7StateTitle.Location = '15,105'
$c7StateTitle.AutoSize = $true
$c7Group.Controls.Add($c7StateTitle)

$c7StateValue = New-Object Windows.Forms.Label
$c7StateValue.Text = '尚未收到0x0B反馈'
$c7StateValue.Location = '70,103'
$c7StateValue.Size = '475,24'
$c7StateValue.ForeColor = 'DarkOrange'
$c7StateValue.Font = New-Object Drawing.Font('Microsoft YaHei UI',10,[Drawing.FontStyle]::Bold)
$c7Group.Controls.Add($c7StateValue)

$c7CurrentTitle = New-Object Windows.Forms.Label
$c7CurrentTitle.Text = '当前指令：'
$c7CurrentTitle.Location = '15,140'
$c7CurrentTitle.AutoSize = $true
$c7Group.Controls.Add($c7CurrentTitle)

$c7CurrentValue = New-Object Windows.Forms.Label
$c7CurrentValue.Text = '---- us'
$c7CurrentValue.Location = '95,137'
$c7CurrentValue.Size = '100,25'
$c7CurrentValue.ForeColor = 'Navy'
$c7CurrentValue.Font = New-Object Drawing.Font('Consolas',11,[Drawing.FontStyle]::Bold)
$c7Group.Controls.Add($c7CurrentValue)

$c7TargetTitle = New-Object Windows.Forms.Label
$c7TargetTitle.Text = '目标指令：'
$c7TargetTitle.Location = '220,140'
$c7TargetTitle.AutoSize = $true
$c7Group.Controls.Add($c7TargetTitle)

$c7TargetValue = New-Object Windows.Forms.Label
$c7TargetValue.Text = '---- us'
$c7TargetValue.Location = '300,137'
$c7TargetValue.Size = '100,25'
$c7TargetValue.ForeColor = 'Navy'
$c7TargetValue.Font = New-Object Drawing.Font('Consolas',11,[Drawing.FontStyle]::Bold)
$c7Group.Controls.Add($c7TargetValue)

$c7AngleValue = New-Object Windows.Forms.Label
$c7AngleValue.Text = '参考角度：----°'
$c7AngleValue.Location = '15,175'
$c7AngleValue.Size = '250,25'
$c7Group.Controls.Add($c7AngleValue)

$c7FeedbackAge = New-Object Windows.Forms.Label
$c7FeedbackAge.Text = '等待0x0B反馈'
$c7FeedbackAge.Location = '280,175'
$c7FeedbackAge.Size = '265,25'
$c7FeedbackAge.ForeColor = 'DarkOrange'
$c7Group.Controls.Add($c7FeedbackAge)

$c7PresetLabel = New-Object Windows.Forms.Label
$c7PresetLabel.Text = '预置脉宽'
$c7PresetLabel.Location = '15,215'
$c7PresetLabel.AutoSize = $true
$c7Group.Controls.Add($c7PresetLabel)

$c7PresetBox = New-Object Windows.Forms.NumericUpDown
$c7PresetBox.Location = '85,211'
$c7PresetBox.Size = '85,28'
$c7PresetBox.Minimum = 550
$c7PresetBox.Maximum = 2450
$c7PresetBox.Increment = 5
$c7PresetBox.Value = [decimal]$script:C7Config['last_pulse_us']
$c7Group.Controls.Add($c7PresetBox)

$c7PresetButton = New-Object Windows.Forms.Button
$c7PresetButton.Text = '预置并使能（先确认主板有足够5V供电）'
$c7PresetButton.Location = '185,205'
$c7PresetButton.Size = '360,40'
$c7PresetButton.BackColor = 'LightGreen'
$c7PresetButton.Enabled = $false
$c7Group.Controls.Add($c7PresetButton)

$c7DecreaseButton = New-Object Windows.Forms.Button
$c7DecreaseButton.Text = '连续减小'
$c7DecreaseButton.Location = '15,260'
$c7DecreaseButton.Size = '105,40'
$c7DecreaseButton.Enabled = $false
$c7Group.Controls.Add($c7DecreaseButton)

$c7IncreaseButton = New-Object Windows.Forms.Button
$c7IncreaseButton.Text = '连续增加'
$c7IncreaseButton.Location = '130,260'
$c7IncreaseButton.Size = '105,40'
$c7IncreaseButton.Enabled = $false
$c7Group.Controls.Add($c7IncreaseButton)

$c7HoldButton = New-Object Windows.Forms.Button
$c7HoldButton.Text = '保持'
$c7HoldButton.Location = '245,260'
$c7HoldButton.Size = '105,40'
$c7HoldButton.Enabled = $false
$c7Group.Controls.Add($c7HoldButton)

$c7SpeedLabel = New-Object Windows.Forms.Label
$c7SpeedLabel.Text = '手动速度'
$c7SpeedLabel.Location = '365,243'
$c7SpeedLabel.AutoSize = $true
$c7Group.Controls.Add($c7SpeedLabel)

$c7SpeedBox = New-Object Windows.Forms.ComboBox
$c7SpeedBox.Location = '365,270'
$c7SpeedBox.Size = '90,28'
$c7SpeedBox.DropDownStyle = 'DropDownList'
[void]$c7SpeedBox.Items.Add('中速')
[void]$c7SpeedBox.Items.Add('高速')
$c7SpeedBox.SelectedIndex = 0
$c7Group.Controls.Add($c7SpeedBox)

$c7EmergencyButton = New-Object Windows.Forms.Button
$c7EmergencyButton.Text = 'C7急停'
$c7EmergencyButton.Location = '465,260'
$c7EmergencyButton.Size = '80,40'
$c7EmergencyButton.BackColor = 'IndianRed'
$c7EmergencyButton.ForeColor = 'White'
$c7EmergencyButton.Enabled = $false
$c7Group.Controls.Add($c7EmergencyButton)

$c7PositionGroup = New-Object Windows.Forms.GroupBox
$c7PositionGroup.Text = '整机记录：0位 + 三个固定角度'
$c7PositionGroup.Location = '15,315'
$c7PositionGroup.Size = '530,230'
$c7Group.Controls.Add($c7PositionGroup)

$script:C7PositionLabels = @()
$script:C7RecordButtons = @()
$script:C7MoveButtons = @()
foreach ($index in 0..3) {
    $y = 30 + ($index * 47)
    $name = if ($index -eq 0) { '0位' } else { "位置$index" }
    $valueLabel = New-Object Windows.Forms.Label
    $valueLabel.Text = "$name：未记录"
    $valueLabel.Location = "15,$($y + 8)"
    $valueLabel.Size = '160,25'
    $c7PositionGroup.Controls.Add($valueLabel)
    $script:C7PositionLabels += $valueLabel

    $recordButton = New-Object Windows.Forms.Button
    $recordButton.Text = "记录$name"
    $recordButton.Location = "180,$y"
    $recordButton.Size = '145,36'
    $recordButton.Tag = $index
    $recordButton.Enabled = $false
    $c7PositionGroup.Controls.Add($recordButton)
    $script:C7RecordButtons += $recordButton

    $moveButton = New-Object Windows.Forms.Button
    $moveButton.Text = if ($index -eq 0) { '回0位' } else { "去位置$index" }
    $moveButton.Location = "340,$y"
    $moveButton.Size = '165,36'
    $moveButton.Tag = $index
    $moveButton.Enabled = $false
    $c7PositionGroup.Controls.Add($moveButton)
    $script:C7MoveButtons += $moveButton
}

$c7BottomHint = New-Object Windows.Forms.Label
$c7BottomHint.Text = '中/高速只控制手动连续移动；回0位和去位置1/2/3始终使用固件最快速度。四个位置可重新记录覆盖。'
$c7BottomHint.Location = '15,565'
$c7BottomHint.Size = '540,45'
$c7BottomHint.ForeColor = 'DarkOrange'
$c7Group.Controls.Add($c7BottomHint)

if ($Mode -eq 'C6') {
    $form.Size = '870,700'
    $c7Group.Visible = $false
} elseif ($Mode -eq 'C7') {
    $form.Size = '1185,800'
    $wiring.Text = 'C7接线：白线=板内侧PC7信号，红线=中间+5V，黑线=板边GND。该程序只发送C7命令，不改变C6。'
    $wiring.Size = '1120,42'

    $startButton.Visible = $false
    $startHint.Visible = $false
    $actionGroup.Visible = $false
    $placeholderHint.Visible = $false

    $stateTitle.Visible = $false
    $stateValue.Visible = $false
    $pulseTitle.Visible = $false
    $pulseValue.Visible = $false
    $feedbackAge.Visible = $false
    $feedbackGroup.Text = '串口诊断'
    $feedbackGroup.Location = '610,115'
    $feedbackGroup.Size = '540,65'
    $rxDiagnostic.Location = '15,30'
    $rxDiagnostic.Size = '510,22'

    $c7Group.Location = '20,115'
    $logBox.Location = '610,195'
    $logBox.Size = '540,550'
}

function Add-Log([string]$Text) {
    $logBox.AppendText("$(Get-Date -Format 'HH:mm:ss.fff')  $Text`r`n")
}

function Test-PortOpen {
    return ($null -ne $script:Port -and $script:Port.IsOpen)
}

function Send-Frame([byte[]]$Frame, [bool]$Quiet = $false) {
    if (-not (Test-PortOpen)) {
        if (-not $Quiet) { Add-Log '发送失败：请先连接CH340串口。' }
        return $false
    }
    try {
        $script:Port.Write($Frame, 0, $Frame.Length)
        if (-not $Quiet) { Add-Log ('TX ' + (Format-Frame $Frame)) }
        return $true
    } catch {
        Add-Log "发送失败：$($_.Exception.Message)"
        return $false
    }
}

function Send-BucketCommand([int]$Command, [string]$Name) {
    if (Send-Frame (New-BucketFrame $Command)) {
        Add-Log "命令：$Name"
    }
}

function Send-CalibrationCommand([int]$Action, [string]$Name, [bool]$Quiet = $false) {
    if (Send-Frame (New-CalibrationFrame $Action) $Quiet) {
        if (-not $Quiet) { Add-Log "标定：$Name" }
    }
}


function Test-C7FeedbackFresh {
    return ($script:C7FeedbackSeen -and
        (([DateTime]::Now - $script:C7LastFeedbackTime).TotalMilliseconds -le 1000))
}

function Update-C7PositionControls {
    $fresh = Test-C7FeedbackFresh
    $canMove = ((Test-PortOpen) -and $fresh -and $script:C7SessionEnabled -and
        ($script:C7State -eq 1 -or $script:C7State -eq 2))
    $c7PresetButton.Enabled = ((Test-PortOpen) -and $fresh)
    $c7DecreaseButton.Enabled = $canMove
    $c7IncreaseButton.Enabled = $canMove
    $c7HoldButton.Enabled = $canMove
    $c7EmergencyButton.Enabled = ((Test-PortOpen) -and $script:C7SessionEnabled)

    foreach ($index in 0..3) {
        $key = "position_${index}_us"
        $pulse = $script:C7Config[$key]
        $name = if ($index -eq 0) { '0位' } else { "位置$index" }
        if ($null -eq $pulse) {
            $script:C7PositionLabels[$index].Text = "$name：未记录"
            $script:C7MoveButtons[$index].Enabled = $false
        } else {
            $angle = Convert-C7PulseToAngle ([int]$pulse)
            $script:C7PositionLabels[$index].Text = "$name：$pulse us / $angle°"
            $script:C7MoveButtons[$index].Enabled = $canMove
        }
        $script:C7RecordButtons[$index].Enabled = ($canMove -and $script:C7State -eq 1)
    }
}

function Send-C7Command([int]$Action, [int]$Pulse, [string]$Name, [bool]$Quiet = $false) {
    $limitedPulse = if ($Action -eq 1 -or $Action -eq 2) {
        Limit-C7Pulse $Pulse
    } else {
        0
    }
    if (Send-Frame (New-C7Frame $Action $limitedPulse) $Quiet) {
        $delayMs = if ($Action -eq 7) { 300 } else { 150 }
        $script:C7NextQueryTime = [DateTime]::Now.AddMilliseconds($delayMs)
        if (-not $Quiet) { Add-Log "C7命令：$Name" }
        return $true
    }
    return $false
}

function Save-C7LastPulse([int]$Pulse) {
    $script:C7Config['last_pulse_us'] = Limit-C7Pulse $Pulse
    try {
        Save-C7Config
    } catch {
        Add-Log "C7配置保存失败：$($_.Exception.Message)"
    }
}

function Start-C7Preset {
    Stop-C7Continuous $false
    if (-not (Test-PortOpen) -or -not (Test-C7FeedbackFresh)) {
        Add-Log 'C7尚无新鲜0x0B反馈，拒绝预置。'
        return
    }
    $pulse = Limit-C7Pulse ([int]$c7PresetBox.Value)
    [void](Send-Frame (New-ZeroMotionFrame) $true)
    if (Send-C7Command 1 $pulse "预置并使能 $pulse us") {
        $script:C7SessionEnabled = $true
        $script:C7FeedbackTimeoutLatched = $false
        $script:HeartbeatEnabled = $true
        $script:C7CurrentPulse = $pulse
        $script:C7TargetPulse = $pulse
        Save-C7LastPulse $pulse
        $connectionLabel.Text = '已连接 / C7状态查询心跳运行中'
        $connectionLabel.ForeColor = 'DarkGreen'
        Add-Log 'C7已输出预置脉宽；若机构异常，立即切断主板12V。'
        Update-C7PositionControls
    }
}

function Update-C7ContinuousButtons {
    $c7DecreaseButton.BackColor = if ($script:C7ContinuousDirection -lt 0) { 'Gold' } else { [Drawing.SystemColors]::Control }
    $c7IncreaseButton.BackColor = if ($script:C7ContinuousDirection -gt 0) { 'Gold' } else { [Drawing.SystemColors]::Control }
}

function Invoke-C7ContinuousTick {
    if ($script:C7ContinuousDirection -eq 0) { return }
    if (-not $script:C7SessionEnabled -or -not (Test-C7FeedbackFresh)) {
        Stop-C7Continuous $false
        return
    }
    $basePulse = if ($null -ne $script:C7ContinuousCommandPulse) {
        [int]$script:C7ContinuousCommandPulse
    } else {
        [int]$script:C7CurrentPulse
    }
    $pulse = Limit-C7Pulse ($basePulse + $script:C7ContinuousDirection)
    if ($pulse -eq $basePulse) {
        $script:C7ContinuousDirection = 0
        $script:C7ContinuousTickCount = 0
        $script:C7ContinuousCommandPulse = $null
        Update-C7ContinuousButtons
        Save-C7LastPulse $pulse
        Add-Log "C7已到安全限幅 $pulse us，连续移动自动停止。"
        return
    }
    if (Send-C7Command 2 $pulse "连续移动到 $pulse us" $true) {
        $script:C7ContinuousCommandPulse = $pulse
        $script:C7Config['last_pulse_us'] = $pulse
    }
}

function Start-C7Continuous([int]$Delta) {
    if ($Delta -ne -5 -and $Delta -ne 5) { return }
    if (-not $script:C7SessionEnabled -or -not (Test-C7FeedbackFresh)) {
        Add-Log 'C7未使能或反馈已超时，不能开始连续移动。'
        return
    }
    $script:C7ContinuousDirection = $Delta
    $script:C7ContinuousCommandPulse = [int]$script:C7CurrentPulse
    $script:C7ContinuousTickCount = 0
    Update-C7ContinuousButtons
    $directionName = if ($Delta -lt 0) { '减小脉宽' } else { '增加脉宽' }
    $speedName = [string]$c7SpeedBox.SelectedItem
    Add-Log "C7开始连续$directionName（$speedName）；点击保持停止。"
    Invoke-C7ContinuousTick
}

function Start-C7AutoMove([int]$TargetPulse, [string]$Name) {
    if (-not $script:C7SessionEnabled -or -not (Test-C7FeedbackFresh)) {
        Add-Log 'C7未使能或反馈已超时，拒绝位置移动。'
        return
    }
    $target = Limit-C7Pulse $TargetPulse
    $current = [int]$script:C7CurrentPulse
    Stop-C7Continuous $false
    if ($target -eq $current) {
        Add-Log "C7已经位于$Name：$target us。"
        return
    }
    if (Send-C7Command 2 $target "$Name / $target us（固件最快速度）") {
        Save-C7LastPulse $target
        Add-Log "C7已按固件最快速度前往$Name：$current -> $target us。"
    }
}
function Stop-C7Continuous([bool]$SendHold) {
    $wasMoving = ($script:C7ContinuousDirection -ne 0)
    $script:C7ContinuousDirection = 0
    $script:C7ContinuousTickCount = 0
    if ($null -ne $script:C7ContinuousCommandPulse) {
        Save-C7LastPulse ([int]$script:C7ContinuousCommandPulse)
    }
    $script:C7ContinuousCommandPulse = $null
    Update-C7ContinuousButtons
    if ($SendHold -and $script:C7SessionEnabled -and (Test-C7FeedbackFresh)) {
        [void](Send-C7Command 0 0 '保持当前指令脉宽')
    }
    if ($wasMoving) { Add-Log 'C7连续移动已停止并保持。' }
}

function Hold-C7 {
    Stop-C7Continuous $true
}

function Move-C7Step([int]$Delta) {
    Start-C7Continuous $Delta
}

function Stop-C7Emergency([string]$Reason) {
    Stop-C7Continuous $false
    if (Test-PortOpen) {
        [void](Send-C7Command 255 0 '急停锁定' $true)
    }
    $script:C7SessionEnabled = $false
    Update-C7PositionControls
    Add-Log "$Reason；软件急停只锁定当前指令脉宽，不会切断舵机电源。"
}

function Record-C7Position([int]$Index) {
    if (-not $script:C7SessionEnabled -or -not (Test-C7FeedbackFresh) -or
        $script:C7State -ne 1) {
        Add-Log 'C7必须处于稳定保持状态，才能记录位置。'
        return
    }
    $key = "position_${Index}_us"
    $script:C7Config[$key] = [int]$script:C7CurrentPulse
    try {
        Save-C7Config
        $name = if ($Index -eq 0) { '0位' } else { "位置$Index" }
        Add-Log "C7已记录$name：$($script:C7CurrentPulse) us。"
    } catch {
        Add-Log "C7位置保存失败：$($_.Exception.Message)"
    }
    Update-C7PositionControls
}

function Move-ToC7Position([int]$Index) {
    $key = "position_${Index}_us"
    $pulse = $script:C7Config[$key]
    if ($null -eq $pulse) {
        Add-Log "C7位置$Index尚未记录，拒绝移动。"
        return
    }
    if (-not $script:C7SessionEnabled -or -not (Test-C7FeedbackFresh)) {
        Add-Log 'C7未使能或反馈已超时，拒绝移动。'
        return
    }
    $name = if ($Index -eq 0) { '回0位' } else { "去位置$Index" }
    Start-C7AutoMove ([int]$pulse) $name
}

function Update-C7Feedback([byte[]]$Frame) {
    $script:C7State = [int]$Frame[3]
    $script:C7CurrentPulse = ([int]$Frame[4] -shl 8) -bor [int]$Frame[5]
    $script:C7TargetPulse = ([int]$Frame[6] -shl 8) -bor [int]$Frame[7]
    $script:C7FeedbackSeen = $true
    $script:C7FeedbackTimeoutLatched = $false
    $script:C7LastFeedbackTime = [DateTime]::Now
    $c7CurrentValue.Text = "$($script:C7CurrentPulse) us"
    $c7TargetValue.Text = "$($script:C7TargetPulse) us"
    $currentAngle = Convert-C7PulseToAngle ([int]$script:C7CurrentPulse)
    $targetAngle = Convert-C7PulseToAngle ([int]$script:C7TargetPulse)
    $c7AngleValue.Text = "参考角度：$currentAngle° -> $targetAngle°"
    if ($script:C7StateNames.ContainsKey($script:C7State)) {
        $c7StateValue.Text = $script:C7StateNames[$script:C7State]
    } else {
        $c7StateValue.Text = "未知状态 $($script:C7State)"
    }
    $c7StateValue.ForeColor = if ($script:C7State -eq 255) { 'DarkRed' } else { 'DarkGreen' }
    $c7FeedbackAge.Text = '0x0B反馈正常'
    $c7FeedbackAge.ForeColor = 'DarkGreen'
    Update-C7PositionControls
}
function Start-CalibrationRepeat([int]$Action) {
    if (-not $script:HeartbeatEnabled -or -not (Test-PortOpen) -or -not $actionGroup.Enabled) {
        return
    }
    if ($null -ne $script:AutoTargetPulse) {
        Stop-AutoTarget '自动开合已取消，切换为手动标定。' $true
    }
    $script:CalibrationDirection = $Action
    $script:CalibrationHoldTicks = 0
    $script:CalibrationSendTicks = 0
    $script:CalibrationRepeated = $false
}

function Stop-CalibrationRepeat([int]$Action, [bool]$SendSingleStep) {
    if ($null -ne $script:AutoTargetPulse) { return }
    if ($script:CalibrationDirection -ne $Action) { return }

    $wasRepeated = $script:CalibrationRepeated
    $script:CalibrationDirection = 0
    $script:CalibrationHoldTicks = 0
    $script:CalibrationSendTicks = 0
    $script:CalibrationRepeated = $false

    if ($wasRepeated) {
        [void](Send-Frame (New-CalibrationFrame 0) $true)
        Add-Log '连续标定已停止，保持当前脉宽。'
    } elseif ($SendSingleStep) {
        if ($Action -eq 1) {
            Send-CalibrationCommand 1 '脉宽减小10us'
        } elseif ($Action -eq 2) {
            Send-CalibrationCommand 2 '脉宽增加10us'
        }
    }
}

function Stop-AutoTarget([string]$Reason, [bool]$SendHold = $true) {
    if ($null -eq $script:AutoTargetPulse) { return }

    $script:AutoTargetPulse = $null
    $script:AutoTargetName = ''
    $script:CalibrationDirection = 0
    $script:CalibrationHoldTicks = 0
    $script:CalibrationSendTicks = 0
    $script:CalibrationRepeated = $false

    if ($SendHold -and $script:HeartbeatEnabled -and (Test-PortOpen)) {
        [void](Send-Frame (New-CalibrationFrame 0) $true)
    }
    if ($Reason) { Add-Log $Reason }
}

function Start-AutoTarget([int]$TargetPulse, [string]$Name) {
    if (-not $script:HeartbeatEnabled -or -not (Test-PortOpen) -or -not $actionGroup.Enabled) {
        Add-Log '请先连接串口并点击“开始安全测试”。'
        return
    }
    if ($null -eq $script:CurrentPulse -or
        ([DateTime]::Now - $script:LastFeedbackTime).TotalMilliseconds -gt 1000) {
        Add-Log '当前脉宽反馈无效，拒绝自动开合。'
        return
    }

    if ($null -ne $script:AutoTargetPulse) {
        Stop-AutoTarget '上一条自动开合命令已取消。' $true
    } elseif ($script:CalibrationDirection -ne 0) {
        Stop-CalibrationRepeat $script:CalibrationDirection $false
    }

    $script:CommandedPulse = [int]$script:CurrentPulse
    if ($script:CommandedPulse -eq $TargetPulse) {
        [void](Send-Frame (New-CalibrationFrame 0) $true)
        Add-Log "$Name：当前已经是 $TargetPulse us。"
        return
    }

    $script:AutoTargetPulse = $TargetPulse
    $script:AutoTargetName = $Name
    $script:CalibrationDirection = if ($TargetPulse -lt $script:CommandedPulse) { 1 } else { 2 }
    $script:CalibrationHoldTicks = 15
    $script:CalibrationSendTicks = 0
    $script:CalibrationRepeated = $true
    Add-Log "$Name 开始：$($script:CommandedPulse) us -> $TargetPulse us，速度=$($speedBox.SelectedItem)。"
}
function Cancel-CalibrationRepeat {
    if ($null -ne $script:AutoTargetPulse) {
        Stop-AutoTarget '自动开合已取消，保持当前位置。' $true
    } elseif ($script:CalibrationDirection -ne 0) {
        Stop-CalibrationRepeat $script:CalibrationDirection $false
    }
}

function Update-Feedback([byte[]]$Frame) {
    [int]$state = $Frame[3]
    [int]$pulse = ([int]$Frame[4] -shl 8) -bor [int]$Frame[5]
    $script:FeedbackSeen = $true
    $script:FeedbackTimeoutLatched = $false
    $script:LastFeedbackTime = [DateTime]::Now
    $script:CurrentPulse = $pulse
    if ($null -eq $script:AutoTargetPulse -and $script:CalibrationDirection -eq 0) {
        $script:CommandedPulse = $pulse
    }
    $pulseValue.Text = "$pulse us"
    if ($script:StateNames.ContainsKey($state)) {
        $stateValue.Text = $script:StateNames[$state]
    } else {
        $stateValue.Text = "未知状态 $state"
    }
    $stateValue.ForeColor = if ($state -eq 255) { 'DarkRed' } else { 'DarkGreen' }
    $feedbackAge.Text = '反馈正常'
    $feedbackAge.ForeColor = 'DarkGreen'
    if ((Test-PortOpen) -and -not $script:HeartbeatEnabled) {
        $startButton.Enabled = $true
    }
}

function Process-RxBuffer {
    while ($script:RxBuffer.Count -ge 2) {
        [int]$headIndex = $script:RxBuffer.IndexOf([byte]0xFD)
        if ($headIndex -lt 0) {
            $script:RxBuffer.Clear()
            return
        }
        if ($headIndex -gt 0) {
            $script:RxBuffer.RemoveRange(0, $headIndex)
        }
        if ($script:RxBuffer.Count -lt 2) { return }

        [int]$length = $script:RxBuffer[1]
        if ($length -lt 1 -or $length -gt 20) {
            $script:RxBuffer.RemoveAt(0)
            continue
        }

        [int]$total = $length + 3
        if ($script:RxBuffer.Count -lt $total) { return }

        [byte[]]$frame = $script:RxBuffer.GetRange(0, $total).ToArray()
        $script:RxBuffer.RemoveRange(0, $total)

        [byte]$xorValue = 0
        for ($i = 2; $i -lt (2 + $length); $i++) {
            $xorValue = $xorValue -bxor $frame[$i]
        }
        if ($xorValue -ne $frame[$total - 1]) {
            $script:RxChecksumErrorCount++
            Add-Log ('RX校验失败 ' + (Format-Frame $frame))
            continue
        }

        $script:RxValidFrameCount++
        $script:RxLastCode = [int]$frame[2]

        if ($frame[2] -eq 0x08 -and $length -eq 4 -and $Mode -ne 'C7') {
            Update-Feedback $frame
        } elseif ($frame[2] -eq 0x0B -and $length -eq 6 -and $Mode -ne 'C6') {
            Update-C7Feedback $frame
        }
    }
}

function Read-SerialData {
    if (-not (Test-PortOpen)) { return }
    try {
        [int]$count = $script:Port.BytesToRead
        if ($count -le 0) { return }
        [byte[]]$bytes = New-Object byte[] $count
        [void]$script:Port.Read($bytes, 0, $count)
        $script:RxByteCount += $count
        $script:RxLastSample = (($bytes | Select-Object -First 16 | ForEach-Object { '{0:X2}' -f $_ }) -join ' ')
        if (-not $script:RxActivityLogged) {
            $script:RxActivityLogged = $true
            Add-Log "已收到原始串口字节：$($script:RxLastSample)"
        }
        foreach ($byte in $bytes) { $script:RxBuffer.Add($byte) }
        Process-RxBuffer
    } catch {
        Add-Log "读取失败：$($_.Exception.Message)"
    }
}

function Refresh-Ports {
    $portBox.Items.Clear()
    $names = [IO.Ports.SerialPort]::GetPortNames() | Sort-Object
    foreach ($name in $names) {
        $display = $name
        try {
            $device = Get-CimInstance Win32_PnPEntity -Filter "Name LIKE '%($name)%'" -ErrorAction Stop | Select-Object -First 1
            if ($device.Name) { $display = "$name - $($device.Name)" }
        } catch {}
        [void]$portBox.Items.Add($display)
    }
    if ($portBox.Items.Count -gt 0) {
        $index = 0
        for ($i = 0; $i -lt $portBox.Items.Count; $i++) {
            if ($portBox.Items[$i] -match 'CH340|USB-SERIAL') {
                $index = $i
                break
            }
        }
        $portBox.SelectedIndex = $index
    }
    Add-Log "检测到 $($names.Count) 个串口。"
}

function Disconnect-ServoTool([string]$Reason) {
    $script:CalibrationDirection = 0
    $script:CalibrationHoldTicks = 0
    $script:CalibrationSendTicks = 0
    $script:CalibrationRepeated = $false
    $script:AutoTargetPulse = $null
    $script:AutoTargetName = ''
    $script:CurrentPulse = $null
    $script:CommandedPulse = $null
    $script:C7CurrentPulse = $null
    $script:C7TargetPulse = $null
    $script:C7SessionEnabled = $false
    $script:HeartbeatEnabled = $false
    if (Test-PortOpen) {
        [void](Send-Frame (New-ZeroMotionFrame) $true)
        if ($Mode -ne 'C7') {
            [void](Send-Frame (New-BucketFrame 255) $true)
        }
        if ($Mode -ne 'C6') {
            [void](Send-Frame (New-C7Frame 255 0) $true)
        }
        try { $script:Port.Close() } catch {}
        try { $script:Port.Dispose() } catch {}
    }
    $script:Port = $null
    $script:FeedbackSeen = $false
    $script:FeedbackTimeoutLatched = $false
    $script:C7FeedbackSeen = $false
    $script:C7FeedbackTimeoutLatched = $false
    $script:RxBuffer.Clear()
    $connectButton.Text = '连接'
    $connectionLabel.Text = '未连接'
    $connectionLabel.ForeColor = 'DarkRed'
    $startButton.Enabled = $false
    $startButton.Text = '开始安全测试'
    $actionGroup.Enabled = $false
    if ($Reason) { Add-Log "已断开：$Reason；已尝试发送零速度和舵机急停。" }
    $c7StateValue.Text = '尚未收到0x0B反馈'
    $c7CurrentValue.Text = '---- us'
    $c7TargetValue.Text = '---- us'
    $c7FeedbackAge.Text = '等待0x0B反馈'
    Update-C7PositionControls
}

$refreshButton.Add_Click({ Refresh-Ports })

$connectButton.Add_Click({
    if (Test-PortOpen) {
        Disconnect-ServoTool '用户断开'
        return
    }
    if ($null -eq $portBox.SelectedItem) {
        Add-Log '没有可用串口，请先刷新。'
        return
    }
    $name = ([string]$portBox.SelectedItem -split ' ')[0]
    try {
        $script:Port = New-Object IO.Ports.SerialPort($name,115200,'None',8,'One')
        $script:Port.Handshake = 'None'
        $script:Port.DtrEnable = $false
        # Keep both modem-control outputs deasserted during normal application use.
        # FlyMcu controls these lines itself when it needs to enter the ROM bootloader.
        $script:Port.RtsEnable = $false
        $script:Port.ReadTimeout = 50
        $script:Port.WriteTimeout = 200
        $script:Port.Open()
        $script:HeartbeatEnabled = $false
        $script:FeedbackSeen = $false
        $script:AutoTargetPulse = $null
        $script:AutoTargetName = ''
        $script:CurrentPulse = $null
        $script:CommandedPulse = $null
        $script:C7FeedbackSeen = $false
        $script:C7FeedbackTimeoutLatched = $false
        $script:C7LastFeedbackTime = [DateTime]::MinValue
        $script:C7CurrentPulse = $null
        $script:C7TargetPulse = $null
        $script:C7State = 0
        $script:C7SessionEnabled = $false
        $script:C7NextQueryTime = [DateTime]::Now
        $script:RxBuffer.Clear()
        $script:RxByteCount = 0L
        $script:RxValidFrameCount = 0L
        $script:RxChecksumErrorCount = 0L
        $script:RxLastCode = $null
        $script:RxLastSample = '--'
        $script:RxActivityLogged = $false
        $rxDiagnostic.Text = '原始串口：RX 0 B / 有效帧 0 / 校验错误 0 / 最后帧 --'
        $connectButton.Text = '断开'
        $feedbackCodeText = switch ($Mode) {
            'C6' { '等待0x08反馈' }
            'C7' { '等待0x0B反馈' }
            default { '等待0x08和0x0B反馈' }
        }
        $connectionLabel.Text = "已连接 $name / $feedbackCodeText"
        $connectionLabel.ForeColor = 'DarkOrange'
        Add-Log '连接成功：尚未发送任何动作，请等待STM32反馈。'
    } catch {
        if ($script:Port) {
            try { $script:Port.Dispose() } catch {}
            $script:Port = $null
        }
        Add-Log "连接失败：$($_.Exception.Message)"
    }
})

$startButton.Add_Click({
    if (-not (Test-PortOpen) -or -not $script:FeedbackSeen) {
        Add-Log '尚未收到有效0x08反馈，不能开始测试。'
        return
    }
    $script:CalibrationDirection = 0
    $script:CalibrationHoldTicks = 0
    $script:CalibrationSendTicks = 0
    $script:CalibrationRepeated = $false
    $script:AutoTargetPulse = $null
    $script:AutoTargetName = ''
    $script:CurrentPulse = 1500
    $script:CommandedPulse = 1500
    [void](Send-Frame (New-ZeroMotionFrame) $true)
    Send-BucketCommand 5 '复位/解除急停并回1500us'
    $script:HeartbeatEnabled = $true
    $script:LastFeedbackTime = [DateTime]::Now
    $startButton.Enabled = $false
    $startButton.Text = '安全测试运行中'
    $actionGroup.Enabled = $true
    $connectionLabel.Text = '已连接 / 零速度心跳运行中'
    $connectionLabel.ForeColor = 'DarkGreen'
    Add-Log '已启动100ms零速度心跳；四个车轮目标速度均为0。'
})

$decreaseButton.Add_MouseDown({
    param($sender, $eventArgs)
    if ($eventArgs.Button -eq [Windows.Forms.MouseButtons]::Left) { Start-CalibrationRepeat 1 }
})
$decreaseButton.Add_MouseUp({
    param($sender, $eventArgs)
    if ($eventArgs.Button -eq [Windows.Forms.MouseButtons]::Left) { Stop-CalibrationRepeat 1 $true }
})
$decreaseButton.Add_MouseLeave({ Stop-CalibrationRepeat 1 $false })

$increaseButton.Add_MouseDown({
    param($sender, $eventArgs)
    if ($eventArgs.Button -eq [Windows.Forms.MouseButtons]::Left) { Start-CalibrationRepeat 2 }
})
$increaseButton.Add_MouseUp({
    param($sender, $eventArgs)
    if ($eventArgs.Button -eq [Windows.Forms.MouseButtons]::Left) { Stop-CalibrationRepeat 2 $true }
})
$increaseButton.Add_MouseLeave({ Stop-CalibrationRepeat 2 $false })
$holdButton.Add_Click({
    Cancel-CalibrationRepeat
    Send-CalibrationCommand 0 '保持当前脉宽'
    Send-BucketCommand 0 '停止并保持'
})
$resetButton.Add_Click({
    Cancel-CalibrationRepeat
    Send-BucketCommand 5 '复位/解除急停并回1500us'
    $script:CurrentPulse = 1500
    $script:CommandedPulse = 1500
})
$openButton.Add_Click({ Start-AutoTarget 550 '一键开爪' })
$closeButton.Add_Click({ Start-AutoTarget 2450 '一键合爪' })
$emergencyButton.Add_Click({
    Cancel-CalibrationRepeat
    [void](Send-Frame (New-ZeroMotionFrame) $true)
    Send-BucketCommand 255 '舵机急停锁存'
    Add-Log '注意：软件急停只保持当前脉宽，不会切断舵机电源。'
})

$c7PresetButton.Add_Click({ Start-C7Preset })
$c7DecreaseButton.Add_Click({ Move-C7Step -5 })
$c7IncreaseButton.Add_Click({ Move-C7Step 5 })
$c7HoldButton.Add_Click({ Hold-C7 })
$c7EmergencyButton.Add_Click({
    [void](Send-Frame (New-ZeroMotionFrame) $true)
    Stop-C7Emergency 'C7已发送急停锁定'
})

foreach ($button in $script:C7RecordButtons) {
    $button.Add_Click({
        param($sender, $eventArgs)
        Record-C7Position ([int]$sender.Tag)
    })
}

foreach ($button in $script:C7MoveButtons) {
    $button.Add_Click({
        param($sender, $eventArgs)
        Move-ToC7Position ([int]$sender.Tag)
    })
}

$calibrationTimer = New-Object Windows.Forms.Timer
$calibrationTimer.Interval = 20
$calibrationTimer.Add_Tick({
    if ($script:CalibrationDirection -ne 0) {
        if ($script:HeartbeatEnabled -and (Test-PortOpen) -and $actionGroup.Enabled) {
            $script:CalibrationHoldTicks++
            if ($script:CalibrationHoldTicks -ge 15) {
                $script:CalibrationSendTicks++
                $requiredTicks = switch ($speedBox.SelectedIndex) {
                    0 { 3 }
                    2 { 1 }
                    default { 2 }
                }
                if ($script:CalibrationSendTicks -ge $requiredTicks) {
                    $script:CalibrationSendTicks = 0
                    if (-not $script:CalibrationRepeated) {
                        Add-Log "连续标定开始：$($speedBox.SelectedItem)"
                    }
                    $script:CalibrationRepeated = $true
                    if ($null -ne $script:AutoTargetPulse) {
                        $direction = $script:CalibrationDirection
                        $sent = Send-Frame (New-CalibrationFrame $direction) $true
                        if ($sent) {
                            if ($direction -eq 1) {
                                $script:CommandedPulse = [Math]::Max($script:AutoTargetPulse, $script:CommandedPulse - 10)
                            } else {
                                $script:CommandedPulse = [Math]::Min($script:AutoTargetPulse, $script:CommandedPulse + 10)
                            }
                            if ($script:CommandedPulse -eq $script:AutoTargetPulse) {
                                $completedName = $script:AutoTargetName
                                $completedTarget = $script:AutoTargetPulse
                                Stop-AutoTarget '' $true
                                Add-Log "$completedName 完成：已到达 $completedTarget us。"
                            }
                        }
                    } elseif ($script:CalibrationDirection -eq 1) {
                        Send-CalibrationCommand 1 '连续减小10us' $true
                    } elseif ($script:CalibrationDirection -eq 2) {
                        Send-CalibrationCommand 2 '连续增加10us' $true
                    }
                }
            }
        } else {
            if ($null -ne $script:AutoTargetPulse) {
                Stop-AutoTarget '' $false
            } else {
                $script:CalibrationDirection = 0
                $script:CalibrationHoldTicks = 0
                $script:CalibrationSendTicks = 0
                $script:CalibrationRepeated = $false
            }
        }
    }
})
$calibrationTimer.Start()

$c7ContinuousTimer = New-Object Windows.Forms.Timer
$c7ContinuousTimer.Interval = 50
$c7ContinuousTimer.Add_Tick({
    if ($script:C7ContinuousDirection -ne 0) {
        $script:C7ContinuousTickCount++
        $requiredTicks = if ($c7SpeedBox.SelectedIndex -eq 1) { 1 } else { 2 }
        if ($script:C7ContinuousTickCount -ge $requiredTicks) {
            $script:C7ContinuousTickCount = 0
            Invoke-C7ContinuousTick
        }
    }
})
$c7ContinuousTimer.Start()

$timer = New-Object Windows.Forms.Timer
$timer.Interval = 100
$timer.Add_Tick({
    if ($Mode -ne 'C6' -and (Test-PortOpen) -and
        [DateTime]::Now -ge $script:C7NextQueryTime) {
        [void](Send-C7Command 7 0 '查询状态' $true)
    }
    if ($Mode -ne 'C7' -and $script:HeartbeatEnabled -and (Test-PortOpen)) {
        [void](Send-Frame (New-ZeroMotionFrame) $true)
    }
    Read-SerialData
    if (Test-PortOpen) {
        $lastCodeText = if ($null -eq $script:RxLastCode) { '--' } else { '0x{0:X2}' -f $script:RxLastCode }
        $rxDiagnostic.Text = "原始串口：RX $($script:RxByteCount) B / 有效帧 $($script:RxValidFrameCount) / 校验错误 $($script:RxChecksumErrorCount) / 最后帧 $lastCodeText"
        if ($script:RxByteCount -eq 0) {
            $rxDiagnostic.ForeColor = 'DarkRed'
        } elseif ($script:RxValidFrameCount -eq 0) {
            $rxDiagnostic.ForeColor = 'DarkOrange'
        } else {
            $rxDiagnostic.ForeColor = 'DarkGreen'
        }
    }
    if ($Mode -ne 'C7' -and $script:FeedbackSeen) {
        $age = ([DateTime]::Now - $script:LastFeedbackTime).TotalMilliseconds
        if ($age -le 1000) {
            $feedbackAge.Text = "反馈正常 / $([int]$age) ms"
            $feedbackAge.ForeColor = 'DarkGreen'
        } else {
            $feedbackAge.Text = '反馈超时，已发送急停'
            $feedbackAge.ForeColor = 'DarkRed'
            if ($script:HeartbeatEnabled -and -not $script:FeedbackTimeoutLatched) {
                $script:FeedbackTimeoutLatched = $true
                $script:CalibrationDirection = 0
                $script:CalibrationHoldTicks = 0
                $script:CalibrationSendTicks = 0
                $script:CalibrationRepeated = $false
                $script:AutoTargetPulse = $null
                $script:AutoTargetName = ''
                $script:HeartbeatEnabled = $false
                [void](Send-Frame (New-ZeroMotionFrame) $true)
                [void](Send-Frame (New-BucketFrame 255) $true)
                $actionGroup.Enabled = $false
                $startButton.Enabled = $false
                Add-Log '超过1000ms未收到舵机反馈：已停止心跳并发送急停。'
            }
        }
    }
    if ($Mode -ne 'C6' -and $script:C7FeedbackSeen) {
        $c7Age = ([DateTime]::Now - $script:C7LastFeedbackTime).TotalMilliseconds
        if ($c7Age -le 1000) {
            $c7FeedbackAge.Text = "0x0B反馈正常 / $([int]$c7Age) ms"
            $c7FeedbackAge.ForeColor = 'DarkGreen'
        } else {
            $c7FeedbackAge.Text = '0x0B反馈超时'
            $c7FeedbackAge.ForeColor = 'DarkRed'
            if ($script:C7SessionEnabled -and -not $script:C7FeedbackTimeoutLatched) {
                $script:C7FeedbackTimeoutLatched = $true
                Stop-C7Continuous $false
                $script:C7SessionEnabled = $false
                [void](Send-Frame (New-ZeroMotionFrame) $true)
                [void](Send-Frame (New-C7Frame 255 0) $true)
                Update-C7PositionControls
                Add-Log 'C7超过1000ms未收到0x0B反馈：已停止移动并发送急停锁定。'
            } else {
                Update-C7PositionControls
            }
        }
    }
})
$timer.Start()
$form.Add_Deactivate({ Cancel-CalibrationRepeat })

$form.Add_FormClosing({
    try { Disconnect-ServoTool '关闭工具' } catch {}
})

Add-Log '协议自检通过：0x07/0x09帧和异或校验正确。'
Add-Log 'C7协议自检通过：0x0A/0x0B、限幅、角度换算和JSON读写正确。'
if ($script:C7ConfigLoadMessage) {
    Add-Log $script:C7ConfigLoadMessage
}
Update-C7PositionControls
Refresh-Ports
[void]$form.ShowDialog()
