# 四轮底盘代码索引

| 标记 | 文件 | 作用 | 正式源文件位置 |
|---|---|---|---|
| 核心 | `STM32代码/motor.c` | 麦轮运动分解、四轮目标速度和PID调用 | `User/app/src/motor.c` |
| 核心 | `STM32代码/pid.c` | 四路轮速PID计算 | `User/app/src/pid.c` |
| 底层 | `STM32代码/bsp_motor.c` | 四路PWM、方向引脚和电机使能 | `User/bsp/src/bsp_motor.c` |
| 底层 | `STM32代码/bsp_encoder.c` | 四路编码器计数与方向读取 | `User/bsp/src/bsp_encoder.c` |
| 共享 | `STM32代码/bsp_timer.c` | 电机PWM、舵机PWM等定时器初始化 | `User/bsp/src/bsp_timer.c` |
| 共享 | `STM32代码/autocontrol.c` | 解析底盘命令码0x01，也解析铲斗命令码0x07 | `User/app/src/autocontrol.c` |
| 共享 | `STM32代码/main.c` | FreeRTOS控制任务、500ms断连停车 | `User/app/src/main.c` |
| 配置 | `STM32代码/competition_config.h` | 30% PWM限制、控制周期和超时 | `User/app/inc/competition_config.h` |
| 核心 | `ROS2代码/codbot_base.cpp` | `/cmd_vel`封包、串口读写、里程计 | `src/codbot_base.cpp` |
| 共享 | `ROS2代码/codbot_base.h` | 底盘和铲斗ROS2接口声明 | `include/codbot_base/codbot_base.h` |
| 配置 | `ROS2代码/codbot_base.launch.py` | M100类型、速度限制和话题参数 | `launch/codbot_base.launch.py` |

## 四轮硬件对应

- A：PWM PA6；方向PC1/PC0；编码器PA0/PA4。
- B：PWM PA7；方向PC2/PC3；编码器PA1/PA5。
- C：PWM PB0；方向PB8/PB9；编码器PA2/PC4。
- D：PWM PB1；方向PC11/PC10；编码器PA3/PA15。

注意：本文件夹是阅读副本，不能单独编译。正式工程入口仍是原Keil工程和ROS2工作区。
