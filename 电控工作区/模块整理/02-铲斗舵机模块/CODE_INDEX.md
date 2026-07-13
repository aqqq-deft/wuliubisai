# 铲斗舵机代码索引

| 标记 | 文件 | 作用 | 正式源文件位置 |
|---|---|---|---|
| 核心 | `STM32代码/bucket_control.h` | 命令编号、状态编号和公开函数 | `User/app/inc/bucket_control.h` |
| 核心 | `STM32代码/bucket_control.c` | 平滑运动、限幅、急停锁存和状态反馈 | `User/app/src/bucket_control.c` |
| 配置 | `STM32代码/competition_config.h` | 三个位置脉宽、1000～2000μs限制、使能开关 | `User/app/inc/competition_config.h` |
| 底层 | `STM32代码/bsp_servo.c` | 将脉宽写入TIM8/TIM1比较寄存器 | `User/bsp/src/bsp_servo.c` |
| 共享 | `STM32代码/bsp_timer.c` | 初始化TIM8舵机PWM，也初始化电机PWM | `User/bsp/src/bsp_timer.c` |
| 共享 | `STM32代码/autocontrol.c` | 将串口命令码0x07交给铲斗模块 | `User/app/src/autocontrol.c` |
| 共享 | `STM32代码/main.c` | 周期更新、状态发送和断连急停 | `User/app/src/main.c` |
| 共享 | `STM32代码/includes.h` | 将铲斗模块加入应用公共头文件 | `User/app/inc/includes.h` |
| 共享 | `ROS2代码/codbot_base.cpp` | `/bucket_command`封包和`/bucket_state`发布 | `src/codbot_base.cpp` |
| 共享 | `ROS2代码/codbot_base.h` | 铲斗订阅器、发布器和串口缓存声明 | `include/codbot_base/codbot_base.h` |
| 配置 | `ROS2代码/codbot_base.launch.py` | 铲斗话题名称 | `launch/codbot_base.launch.py` |

## 命令值

- 0：停止；1：开始挖豆；2：抬到运输位。
- 3：开始卸豆；4：卸豆后回运输位；5：复位/解除急停。
- 255：急停并锁存，之后必须发送5复位。

## 状态值

- 0停止；1前往挖豆位；2挖豆就绪；3前往运输位；4运输就绪。
- 5前往卸豆位；6卸豆就绪；255急停锁定。


注意：本文件夹是阅读副本，不能单独编译。正式修改必须回到原STM32和ROS2工程。
