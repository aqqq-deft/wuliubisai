#!/usr/bin/env python3
"""树莓派三路电磁铁命令行调试工具。"""

import argparse
import sys
from typing import List

from magnet_controller import (
    MagnetController,
    MagnetError,
    find_ch340_ports,
    list_serial_ports,
)


def print_state(state) -> None:
    print("当前状态：{0}".format(state))


def print_ports() -> None:
    ports = list_serial_ports()
    ch340_ports = set(find_ch340_ports())
    if not ports:
        print("没有检测到串口设备。")
        return
    print("检测到的串口：")
    for port in ports:
        suffix = "  <- 可能是 CH340" if port in ch340_ports else ""
        print("  {0}{1}".format(port, suffix))


def run_one_shot(controller: MagnetController, words: List[str]) -> None:
    normalized = [word.lower() for word in words]
    if normalized == ["status"]:
        print_state(controller.get_status())
        return

    if len(normalized) == 2 and normalized[0] in ("m1", "m2", "m3"):
        channel = int(normalized[0][1])
        if normalized[1] == "on":
            print_state(controller.set_magnet(channel, True))
            return
        if normalized[1] == "off":
            print_state(controller.set_magnet(channel, False))
            return
        if normalized[1] == "toggle":
            print_state(controller.toggle(channel))
            return

    if len(normalized) == 2 and normalized[0] == "all":
        if normalized[1] == "on":
            print_state(controller.all_on())
            return
        if normalized[1] == "off":
            print_state(controller.all_off())
            return

    raise ValueError(
        "命令格式错误。示例：status、m1 on、m2 off、m3 toggle、all off"
    )


def interactive_loop(controller: MagnetController) -> None:
    print("\n三路电磁铁交互调试")
    print("1/2/3：切换对应电磁铁")
    print("a：全部通电    x：全部断电")
    print("s：读取状态    q：全部断电并退出\n")
    print_state(controller.get_status())

    while True:
        command = input("请输入命令> ").strip().lower()
        if command in ("1", "2", "3"):
            print_state(controller.toggle(int(command)))
        elif command == "a":
            print_state(controller.all_on())
        elif command == "x":
            print_state(controller.all_off())
        elif command == "s":
            print_state(controller.get_status())
        elif command == "q":
            print_state(controller.all_off())
            print("三路已断电，程序退出。")
            return
        elif command:
            print("无法识别，请输入 1、2、3、a、x、s 或 q。")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="树莓派通过 CH340 控制 STC89C52RC 三路电磁铁"
    )
    parser.add_argument(
        "--port",
        default="auto",
        help="串口设备，默认 auto；也可指定 /dev/ttyUSB0",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=1.2,
        help="等待下位机回复的超时时间，默认 1.2 秒",
    )
    parser.add_argument(
        "command",
        nargs="*",
        help="不填写则进入交互模式；可填写 status、m1 on、all off 等",
    )
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    if [word.lower() for word in args.command] == ["ports"]:
        print_ports()
        return 0

    controller = MagnetController(port=args.port, timeout=args.timeout)
    interactive = len(args.command) == 0

    try:
        initial_state = controller.connect()
        print("已连接：{0}".format(controller.active_port))
        print_state(initial_state)

        if interactive:
            interactive_loop(controller)
        else:
            run_one_shot(controller, args.command)
        return 0
    except (MagnetError, ValueError) as exc:
        print("错误：{0}".format(exc), file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("\n收到 Ctrl+C，准备全部断电并退出。")
        if controller.is_connected:
            try:
                controller.all_off()
            except MagnetError as exc:
                print("警告：ALL OFF 未确认：{0}".format(exc), file=sys.stderr)
        return 130
    finally:
        # 交互模式退出时执行安全断电；单次命令允许状态按命令结果保持。
        controller.close(turn_all_off=interactive)


if __name__ == "__main__":
    raise SystemExit(main())
