import json
import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BOARD_DIR = ROOT / "main" / "boards" / "chatchat" / "hi-yaka-v3"


class HiYakaBoardContractTest(unittest.TestCase):
    def test_board_identity_and_no_psram_policy(self):
        config = json.loads((BOARD_DIR / "config.json").read_text(encoding="utf-8"))
        self.assertEqual(config["manufacturer"], "chatchat")
        self.assertEqual(config["type"], "hi-yaka-v3")
        self.assertEqual(config["target"], "esp32c6")
        self.assertEqual(len(config["builds"]), 1)
        build = config["builds"][0]
        self.assertEqual(build["name"], "hi-yaka-v3")
        options = set(build["sdkconfig_append"])
        self.assertIn("CONFIG_SPIRAM=n", options)
        self.assertIn("CONFIG_ESPTOOLPY_FLASHMODE_DIO=y", options)
        self.assertIn("CONFIG_WAKE_WORD_DISABLED=y", options)
        self.assertNotIn("CONFIG_USE_DEVICE_AEC=y", options)

    def test_build_selection_chain_is_complete(self):
        kconfig = (ROOT / "main" / "Kconfig.projbuild").read_text(encoding="utf-8")
        cmake = (ROOT / "main" / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertRegex(kconfig, r"config BOARD_TYPE_HI_YAKA_V3\s+bool")
        self.assertIn("depends on IDF_TARGET_ESP32C6", kconfig)
        self.assertIn("CONFIG_BOARD_TYPE_HI_YAKA_V3", cmake)
        self.assertIn('set(BOARD_DIR "chatchat/hi-yaka-v3")', cmake)

    def test_hardware_contract_constants(self):
        header = (BOARD_DIR / "config.h").read_text(encoding="utf-8")
        expected = {
            "YAKA_I2C_SDA_GPIO": "GPIO_NUM_10",
            "YAKA_I2C_SCL_GPIO": "GPIO_NUM_11",
            "YAKA_PCA9557_I2C_ADDRESS": "0x19",
            "YAKA_PCA9557_SAFE_OUTPUT": "0x40",
            "YAKA_PCA9557_SAFE_CONFIGURATION": "0x38",
        }
        for name, value in expected.items():
            self.assertRegex(header, rf"#define\s+{name}\s+{re.escape(value)}")

    def test_pca9557_safe_write_precedes_direction_change(self):
        source = (BOARD_DIR / "hi_yaka_v3_board.cc").read_text(encoding="utf-8")
        output_write = source.index(
            "WritePca9557Register(kPcaOutputPortRegister, YAKA_PCA9557_SAFE_OUTPUT)"
        )
        direction_write = source.index(
            "WritePca9557Register(kPcaConfigurationRegister,"
        )
        self.assertLess(output_write, direction_write)
        self.assertEqual(source.count("DECLARE_BOARD("), 1)

    def test_m1_does_not_enable_unvalidated_peripherals(self):
        source = (BOARD_DIR / "hi_yaka_v3_board.cc").read_text(encoding="utf-8")
        self.assertIn("DummyAudioCodec", source)
        self.assertNotRegex(source, r"Es7210|Es8311AudioCodec|esp_lcd_new_panel|esp_vfs_fat")


if __name__ == "__main__":
    unittest.main()
