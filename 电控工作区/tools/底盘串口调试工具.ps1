Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[Windows.Forms.Application]::EnableVisualStyles()

$script:Port = $null
$script:Motion = @(0, 0, 0)
$script:IsMoving = $false

function New-ControlFrame([int]$X, [int]$Y, [int]$Z) {
    $xd = if ($X -ge 0) { 0 } else { 1 }
    $yd = if ($Y -ge 0) { 0 } else { 1 }
    $zd = if ($Z -gt 0) { 0 } else { 1 }
    $xv = [Math]::Min([Math]::Abs($X), 300)
    $yv = [Math]::Min([Math]::Abs($Y), 300)
    $zv = [Math]::Min([Math]::Abs($Z), 100)
    [byte[]]$f = @(0xCD,0x0A,0x01,$xd,(($xv-shr 8)-band 255),($xv-band 255),$yd,(($yv-shr 8)-band 255),($yv-band 255),$zd,(($zv-shr 8)-band 255),($zv-band 255),0)
    for ($i=2; $i -lt 12; $i++) { $f[12] = $f[12] -bxor $f[$i] }
    return $f
}

$form = New-Object Windows.Forms.Form
$form.Text='CODBOT-RCU 麦轮底盘调试工具'; $form.Size='760,610'; $form.StartPosition='CenterScreen'; $form.FormBorderStyle='FixedDialog'; $form.MaximizeBox=$false
$portBox=New-Object Windows.Forms.ComboBox; $portBox.Location='20,20'; $portBox.Size='250,28'; $portBox.DropDownStyle='DropDownList'; $form.Controls.Add($portBox)
$refresh=New-Object Windows.Forms.Button; $refresh.Text='刷新串口'; $refresh.Location='280,18'; $refresh.Size='95,30'; $form.Controls.Add($refresh)
$connect=New-Object Windows.Forms.Button; $connect.Text='连接'; $connect.Location='385,18'; $connect.Size='95,30'; $form.Controls.Add($connect)
$status=New-Object Windows.Forms.Label; $status.Text='未连接（请选择CH340）'; $status.Location='495,24'; $status.Size='230,24'; $status.ForeColor='DarkRed'; $form.Controls.Add($status)
$label=New-Object Windows.Forms.Label; $label.Text='直线速度(mm/s)：'; $label.Location='20,68'; $label.AutoSize=$true; $form.Controls.Add($label)
$speed=New-Object Windows.Forms.ComboBox; $speed.Location='150,64'; $speed.Size='80,28'; $speed.DropDownStyle='DropDownList'; [void]$speed.Items.AddRange(@('100','150','200')); $speed.SelectedIndex=0; $form.Controls.Add($speed)
$label2=New-Object Windows.Forms.Label; $label2.Text='转向速度(rad/s)：'; $label2.Location='270,68'; $label2.AutoSize=$true; $form.Controls.Add($label2)
$angular=New-Object Windows.Forms.ComboBox; $angular.Location='410,64'; $angular.Size='80,28'; $angular.DropDownStyle='DropDownList'; [void]$angular.Items.AddRange(@('0.30','0.50','0.80')); $angular.SelectedIndex=0; $form.Controls.Add($angular)
$warn=New-Object Windows.Forms.Label; $warn.Text='底盘必须架空。按住才运动，松开立即停车。首次只用最低速度。'; $warn.Location='20,105'; $warn.Size='700,25'; $warn.ForeColor='DarkOrange'; $form.Controls.Add($warn)
$log=New-Object Windows.Forms.TextBox; $log.Location='20,365'; $log.Size='705,190'; $log.Multiline=$true; $log.ScrollBars='Vertical'; $log.ReadOnly=$true; $log.Font=New-Object Drawing.Font('Consolas',9); $form.Controls.Add($log)
function Add-Log([string]$t) { $log.AppendText("$(Get-Date -Format 'HH:mm:ss.fff')  $t`r`n") }
function Send-Frame([byte[]]$f,[bool]$quiet=$false) { if($null-eq $script:Port -or -not $script:Port.IsOpen){return $false}; try{$script:Port.Write($f,0,$f.Length); if(-not $quiet){Add-Log ('TX '+(($f|%{$_.ToString('X2')})-join ' '))}; return $true}catch{Add-Log "发送失败：$($_.Exception.Message)";return $false} }
function Send-Stop([string]$why) { $script:IsMoving=$false; $script:Motion=@(0,0,0); $f=New-ControlFrame 0 0 0; [void](Send-Frame $f $true); Add-Log "STOP：$why" }

