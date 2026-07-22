#!/usr/bin/env python3

import unittest
from types import SimpleNamespace
from unittest.mock import patch

import magnet_controller


class FakeSerial:
    def __init__(self, port, **kwargs):
        self.port = port
        self.is_open = True
        self.dtr = False
        self.rts = False
        self._states = [False, False, False]
        self._replies = []

    def reset_input_buffer(self):
        self._replies = []

    def write(self, data):
        command = data.decode("ascii").strip()
        if command == "STATUS":
            self._replies.append(
                "M1={0} M2={1} M3={2}\r\n".format(
                    "ON" if self._states[0] else "OFF",
                    "ON" if self._states[1] else "OFF",
                    "ON" if self._states[2] else "OFF",
                ).encode("ascii")
            )
        elif command == "ALL ON":
            self._states = [True, True, True]
            self._replies.append(b"OK\r\n")
        elif command == "ALL OFF":
            self._states = [False, False, False]
            self._replies.append(b"OK\r\n")
        elif command.startswith("M"):
            channel = int(command[1]) - 1
            self._states[channel] = command.endswith(" ON")
            self._replies.append(b"OK\r\n")
        else:
            self._replies.append(b"ERR UNKNOWN\r\n")

    def flush(self):
        pass

    def readline(self):
        if self._replies:
            return self._replies.pop(0)
        return b""

    def close(self):
        self.is_open = False


class MagnetControllerTests(unittest.TestCase):
    def setUp(self):
        serial_patcher = patch.object(magnet_controller.serial, "Serial", FakeSerial)
        self.addCleanup(serial_patcher.stop)
        serial_patcher.start()
        self.controller = magnet_controller.MagnetController("/dev/ttyUSB0", timeout=0.02)
        self.controller.connect()

    def tearDown(self):
        self.controller.close()

    def test_independent_channels(self):
        state = self.controller.set_magnet(2, True)
        self.assertFalse(state.m1)
        self.assertTrue(state.m2)
        self.assertFalse(state.m3)

    def test_toggle(self):
        self.assertTrue(self.controller.toggle(1).m1)
        self.assertFalse(self.controller.toggle(1).m1)

    def test_all_on_and_all_off(self):
        self.assertEqual(
            self.controller.all_on().as_dict(),
            {"M1": True, "M2": True, "M3": True},
        )
        self.assertEqual(
            self.controller.all_off().as_dict(),
            {"M1": False, "M2": False, "M3": False},
        )

    def test_invalid_channel(self):
        with self.assertRaises(ValueError):
            self.controller.set_magnet(4, True)


class AutoDetectTests(unittest.TestCase):
    def test_detects_ch340_vid(self):
        fake_port = SimpleNamespace(
            device="/dev/ttyUSB0",
            vid=0x1A86,
            description="USB Serial",
            manufacturer="QinHeng",
            product="CH340",
        )
        with patch.object(magnet_controller.list_ports, "comports", return_value=[fake_port]):
            self.assertEqual(magnet_controller.auto_detect_port(), "/dev/ttyUSB0")


if __name__ == "__main__":
    unittest.main()
