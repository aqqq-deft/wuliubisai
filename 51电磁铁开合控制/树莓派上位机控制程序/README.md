# 树莓派控制 STC89C52RC 三路电磁铁

本文件夹可以直接发送给负责树莓派的同学。树莓派作为上位机，通过串口向已经烧录程序的 STC89C52RC 下位机发送命令，控制 M1、M2、M3 各自通电或断电。

## 1. 已确定的通信协议

- 串口参数：`9600 baud、8 data bits、None parity、1 stop bit`，即 9600、8N1。
- 命令使用 ASCII 大写字符，以 CRLF 结束。
- C52 上电后返回 `READY`。
- 开关命令成功返回 `OK`。
- `STATUS` 返回：`M1=OFF M2=ON M3=OFF` 这种格式。

支持的命令：

| 命令 | 作用 |
|---|---|
| `M1 ON` / `M1 OFF` | 电磁铁1通电/断电 |
| `M2 ON` / `M2 OFF` | 电磁铁2通电/断电 |
| `M3 ON` / `M3 OFF` | 电磁铁3通电/断电 |
| `ALL ON` / `ALL OFF` | 三路同时通电/断电 |
| `STATUS` | 查询三路逻辑状态 |

## 2. 推荐硬件连接

推荐使用已经在电脑上验证过的 CH340 USB-TTL。把 CH340 插到树莓派 USB 口：

| CH340 | STC89C52RC 开发板 |
|---|---|
| TXD | P3.0 / RXD |
| RXD | P3.1 / TXD |
| GND | 开发板 GND |
| VCC / 5V | 不连接 |

其他电源与 MOS 接线保持原来已经验证成功的方案：

- 独立稳压5V给 C52 开发板供电。
- 独立12V给 MOS 功率侧和三只电磁铁供电。
- 不要把12V接到开发板、树莓派或 CH340 的任何5V/3.3V/GPIO引脚。
- 不要把12V的 DC- 接到开发板 GND，继续保持原方案的光耦隔离。

### 禁止直接连接树莓派 GPIO 串口

树莓派 GPIO 是 3.3V，C52 开发板串口是 5V。不能直接把 C52 的 P3.1/TXD 接到树莓派 GPIO RXD，否则可能损坏树莓派。

如果必须使用树莓派 GPIO14/GPIO15 串口，应增加可靠的 3.3V/5V 电平转换模块，并另行配置 `/dev/serial0`。本项目默认不使用这种连接。

## 3. 文件说明

| 文件 | 用途 |
|---|---|
| `magnet_controller.py` | 可导入其他项目的三路控制类 |
| `magnet_cli.py` | 树莓派终端调试工具 |
| `integration_example.py` | 主程序集成示例，运行会实际驱动三路 |
| `requirements.txt` | Python依赖 |
| `test_magnet_controller.py` | 不接硬件也能运行的协议测试 |

## 4. 把文件夹放到树莓派

例如放到：

```bash
/home/pi/magnet-control
```

进入目录：

```bash
cd /home/pi/magnet-control
```

如果树莓派用户名不是 `pi`，路径按实际用户名修改。

## 5. 安装 Python 依赖

推荐使用虚拟环境：

```bash
sudo apt update
sudo apt install -y python3-venv
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
```

以后每次打开新终端，先运行：

```bash
cd /home/pi/magnet-control
source .venv/bin/activate
```

## 6. 检查 CH340 串口

插入 CH340 后运行：

```bash
python magnet_cli.py ports
```

正常情况下会看到：

```text
/dev/ttyUSB0  <- 可能是 CH340
```

也可以运行：

```bash
ls -l /dev/ttyUSB*
```

如果提示没有权限，执行：

```bash
sudo usermod -aG dialout $USER
```

然后注销并重新登录树莓派，或重启一次。不要长期使用 `sudo python` 运行控制程序。

## 7. 交互调试

先只接5V和串口，确认通信正常；再按原测试流程接12V。

自动识别 CH340：

```bash
python magnet_cli.py --port auto
```

如果需要明确指定设备：