function Add-MotionButton([string]$text,[int]$x,[int]$y,[int]$z,[int]$left,[int]$top) {
    $b=New-Object Windows.Forms.Button; $b.Text=$text; $b.Location="$left,$top"; $b.Size='125,48'; $b.Tag=@($x,$y,$z)
    $b.Add_MouseDown({param($sender,$e); if($null-eq $script:Port -or -not $script:Port.IsOpen){Add-Log '请先连接CH340串口。';return}; $v=[int]$speed.SelectedItem; $w=[int]([double]$angular.SelectedItem*100); $dx=[Convert]::ToInt32($sender.Tag[0]); $dy=[Convert]::ToInt32($sender.Tag[1]); $dz=[Convert]::ToInt32($sender.Tag[2]); $script:Motion=@([int]($dx*$v),[int]($dy*$v),[int]($dz*$w)); $script:IsMoving=$true; [void](Send-Frame (New-ControlFrame -X $script:Motion[0] -Y $script:Motion[1] -Z $script:Motion[2])) })
    $b.Add_MouseUp({Send-Stop '松开动作按钮'}); $b.Add_MouseLeave({if($script:IsMoving){Send-Stop '鼠标离开动作按钮'}}); $form.Controls.Add($b)
}
Add-MotionButton '前进' 1 0 0 160 145; Add-MotionButton '后退' -1 0 0 160 255; Add-MotionButton '左横移' 0 -1 0 25 200; Add-MotionButton '右横移' 0 1 0 295 200
Add-MotionButton '原地左转' 0 0 1 455 145; Add-MotionButton '原地右转' 0 0 -1 590 145; Add-MotionButton '前进左转' 1 0 1 455 255; Add-MotionButton '前进右转' 1 0 -1 590 255
$stop=New-Object Windows.Forms.Button; $stop.Text='急停 / STOP'; $stop.Location='160,200'; $stop.Size='125,48'; $stop.BackColor='IndianRed'; $stop.ForeColor='White'; $stop.Add_Click({Send-Stop '点击急停'}); $form.Controls.Add($stop)

function Refresh-Ports { $portBox.Items.Clear(); $names=[IO.Ports.SerialPort]::GetPortNames()|Sort-Object; foreach($n in $names){$display=$n; try{$d=Get-CimInstance Win32_PnPEntity -Filter "Name LIKE '%($n)%'" -ErrorAction Stop|Select-Object -First 1;if($d.Name){$display="$n - $($d.Name)"}}catch{};[void]$portBox.Items.Add($display)}; if($portBox.Items.Count){$idx=0;for($i=0;$i-lt$portBox.Items.Count;$i++){if($portBox.Items[$i]-match 'CH340|USB-SERIAL'){$idx=$i;break}};$portBox.SelectedIndex=$idx};Add-Log "检测到 $($names.Count) 个串口。" }
$refresh.Add_Click({Refresh-Ports})
$connect.Add_Click({
    if($null-ne $script:Port -and $script:Port.IsOpen){Send-Stop '断开串口';$script:Port.Close();$script:Port.Dispose();$script:Port=$null;$connect.Text='连接';$status.Text='未连接';$status.ForeColor='DarkRed';return}
    if($null-eq $portBox.SelectedItem){Add-Log '没有可用串口。';return}; $name=([string]$portBox.SelectedItem -split ' ')[0]
    try{$script:Port=New-Object IO.Ports.SerialPort($name,115200,'None',8,'One');$script:Port.Handshake='None';$script:Port.DtrEnable=$false;$script:Port.RtsEnable=$false;$script:Port.WriteTimeout=200;$script:Port.Open();$connect.Text='断开';$status.Text="已连接 $name / 115200";$status.ForeColor='DarkGreen';Add-Log '连接成功：未自动发送控制帧，请先观察 LED/C14 15 秒。'}catch{if($script:Port){$script:Port.Dispose();$script:Port=$null};Add-Log "连接失败：$($_.Exception.Message)"}
})
$timer=New-Object Windows.Forms.Timer; $timer.Interval=100; $timer.Add_Tick({if($script:IsMoving){[void](Send-Frame (New-ControlFrame -X ([int]$script:Motion[0]) -Y ([int]$script:Motion[1]) -Z ([int]$script:Motion[2])) $true)};if($script:Port -and $script:Port.IsOpen -and $script:Port.BytesToRead){$n=$script:Port.BytesToRead;$b=New-Object byte[] $n;[void]$script:Port.Read($b,0,$n);Add-Log ('RX '+(($b|%{$_.ToString('X2')})-join ' '))}});$timer.Start()
$form.Add_Deactivate({if($script:IsMoving){Send-Stop '窗口失去焦点'}})
$form.Add_FormClosing({try{Send-Stop '关闭工具'}catch{};if($script:Port){try{$script:Port.Close();$script:Port.Dispose()}catch{}}})
Refresh-Ports; [void]$form.ShowDialog()