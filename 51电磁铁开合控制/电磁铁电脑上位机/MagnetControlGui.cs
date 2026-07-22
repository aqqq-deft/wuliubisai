using System;
using System.Drawing;
using System.IO.Ports;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace MagnetControlGui
{
    internal static class Program
    {
        [STAThread]
        private static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
        }
    }

    internal sealed class MainForm : Form
    {
        private const int BaudRate = 9600;
        private readonly Regex statusRegex = new Regex(
            "^M1=(ON|OFF) M2=(ON|OFF) M3=(ON|OFF)$",
            RegexOptions.Compiled);

        private SerialPort serialPort;
        private ComboBox portCombo;
        private Button refreshButton;
        private Button connectButton;
        private Label connectionLabel;
        private Label summaryLabel;
        private TextBox logBox;
        private readonly Button[] magnetButtons = new Button[3];
        private readonly bool?[] magnetStates = new bool?[3];
        private Button allOnButton;
        private Button allOffButton;
        private Button readStatusButton;

        public MainForm()
        {
            InitializeWindow();
            BuildInterface();
            RefreshPorts();
            SetConnectedUi(false);
        }

        private void InitializeWindow()
        {
            Text = "三路电磁铁控制上位机";
            StartPosition = FormStartPosition.CenterScreen;
            ClientSize = new Size(720, 610);
            MinimumSize = new Size(700, 620);
            BackColor = Color.FromArgb(243, 245, 247);
            Font = new Font("Microsoft YaHei UI", 10F, FontStyle.Regular);
            AutoScaleMode = AutoScaleMode.Dpi;
            FormClosing += MainForm_FormClosing;
        }

        private void BuildInterface()
        {
            Label title = new Label();
            title.Text = "STC89C52RC 三路电磁铁控制";
            title.Font = new Font("Microsoft YaHei UI", 20F, FontStyle.Bold);
            title.TextAlign = ContentAlignment.MiddleCenter;
            title.SetBounds(20, 15, 680, 45);
            Controls.Add(title);

            GroupBox connectionGroup = new GroupBox();
            connectionGroup.Text = "串口连接";
            connectionGroup.SetBounds(20, 70, 680, 105);
            Controls.Add(connectionGroup);

            Label portLabel = new Label();
            portLabel.Text = "串口：";
            portLabel.AutoSize = true;
            portLabel.Location = new Point(18, 32);
            connectionGroup.Controls.Add(portLabel);

            portCombo = new ComboBox();
            portCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            portCombo.SetBounds(75, 27, 315, 30);
            connectionGroup.Controls.Add(portCombo);

            refreshButton = new Button();
            refreshButton.Text = "刷新";
            refreshButton.SetBounds(405, 26, 90, 32);
            refreshButton.Click += delegate { RefreshPorts(); };
            connectionGroup.Controls.Add(refreshButton);

            connectButton = new Button();
            connectButton.Text = "连接";
            connectButton.SetBounds(510, 26, 145, 32);
            connectButton.Click += delegate { ToggleConnection(); };
            connectionGroup.Controls.Add(connectButton);

            Label parametersLabel = new Label();
            parametersLabel.Text = "通信参数：9600 bps，8N1";
            parametersLabel.AutoSize = true;
            parametersLabel.Location = new Point(18, 70);
            connectionGroup.Controls.Add(parametersLabel);

            connectionLabel = new Label();
            connectionLabel.Text = "未连接";
            connectionLabel.ForeColor = Color.FromArgb(180, 35, 24);
            connectionLabel.TextAlign = ContentAlignment.MiddleRight;
            connectionLabel.SetBounds(425, 67, 230, 24);
            connectionGroup.Controls.Add(connectionLabel);

            GroupBox magnetGroup = new GroupBox();
            magnetGroup.Text = "独立控制（再次点击同一按钮即可切换）";
            magnetGroup.SetBounds(20, 188, 680, 145);
            Controls.Add(magnetGroup);

            for (int index = 0; index < 3; index++)
            {
                int channel = index + 1;
                Button button = new Button();
                button.Text = "M" + channel + "\r\n状态未知";
                button.Font = new Font("Microsoft YaHei UI", 12F, FontStyle.Bold);
                button.BackColor = Color.FromArgb(208, 213, 221);
                button.FlatStyle = FlatStyle.Flat;
                button.SetBounds(18 + index * 220, 30, 205, 95);
                button.Tag = channel;
                button.Click += MagnetButton_Click;
                magnetGroup.Controls.Add(button);
                magnetButtons[index] = button;
            }

            allOnButton = new Button();
            allOnButton.Text = "全部通电";
            allOnButton.Font = new Font("Microsoft YaHei UI", 11F, FontStyle.Bold);
            allOnButton.BackColor = Color.FromArgb(253, 176, 34);
            allOnButton.FlatStyle = FlatStyle.Flat;
            allOnButton.SetBounds(20, 347, 210, 50);
            allOnButton.Click += delegate { SetAll(true); };
            Controls.Add(allOnButton);

            allOffButton = new Button();
            allOffButton.Text = "全部断电";
            allOffButton.Font = new Font("Microsoft YaHei UI", 11F, FontStyle.Bold);
            allOffButton.BackColor = Color.FromArgb(240, 68, 56);
            allOffButton.ForeColor = Color.White;
            allOffButton.FlatStyle = FlatStyle.Flat;
            allOffButton.SetBounds(245, 347, 210, 50);
            allOffButton.Click += delegate { SetAll(false); };
            Controls.Add(allOffButton);

            readStatusButton = new Button();
            readStatusButton.Text = "读取状态";
            readStatusButton.SetBounds(470, 347, 230, 50);
            readStatusButton.Click += delegate { QueryStatus(true); };
            Controls.Add(readStatusButton);

            GroupBox statusGroup = new GroupBox();
            statusGroup.Text = "当前状态";
            statusGroup.SetBounds(20, 410, 680, 60);
            Controls.Add(statusGroup);

            summaryLabel = new Label();
            summaryLabel.Text = "请先选择 CH340 串口并连接";
            summaryLabel.SetBounds(15, 24, 650, 25);
            statusGroup.Controls.Add(summaryLabel);

            GroupBox logGroup = new GroupBox();
            logGroup.Text = "通信记录";
            logGroup.SetBounds(20, 482, 680, 92);
            Controls.Add(logGroup);

            logBox = new TextBox();
            logBox.Multiline = true;
            logBox.ReadOnly = true;
            logBox.ScrollBars = ScrollBars.Vertical;
            logBox.Font = new Font("Consolas", 9.5F);
            logBox.SetBounds(12, 22, 655, 58);
            logGroup.Controls.Add(logBox);

            Label safetyLabel = new Label();
            safetyLabel.Text = "关闭窗口或主动断开时会先发送 ALL OFF；意外拔线时请手动关闭12V。";
            safetyLabel.ForeColor = Color.FromArgb(181, 71, 8);
            safetyLabel.SetBounds(22, 582, 675, 24);
            Controls.Add(safetyLabel);
        }

        private void RefreshPorts()
        {
            string previous = portCombo.SelectedItem as string;
            string[] ports = SerialPort.GetPortNames();
            Array.Sort(ports, StringComparer.OrdinalIgnoreCase);
            portCombo.Items.Clear();
            portCombo.Items.AddRange(ports);

            if (ports.Length == 0)
            {
                AppendLog("没有发现串口，请检查 CH340 是否已连接。");
                return;
            }

            int selectedIndex = Array.IndexOf(ports, previous);
            if (selectedIndex < 0)
            {
                selectedIndex = Array.FindIndex(ports, delegate(string port)
                {
                    return string.Equals(port, "COM5", StringComparison.OrdinalIgnoreCase);
                });
            }
            portCombo.SelectedIndex = selectedIndex >= 0 ? selectedIndex : 0;
        }

        private void ToggleConnection()
        {
            if (IsConnected())
            {
                Disconnect(true);
            }
            else
            {
                ConnectPort();
            }
        }

        private void ConnectPort()
        {
            if (portCombo.SelectedItem == null)
            {
                MessageBox.Show("请连接 CH340，点击“刷新”，然后选择对应的 COM 口。",
                    "未选择串口", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            string portName = portCombo.SelectedItem.ToString();
            try
            {
                serialPort = new SerialPort(portName, BaudRate, Parity.None, 8, StopBits.One);
                serialPort.NewLine = "\r\n";
                serialPort.ReadTimeout = 150;
                serialPort.WriteTimeout = 500;
                serialPort.DtrEnable = false;
                serialPort.RtsEnable = false;
                serialPort.Open();
                serialPort.DiscardInBuffer();
            }
            catch (Exception ex)
            {
                CloseSerialPort();
                MessageBox.Show("无法打开 " + portName + "。\r\n\r\n请先关闭 SSCOM 和 STC-ISP 的串口。\r\n\r\n错误：" + ex.Message,
                    "串口打开失败", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            connectionLabel.Text = "已连接：" + portName;
            connectionLabel.ForeColor = Color.FromArgb(6, 118, 71);
            connectButton.Text = "断开";
            AppendLog("已连接 " + portName + "，波特率 9600。");
            SetConnectedUi(true);

            if (!QueryStatus(false))
            {
                MessageBox.Show("串口已打开，但没有收到正确的 STATUS 回复。\r\n\r\n请检查 TXD/RXD 是否交叉连接、开发板是否供电。",
                    "下位机无响应", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Disconnect(false);
            }
        }

        private void MagnetButton_Click(object sender, EventArgs e)
        {
            Button button = sender as Button;
            if (button == null)
            {
                return;
            }

            int channel = (int)button.Tag;
            if (!magnetStates[channel - 1].HasValue && !QueryStatus(true))
            {
                return;
            }

            bool target = !magnetStates[channel - 1].Value;
            string command = "M" + channel + (target ? " ON" : " OFF");
            if (ExecuteControlCommand(command))
            {
                QueryStatus(false);
            }
        }

        private void SetAll(bool enabled)
        {
            if (ExecuteControlCommand(enabled ? "ALL ON" : "ALL OFF"))
            {
                QueryStatus(false);
            }
        }

        private bool ExecuteControlCommand(string command)
        {
            try
            {
                SendAndWait(command, false, 1200);
                return true;
            }
            catch (Exception ex)
            {
                HandleCommunicationError(ex);
                return false;
            }
        }

        private bool QueryStatus(bool showError)
        {
            try
            {
                string response = SendAndWait("STATUS", true, 1200);
                Match match = statusRegex.Match(response);
                if (!match.Success)
                {
                    throw new InvalidOperationException("无法识别状态回复：" + response);
                }

                for (int index = 0; index < 3; index++)
                {
                    magnetStates[index] = match.Groups[index + 1].Value == "ON";
                }
                UpdateMagnetButtons();
                return true;
            }
            catch (Exception ex)
            {
                SetUnknownState();
                if (showError)
                {
                    MessageBox.Show(ex.Message, "状态读取失败", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
                return false;
            }
        }

        private string SendAndWait(string command, bool expectStatus, int timeoutMilliseconds)
        {
            if (!IsConnected())
            {
                throw new InvalidOperationException("串口未连接。");
            }

            serialPort.DiscardInBuffer();
            serialPort.Write(command + "\r\n");
            AppendLog("> " + command);

            DateTime deadline = DateTime.UtcNow.AddMilliseconds(timeoutMilliseconds);
            while (DateTime.UtcNow < deadline)
            {
                try
                {
                    string line = serialPort.ReadLine().Trim();
                    if (line.Length == 0)
                    {
                        continue;
                    }
                    AppendLog("< " + line);

                    if (line.StartsWith("ERR", StringComparison.Ordinal))
                    {
                        throw new InvalidOperationException("下位机返回错误：" + line);
                    }
                    if (!expectStatus && line == "OK")
                    {
                        return line;
                    }
                    if (expectStatus && statusRegex.IsMatch(line))
                    {
                        return line;
                    }
                }
                catch (TimeoutException)
                {
                    // 在总超时时间内继续等待下一行。
                }
            }

            throw new TimeoutException("发送 " + command + " 后，没有收到正确回复。");
        }

        private void UpdateMagnetButtons()
        {
            string[] parts = new string[3];
            for (int index = 0; index < 3; index++)
            {
                int channel = index + 1;
                if (magnetStates[index] == true)
                {
                    magnetButtons[index].Text = "M" + channel + "\r\n已通电\r\n点击断电";
                    magnetButtons[index].BackColor = Color.FromArgb(18, 183, 106);
                    magnetButtons[index].ForeColor = Color.White;
                    parts[index] = "M" + channel + "=通电";
                }
                else
                {
                    magnetButtons[index].Text = "M" + channel + "\r\n已断电\r\n点击通电";
                    magnetButtons[index].BackColor = Color.FromArgb(208, 213, 221);
                    magnetButtons[index].ForeColor = Color.FromArgb(16, 24, 40);
                    parts[index] = "M" + channel + "=断电";
                }
            }
            summaryLabel.Text = string.Join("        ", parts);
        }

        private void SetUnknownState()
        {
            for (int index = 0; index < 3; index++)
            {
                magnetStates[index] = null;
                magnetButtons[index].Text = "M" + (index + 1) + "\r\n状态未知";
                magnetButtons[index].BackColor = Color.FromArgb(208, 213, 221);
                magnetButtons[index].ForeColor = Color.FromArgb(16, 24, 40);
            }
            summaryLabel.Text = "状态未知；请连接并读取状态";
        }

        private void SetConnectedUi(bool connected)
        {
            portCombo.Enabled = !connected;
            refreshButton.Enabled = !connected;
            allOnButton.Enabled = connected;
            allOffButton.Enabled = connected;
            readStatusButton.Enabled = connected;
            for (int index = 0; index < magnetButtons.Length; index++)
            {
                magnetButtons[index].Enabled = connected;
            }

            if (!connected)
            {
                connectButton.Text = "连接";
                connectionLabel.Text = "未连接";
                connectionLabel.ForeColor = Color.FromArgb(180, 35, 24);
                SetUnknownState();
            }
        }

        private void Disconnect(bool sendAllOff)
        {
            if (IsConnected() && sendAllOff)
            {
                try
                {
                    SendAndWait("ALL OFF", false, 600);
                    AppendLog("断开前已发送 ALL OFF。");
                }
                catch
                {
                    AppendLog("警告：断开前未确认 ALL OFF，请检查电磁铁并关闭12V。");
                }
            }

            CloseSerialPort();
            SetConnectedUi(false);
            AppendLog("串口已断开。");
        }

        private void CloseSerialPort()
        {
            if (serialPort != null)
            {
                try
                {
                    if (serialPort.IsOpen)
                    {
                        serialPort.Close();
                    }
                }
                catch
                {
                }
                serialPort.Dispose();
                serialPort = null;
            }
        }

        private void HandleCommunicationError(Exception ex)
        {
            MessageBox.Show("与下位机通信失败。\r\n\r\n请确认电磁铁状态，必要时关闭12V。\r\n\r\n错误：" + ex.Message,
                "串口通信错误", MessageBoxButtons.OK, MessageBoxIcon.Error);
            Disconnect(false);
        }

        private bool IsConnected()
        {
            return serialPort != null && serialPort.IsOpen;
        }

        private void AppendLog(string text)
        {
            logBox.AppendText("[" + DateTime.Now.ToString("HH:mm:ss") + "] " + text + Environment.NewLine);
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (IsConnected())
            {
                Disconnect(true);
            }
        }
    }
}
