#!/usr/bin/env python3
"""树莓派通过串口控制 STC89C52RC 三路电磁铁。"""

import re
import threading
import time
from dataclasses import dataclass
from typing import Dict, List, Optional

import serial
from serial.tools import list_ports


BAUD_RATE = 9600
STATUS_PATTERN = re.compile(r"^M1=(ON|OFF) M2=(ON|OFF) M3=(ON|OFF)$")
CH340_VID = 0x1A86


class MagnetError(Exception):
    """电磁铁控制基础异常。"""


class MagnetConnectionError(MagnetError):
    """串口连接异常。"""


class MagnetProtocolError(MagnetError):
    """下位机回复不符合协议。"""


class MagnetTimeoutError(MagnetError):
    """等待下位机回复超时。"""


@dataclass(frozen=True)
class MagnetState:
    """三个电磁铁的逻辑状态，True 表示通电。"""

    m1: bool
    m2: bool
    m3: bool

    def get(self, channel: int) -> bool:
        if channel == 1:
            return self.m1
        if channel == 2:
            return self.m2
        if channel == 3:
            return self.m3
        raise ValueError("channel 必须是 1、2 或 3")

    def as_dict(self) -> Dict[str, bool]:
        return {"M1": self.m1, "M2": self.m2, "M3": self.m3}

    def __str__(self) -> str:
        return "M1={0} M2={1} M3={2}".format(
            "ON" if self.m1 else "OFF",
            "ON" if self.m2 else "OFF",
            "ON" if self.m3 else "OFF",
        )


def list_serial_ports() -> List[str]:
    """返回树莓派当前可见的串口设备。"""
    return sorted(port.device for port in list_ports.comports())


def find_ch340_ports() -> List[str]:
    """查找常见 CH340/USB-SERIAL 设备。"""
    matches = []
    for port in list_ports.comports():
        description = "{0} {1} {2}".format(
            port.description or "",
            port.manufacturer or "",
            port.product or "",
        ).lower()
        if (
            port.vid == CH340_VID
            or "ch340" in description
            or "ch341" in description
            or "usb-serial" in description
            or "qin heng" in description
        ):
            matches.append(port.device)
    return sorted(set(matches))


def auto_detect_port() -> str:
    """自动选择唯一的 CH340；有多个设备时要求调用方明确指定。"""
    matches = find_ch340_ports()
    if len(matches) == 1:
        return matches[0]
    if len(matches) > 1:
        raise MagnetConnectionError(
            "发现多个 CH340 串口：{0}，请使用 --port 明确指定。".format(
                ", ".join(matches)
            )
        )

    available = list_serial_ports()
    suffix = "当前串口：{0}".format(", ".join(available)) if available else "当前没有串口"
    raise MagnetConnectionError("没有检测到 CH340。{0}".format(suffix))


def parse_status(line: str) -> MagnetState:
    """解析 C52 的 STATUS 回复。"""
    match = STATUS_PATTERN.fullmatch(line.strip())
    if match is None:
        raise MagnetProtocolError("无法识别下位机状态：{0}".format(line))
    return MagnetState(
        m1=match.group(1) == "ON",
        m2=match.group(2) == "ON",
        m3=match.group(3) == "ON",
    )


class MagnetController:
    """STC89C52RC 三路电磁铁串口控制器。"""

    def __init__(self, port: str = "auto", timeout: float = 1.2) -> None:
        self.port_name = port
        self.timeout = timeout
        self._serial: Optional[serial.Serial] = None
        self._lock = threading.Lock()

    @property
    def is_connected(self) -> bool:
        return self._serial is not None and self._serial.is_open

    @property
    def active_port(self) -> Optional[str]:
        if self._serial is None:
            return None
        return self._serial.port

    def connect(self) -> MagnetState:
        """打开串口，并用 STATUS 验证下位机通信。"""
        if self.is_connected:
            return self.get_status()

        selected_port = auto_detect_port() if self.port_name == "auto" else self.port_name
        try:
            self._serial = serial.Serial(
                port=selected_port,
                baudrate=BAUD_RATE,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.1,
                write_timeout=0.5,
                xonxoff=False,
                rtscts=False,
                dsrdtr=False,
            )
            try:
                self._serial.dtr = False
                self._serial.rts = False
            except (OSError, serial.SerialException):
                pass
            self._serial.reset_input_buffer()
        except (OSError, serial.SerialException) as exc:
            self._serial = None
            raise MagnetConnectionError(
                "无法打开串口 {0}：{1}".format(selected_port, exc)
            ) from exc

        try:
            return self.get_status()
        except MagnetError:
            self.close(turn_all_off=False)
            raise

    def close(self, turn_all_off: bool = False) -> None:
        """关闭串口；按需在关闭前执行 ALL OFF。"""
        if self.is_connected and turn_all_off:
            try:
                self.all_off()
            except MagnetError:
                pass

        if self._serial is not None:
            try:
                self._serial.close()
            finally:
                self._serial = None

    def get_status(self) -> MagnetState:
        response = self._send_command("STATUS", expect_status=True)
        return parse_status(response)

    def set_magnet(self, channel: int, enabled: bool) -> MagnetState:
        if channel not in (1, 2, 3):
            raise ValueError("channel 必须是 1、2 或 3")
        command = "M{0} {1}".format(channel, "ON" if enabled else "OFF")
        self._send_command(command, expect_status=False)
        return self.get_status()

    def toggle(self, channel: int) -> MagnetState:
        current = self.get_status()
        return self.set_magnet(channel, not current.get(channel))

    def all_on(self) -> MagnetState:
        self._send_command("ALL ON", expect_status=False)
        return self.get_status()

    def all_off(self) -> MagnetState:
        self._send_command("ALL OFF", expect_status=False)
        return self.get_status()

    def _send_command(self, command: str, expect_status: bool) -> str:
        if not self.is_connected or self._serial is None:
            raise MagnetConnectionError("串口尚未连接，请先调用 connect()")

        with self._lock:
            try:
                self._serial.reset_input_buffer()
                self._serial.write((command + "\r\n").encode("ascii"))
                self._serial.flush()
            except (OSError, serial.SerialException) as exc:
                raise MagnetConnectionError("发送命令失败：{0}".format(exc)) from exc

            deadline = time.monotonic() + self.timeout
            while time.monotonic() < deadline:
                try:
                    raw_line = self._serial.readline()
                except (OSError, serial.SerialException) as exc:
                    raise MagnetConnectionError("读取串口失败：{0}".format(exc)) from exc

                if not raw_line:
                    continue
                line = raw_line.decode("ascii", errors="replace").strip()
                if not line or line == "READY":
                    continue
                if line.startswith("ERR"):
                    raise MagnetProtocolError("下位机返回错误：{0}".format(line))
                if not expect_status and line == "OK":
                    return line
                if expect_status and STATUS_PATTERN.fullmatch(line):
                    return line

        raise MagnetTimeoutError(
            "发送 {0} 后，{1:.1f} 秒内没有收到正确回复".format(
                command, self.timeout
            )
        )

    def __enter__(self) -> "MagnetController":
        self.connect()
        return self

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        self.close(turn_all_off=True)
