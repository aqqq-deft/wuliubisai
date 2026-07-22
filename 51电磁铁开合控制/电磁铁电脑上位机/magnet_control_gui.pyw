# -*- coding: utf-8 -*-
"""STC89C52RC 三路电磁铁串口上位机。"""

from __future__ import annotations

import re
import time
import tkinter as tk
from tkinter import messagebox, ttk

import serial
from serial.tools import list_ports


BAUD_RATE = 9600
RESPONSE_TIMEOUT_SECONDS = 1.2
STATUS_PATTERN = re.compile(r"^M1=(ON|OFF) M2=(ON|OFF) M3=(ON|OFF)$")


def parse_status(line: str) -> dict[int, bool] | None:
    """把下位机 STATUS 回复转换为三个通道的布尔状态。"""
    match = STATUS_PATTERN.fullmatch(line.strip())
    if match is None:
        return None
    return {
        1: match.group(1) == "ON",
        2: match.group(2) == "ON",
        3: match.group(3) == "ON",
    }


class MagnetControlApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.serial_port: serial.Serial | None = None
        self.port_map: dict[str, str] = {}
        self.magnet_states: dict[int, bool | None] = {1: None, 2: None, 3: None}

        self.port_var = tk.StringVar()
        self.connection_var = tk.StringVar(value="未连接")
        self.summary_var = tk.StringVar(value="请先选择 CH340 串口并连接")
        self.magnet_buttons: dict[int, tk.Button] = {}
        self.control_buttons: list[tk.Widget] = []

        self._configure_window()
        self._build_interface()
        self.refresh_ports()
        self.root.protocol("WM_DELETE_WINDOW", self.close_app)

    def _configure_window(self) -> None:
        self.root.title("三路电磁铁控制上位机")
        self.root.geometry("720x610")
        self.root.minsize(680, 570)
        self.root.configure(bg="#f3f5f7")

        style = ttk.Style()
        try:
            style.theme_use("vista")
        except tk.TclError:
            pass
        style.configure("Title.TLabel", font=("Microsoft YaHei UI", 20, "bold"))
        style.configure("Section.TLabelframe.Label", font=("Microsoft YaHei UI", 11, "bold"))

    def _build_interface(self) -> None:
        main = ttk.Frame(self.root, padding=18)
        main.pack(fill="both", expand=True)

        ttk.Label(main, text="STC89C52RC 三路电磁铁控制", style="Title.TLabel").pack(pady=(0, 14))

        connection_frame = ttk.LabelFrame(main, text="串口连接", style="Section.TLabelframe", padding=12)
        connection_frame.pack(fill="x")

        ttk.Label(connection_frame, text="串口：").grid(row=0, column=0, padx=(0, 6), pady=3)
        self.port_combo = ttk.Combobox(
            connection_frame,
            textvariable=self.port_var,
            state="readonly",
            width=34,
        )
        self.port_combo.grid(row=0, column=1, padx=4, pady=3, sticky="ew")

        self.refresh_button = ttk.Button(connection_frame, text="刷新", command=self.refresh_ports, width=9)
        self.refresh_button.grid(row=0, column=2, padx=4, pady=3)

        self.connect_button = ttk.Button(connection_frame, text="连接", command=self.toggle_connection, width=11)
        self.connect_button.grid(row=0, column=3, padx=(4, 0), pady=3)

        ttk.Label(connection_frame, text="通信参数：9600 bps，8N1").grid(
            row=1, column=0, columnspan=2, sticky="w", pady=(8, 0)
        )
        self.connection_label = ttk.Label(connection_frame, textvariable=self.connection_var, foreground="#b42318")
        self.connection_label.grid(row=1, column=2, columnspan=2, sticky="e", pady=(8, 0))
        connection_frame.columnconfigure(1, weight=1)

        magnet_frame = ttk.LabelFrame(main, text="独立控制", style="Section.TLabelframe", padding=12)
        magnet_frame.pack(fill="x", pady=(14, 0))

        for channel in (1, 2, 3):
            button = tk.Button(
                magnet_frame,
                text=f"M{channel}\n状态未知",
                command=lambda selected=channel: self.toggle_magnet(selected),
                width=16,
                height=4,
                font=("Microsoft YaHei UI", 12, "bold"),
                bg="#d0d5dd",
                activebackground="#d0d5dd",
                relief="raised",
                borderwidth=2,
                state="disabled",
            )
            button.grid(row=0, column=channel - 1, padx=8, pady=4, sticky="ew")
            magnet_frame.columnconfigure(channel - 1, weight=1)
            self.magnet_buttons[channel] = button

        all_frame = ttk.Frame(main)
        all_frame.pack(fill="x", pady=14)

        all_on_button = tk.Button(
            all_frame,
            text="全部通电",
            command=lambda: self.set_all(True),
            font=("Microsoft YaHei UI", 11, "bold"),
            bg="#fdb022",
            activebackground="#f79009",
            state="disabled",
            height=2,
        )
        all_on_button.pack(side="left", fill="x", expand=True, padx=(0, 6))

        all_off_button = tk.Button(
            all_frame,
            text="全部断电",
            command=lambda: self.set_all(False),
            font=("Microsoft YaHei UI", 11, "bold"),
            bg="#f04438",
            fg="white",
            activebackground="#d92d20",
            activeforeground="white",
            state="disabled",
            height=2,
        )
        all_off_button.pack(side="left", fill="x", expand=True, padx=6)

        read_status_button = ttk.Button(all_frame, text="读取状态", command=self.query_status, state="disabled")
        read_status_button.pack(side="left", fill="y", padx=(6, 0))

        self.control_buttons.extend([all_on_button, all_off_button, read_status_button])

        summary_frame = ttk.LabelFrame(main, text="当前状态", style="Section.TLabelframe", padding=10)
        summary_frame.pack(fill="x")
        ttk.Label(
            summary_frame,
            textvariable=self.summary_var,
            font=("Microsoft YaHei UI", 11),
        ).pack(anchor="w")

        log_frame = ttk.LabelFrame(main, text="通信记录", style="Section.TLabelframe", padding=8)
        log_frame.pack(fill="both", expand=True, pady=(14, 0))
        self.log_text = tk.Text(
            log_frame,
            height=9,
            state="disabled",
            wrap="word",
            font=("Consolas", 10),
            bg="#ffffff",
        )
        scrollbar = ttk.Scrollbar(log_frame, orient="vertical", command=self.log_text.yview)
        self.log_text.configure(yscrollcommand=scrollbar.set)
        self.log_text.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")

        ttk.Label(
            main,
            text="安全提示：关闭窗口或主动断开时，软件会先发送 ALL OFF。意外拔线时请手动切断12V。",
            foreground="#b54708",
            font=("Microsoft YaHei UI", 9),
        ).pack(anchor="w", pady=(10, 0))

    def refresh_ports(self) -> None:
        current_port = self._selected_port_name()
        ports = sorted(list_ports.comports(), key=lambda item: item.device)
        self.port_map.clear()

        display_values: list[str] = []
        for port in ports:
            display = f"{port.device} — {port.description}"
            self.port_map[display] = port.device
            display_values.append(display)

        self.port_combo["values"] = display_values
        if not display_values:
            self.port_var.set("")
            self._append_log("没有发现串口，请检查 CH340 是否已连接。")
            return

        preferred_display = next(
            (display for display, device in self.port_map.items() if device == current_port),
            None,
        )
        if preferred_display is None:
            preferred_display = next(
                (display for display, device in self.port_map.items() if device.upper() == "COM5"),
                display_values[0],
            )
        self.port_var.set(preferred_display)

    def _selected_port_name(self) -> str | None:
        selected = self.port_var.get()
        if selected in self.port_map:
            return self.port_map[selected]
        if selected.upper().startswith("COM"):
            return selected.split()[0]
        return None

    def toggle_connection(self) -> None:
        if self._is_connected():
            self.disconnect(send_all_off=True)
        else:
            self.connect()

    def connect(self) -> None:
        port_name = self._selected_port_name()
        if port_name is None:
            messagebox.showwarning("未选择串口", "请连接 CH340，点击“刷新”，然后选择对应的 COM 口。")
            return

        try:
            self.serial_port = serial.Serial(
                port=port_name,
                baudrate=BAUD_RATE,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.12,
                write_timeout=0.5,
            )
            try:
                self.serial_port.dtr = False
                self.serial_port.rts = False
            except (OSError, serial.SerialException):
                pass
            self.serial_port.reset_input_buffer()
        except (OSError, serial.SerialException) as exc:
            self.serial_port = None
            messagebox.showerror(
                "串口打开失败",
                f"无法打开 {port_name}。\n\n请先关闭 SSCOM 和 STC-ISP 的串口，再重试。\n\n错误：{exc}",
            )
            return

        self.connection_var.set(f"已连接：{port_name}")
        self.connection_label.configure(foreground="#067647")
        self.connect_button.configure(text="断开")
        self.port_combo.configure(state="disabled")
        self.refresh_button.configure(state="disabled")
        self._set_controls_enabled(True)
        self._append_log(f"已连接 {port_name}，波特率 {BAUD_RATE}。")

        if not self.query_status(show_error=False):
            messagebox.showerror(
                "下位机无响应",
                "串口已经打开，但没有收到正确的 STATUS 回复。\n\n请检查 TXD/RXD 是否交叉连接、开发板是否供电，以及波特率是否为 9600。",
            )
            self.disconnect(send_all_off=False)

    def disconnect(self, send_all_off: bool = True) -> None:
        if self._is_connected() and send_all_off:
            try:
                self._send_and_wait("ALL OFF", expected="OK", timeout=0.6)
                self._append_log("断开前已发送 ALL OFF。")
            except (OSError, serial.SerialException, TimeoutError):
                self._append_log("警告：断开前未确认 ALL OFF，请检查电磁铁并关闭12V。")

        if self.serial_port is not None:
            try:
                self.serial_port.close()
            except (OSError, serial.SerialException):
                pass
        self.serial_port = None

        self.connection_var.set("未连接")
        self.connection_label.configure(foreground="#b42318")
        self.connect_button.configure(text="连接")
        self.port_combo.configure(state="readonly")
        self.refresh_button.configure(state="normal")
        self._set_controls_enabled(False)
        self._set_unknown_state()
        self._append_log("串口已断开。")

    def toggle_magnet(self, channel: int) -> None:
        current_state = self.magnet_states[channel]
        if current_state is None:
            if not self.query_status():
                return
            current_state = self.magnet_states[channel]

        target_state = not bool(current_state)
        command = f"M{channel} {'ON' if target_state else 'OFF'}"
        if self._execute_control_command(command):
            self.query_status(show_error=False)

    def set_all(self, enabled: bool) -> None:
        command = "ALL ON" if enabled else "ALL OFF"
        if self._execute_control_command(command):
            self.query_status(show_error=False)

    def _execute_control_command(self, command: str) -> bool:
        try:
            self._send_and_wait(command, expected="OK")
            return True
        except TimeoutError as exc:
            messagebox.showerror("下位机未确认", str(exc))
        except (OSError, serial.SerialException) as exc:
            self._handle_connection_error(exc)
        return False

    def query_status(self, show_error: bool = True) -> bool:
        try:
            response = self._send_and_wait("STATUS", expected="STATUS")
            states = parse_status(response)
            if states is None:
                raise TimeoutError(f"收到无法识别的状态：{response}")
            self.magnet_states.update(states)
            self._update_magnet_buttons()
            return True
        except TimeoutError as exc:
            self._set_unknown_state()
            if show_error:
                messagebox.showerror("状态读取失败", str(exc))
        except (OSError, serial.SerialException) as exc:
            self._handle_connection_error(exc)
        return False

    def _send_and_wait(self, command: str, expected: str, timeout: float = RESPONSE_TIMEOUT_SECONDS) -> str:
        if not self._is_connected() or self.serial_port is None:
            raise serial.SerialException("串口未连接")

        self.serial_port.reset_input_buffer()
        payload = (command + "\r\n").encode("ascii")
        self.serial_port.write(payload)
        self.serial_port.flush()
        self._append_log(f"> {command}")

        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            raw_line = self.serial_port.readline()
            if not raw_line:
                continue
            line = raw_line.decode("ascii", errors="replace").strip()
            if not line:
                continue
            self._append_log(f"< {line}")

            if line.startswith("ERR"):
                raise TimeoutError(f"下位机返回错误：{line}")
            if expected == "OK" and line == "OK":
                return line
            if expected == "STATUS" and parse_status(line) is not None:
                return line

        raise TimeoutError(f"发送 {command} 后，未在 {timeout:.1f} 秒内收到正确回复。")

    def _update_magnet_buttons(self) -> None:
        status_parts: list[str] = []
        for channel, button in self.magnet_buttons.items():
            enabled = self.magnet_states[channel]
            if enabled:
                button.configure(
                    text=f"M{channel}\n已通电\n点击断电",
                    bg="#12b76a",
                    fg="white",
                    activebackground="#039855",
                    activeforeground="white",
                )
                status_parts.append(f"M{channel}=通电")
            else:
                button.configure(
                    text=f"M{channel}\n已断电\n点击通电",
                    bg="#d0d5dd",
                    fg="#101828",
                    activebackground="#98a2b3",
                    activeforeground="#101828",
                )
                status_parts.append(f"M{channel}=断电")
        self.summary_var.set("    ".join(status_parts))

    def _set_unknown_state(self) -> None:
        for channel, button in self.magnet_buttons.items():
            self.magnet_states[channel] = None
            button.configure(
                text=f"M{channel}\n状态未知",
                bg="#d0d5dd",
                fg="#101828",
                activebackground="#d0d5dd",
            )
        self.summary_var.set("状态未知；请连接并读取状态")

    def _set_controls_enabled(self, enabled: bool) -> None:
        state = "normal" if enabled else "disabled"
        for button in self.magnet_buttons.values():
            button.configure(state=state)
        for widget in self.control_buttons:
            widget.configure(state=state)

    def _handle_connection_error(self, exc: BaseException) -> None:
        messagebox.showerror(
            "串口通信中断",
            f"与下位机的通信已中断。\n\n请立即确认电磁铁状态，必要时关闭12V。\n\n错误：{exc}",
        )
        self.disconnect(send_all_off=False)

    def _is_connected(self) -> bool:
        return self.serial_port is not None and self.serial_port.is_open

    def _append_log(self, message: str) -> None:
        timestamp = time.strftime("%H:%M:%S")
        self.log_text.configure(state="normal")
        self.log_text.insert("end", f"[{timestamp}] {message}\n")
        self.log_text.see("end")
        self.log_text.configure(state="disabled")

    def close_app(self) -> None:
        if self._is_connected():
            self.disconnect(send_all_off=True)
        self.root.destroy()


def main() -> None:
    root = tk.Tk()
    MagnetControlApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