```bash
python magnet_cli.py --port /dev/ttyUSB0
```

交互按键：

```text
1       切换 M1
2       切换 M2
3       切换 M3
a       全部通电
x       全部断电
s       查询状态
q       全部断电并退出
```

## 8. 单条命令调试

```bash
python magnet_cli.py --port /dev/ttyUSB0 status
python magnet_cli.py --port /dev/ttyUSB0 m1 on
python magnet_cli.py --port /dev/ttyUSB0 m1 off
python magnet_cli.py --port /dev/ttyUSB0 m2 on
python magnet_cli.py --port /dev/ttyUSB0 m2 off
python magnet_cli.py --port /dev/ttyUSB0 m3 toggle
python magnet_cli.py --port /dev/ttyUSB0 all on
python magnet_cli.py --port /dev/ttyUSB0 all off
```

单条 `ON` 命令执行完后会保留 C52 当前状态，因此调试结束必须再执行 `all off`。

## 9. 集成到树莓派主程序

最小用法：

```python
from magnet_controller import MagnetController

controller = MagnetController(port="auto")
controller.connect()

try:
    controller.set_magnet(1, True)   # M1 通电
    controller.set_magnet(2, False)  # M2 断电
    controller.set_magnet(3, True)   # M3 通电

    state = controller.get_status()
    print(state.m1, state.m2, state.m3)
finally:
    controller.close(turn_all_off=True)
```

可用接口：

```python
controller.set_magnet(1, True)
controller.set_magnet(1, False)
controller.toggle(2)
controller.all_on()
controller.all_off()
controller.get_status()
controller.close(turn_all_off=True)
```

每个控制接口都会等待 C52 回复，并再次读取 `STATUS`，返回 `MagnetState`。因此树莓派程序拿到的是 C52 报告的逻辑状态，不只是“已经发出命令”。

## 10. 不接硬件运行测试

```bash
python -m unittest -v test_magnet_controller.py
```

测试使用模拟串口，不会让真实电磁铁动作。

## 11. 建议的实物验收顺序

1. 保持12V关闭，只给 C52 上5V并连接 CH340。
2. 运行 `python magnet_cli.py --port auto status`，应显示三路状态。
3. 进入交互模式，切换 M1/M2/M3，检查 MOS 输入指示灯是否分别动作。
4. 确认退出后 `STATUS` 为三路 `OFF`。
5. 关闭全部电源后接入12V和电磁铁。
6. 重新上电，每路只通电1到2秒进行测试。
7. 最后测试短时间 `ALL ON`，检查电源压降、MOS和线圈温升。
8. 测试结束执行 `all off`，再关闭12V。

## 12. 重要安全说明

- C52 在串口断开后会保持断开前的输出状态。USB意外拔出、树莓派死机或程序被强制杀死时，不能保证自动 `ALL OFF`。
- 实际比赛设备应保留能够人工切断12V的总开关或急停，不要只依赖软件断电。
- 主程序应使用 `try/finally`，在 `finally` 中调用 `controller.close(turn_all_off=True)`。
- 不要让两个程序同时打开同一个 `/dev/ttyUSB0`，例如命令行工具和正式控制程序不能同时运行。
- 电磁铁不需要吸合时应及时断电，避免长时间发热。

## 13. 常见故障

### 找不到 CH340

- 检查 USB 线和 CH340。
- 运行 `python magnet_cli.py ports`。
- 运行 `dmesg | tail -n 30` 查看 USB 识别信息。

### Permission denied

- 把当前用户加入 `dialout` 组，然后重新登录：

```bash
sudo usermod -aG dialout $USER
```

### Device or resource busy

- 同一个串口已经被其他程序占用。关闭 `minicom`、`screen`、串口调试程序或另一个控制进程。

### 能打开串口但 STATUS 超时

- 检查 C52 是否上电。
- 确认 CH340 TXD 接 P3.0/RXD，CH340 RXD 接 P3.1/TXD。
- 确认两边 GND 已连接。
- 确认 CH340 VCC 没有接到开发板。
- 确认 C52 中仍然是本项目的 9600 波特率固件。
