import importlib.machinery
import importlib.util
import pathlib
import unittest


APP_PATH = pathlib.Path(__file__).with_name("magnet_control_gui.pyw")
LOADER = importlib.machinery.SourceFileLoader("magnet_control_gui", str(APP_PATH))
SPEC = importlib.util.spec_from_loader(LOADER.name, LOADER)
APP = importlib.util.module_from_spec(SPEC)
LOADER.exec_module(APP)


class ParseStatusTests(unittest.TestCase):
    def test_all_off(self):
        self.assertEqual(
            APP.parse_status("M1=OFF M2=OFF M3=OFF"),
            {1: False, 2: False, 3: False},
        )

    def test_independent_states(self):
        self.assertEqual(
            APP.parse_status("M1=ON M2=OFF M3=ON\r\n"),
            {1: True, 2: False, 3: True},
        )

    def test_rejects_invalid_reply(self):
        self.assertIsNone(APP.parse_status("OK"))
        self.assertIsNone(APP.parse_status("M1=ON M2=OFF"))


if __name__ == "__main__":
    unittest.main()
