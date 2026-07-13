# 物流比赛电控工作区

这是从厂家 M100 麦克纳姆轮工程和 ROS2 驱动复制出的独立工作副本，原始资料没有修改。

## 当前完成状态

- 已确认 Keil 工程芯片为 STM32F103ZE（Cortex-M3）。
- 已确认 ROS2 驱动支持 `current_type=M100`，订阅 `/cmd_vel` 的 `linear.x`、`linear.y`、`angular.z`。
- STM32 上电后立即执行停车。
- 禁止蓝牙命令抢占 USB/ROS2 串口控制。
- 连续 500 ms 没有收到有效控制帧时自动停车。
- 首次架空测试的 PWM 上限为 300/1000（30%）。
- 已预留搬运机构命令和状态定义，但在机械参数确认前不连接实际引脚。

## 从这里开始

1. 阅读 `docs/01-首次上电前检查.md`，在检查表完成前不要接电。
2. 将要求的实物照片补充给 Codex，确认电源、极性和下载器。
3. 用 Keil 打开 `firmware/codbot-M100-2022.08.19/Project/MDK-ARM/project.uvprojx`。
4. 先只编译，不下载；保存完整 Build Output。
5. 编译通过并确认下载器后，才进入 LED、串口和架空单轮测试。

## 目录

- `firmware/`：STM32 比赛固件工作副本。
- `ros2_ws/src/codbot_base/`：ROS2 Humble 底盘驱动工作副本。
- `docs/`：小白操作清单、接口和验收表。