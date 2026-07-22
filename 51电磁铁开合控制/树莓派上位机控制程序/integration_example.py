#!/usr/bin/env python3
"""供树莓派主程序参考的最小集成示例。

注意：运行本文件会让 M1、M2、M3 依次短暂通电。
"""

import time

from magnet_controller import MagnetController


def main() -> None:
    controller = MagnetController(port="auto")
    controller.connect()
    print("连接成功：{0}".format(controller.active_port))

    try:
        print(controller.set_magnet(1, True))
        time.sleep(1.0)
        print(controller.set_magnet(1, False))

        print(controller.set_magnet(2, True))
        time.sleep(1.0)
        print(controller.set_magnet(2, False))

        print(controller.set_magnet(3, True))
        time.sleep(1.0)
        print(controller.set_magnet(3, False))
    finally:
        # 无论程序正常结束还是发生异常，都尽量让三路断电。
        controller.close(turn_all_off=True)


if __name__ == "__main__":
    main()
