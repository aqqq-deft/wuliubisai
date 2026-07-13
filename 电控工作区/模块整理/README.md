# 电控模块整理总览

本目录根据 `D:\物流比赛\交接1.md` 和当前电控工作区整理，用于分别阅读四轮底盘与铲斗舵机代码。

## 两个模块

- `01-四轮底盘模块`：四个麦克纳姆轮、电机驱动、编码器、PID、`/cmd_vel` 和底盘串口帧。
- `02-铲斗舵机模块`：MG996R、TIM8舵机PWM、铲斗状态机、`/bucket_command` 和 `/bucket_state`。

## 重要说明

1. 这里保存的是便于分析的代码副本，不是新的可编译工程。
2. 真正的STM32工程仍在 `..\firmware\codbot-M100-2022.08.19`。
3. 真正的ROS2工程仍在 `..\ros2_ws\src\codbot_base`。
4. 不要只修改本目录中的副本；正式修改必须回到上述源工程。
5. `main.c`、`autocontrol.c`、`bsp_timer.c`、`codbot_base.cpp` 等同时连接两个模块，所以会在两个文件夹中重复出现，并标为“共享集成文件”。
6. 原厂家STM32文件可能使用GBK编码，复制时保持了原始字节，不要随意转换编码。

## 推荐阅读顺序

### 四轮底盘

`README.md` → `CODE_INDEX.md` → `motor.c` → `bsp_motor.c` → `bsp_encoder.c` → `pid.c` → `autocontrol.c` → ROS2 `codbot_base.cpp`

### 铲斗舵机

`README.md` → `CODE_INDEX.md` → `bucket_control.h` → `bucket_control.c` → `bsp_servo.c` → `bsp_timer.c` → `autocontrol.c` → ROS2 `codbot_base.cpp`

## 当前安全状态

- 四轮PWM仍限制为30%。
- 铲斗 `BUCKET_ENABLE_MOTION=0`，未完成实物标定前不会执行未知角度动作。
- 当前整理不代表已完成Keil编译、烧录、ROS2编译或实车测试。
