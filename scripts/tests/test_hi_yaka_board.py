import json
import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BOARD_DIR = ROOT / "main" / "boards" / "chatchat" / "hi-yaka"


class HiYaKaBoardContractTest(unittest.TestCase):
    def test_board_identity_and_no_psram_policy(self):
        config = json.loads((BOARD_DIR / "config.json").read_text(encoding="utf-8"))
        self.assertEqual(config["manufacturer"], "chatchat")
        self.assertEqual(config["type"], "hi-yaka")
        self.assertEqual(config["target"], "esp32c6")
        self.assertEqual(len(config["builds"]), 2)
        build = config["builds"][0]
        self.assertEqual(build["name"], "hi-yaka")
        options = set(build["sdkconfig_append"])
        self.assertFalse(any(option.startswith("CONFIG_SPIRAM=") for option in options))
        self.assertIn("CONFIG_ESPTOOLPY_FLASHMODE_DIO=y", options)
        self.assertIn("CONFIG_WAKE_WORD_DISABLED=y", options)
        self.assertNotIn("CONFIG_USE_DEVICE_AEC=y", options)
        self.assertFalse(any(option.startswith("CONFIG_HI_YAKA_ENABLE_") for option in options))

        lab_build = config["builds"][1]
        self.assertEqual(lab_build["name"], "hi-yaka-lab")
        lab_options = set(lab_build["sdkconfig_append"])
        self.assertIn("CONFIG_HI_YAKA_ENABLE_DISPLAY=y", lab_options)
        self.assertIn("CONFIG_HI_YAKA_ENABLE_STORAGE=y", lab_options)
        self.assertIn("CONFIG_HI_YAKA_ENABLE_AUDIO=y", lab_options)
        self.assertIn("CONFIG_HI_YAKA_BRINGUP_DIAGNOSTICS=y", lab_options)
        self.assertNotIn("CONFIG_HI_YAKA_ENABLE_PRODUCT_RUNTIME=y", lab_options)

    def test_build_selection_chain_is_complete(self):
        kconfig = (ROOT / "main" / "Kconfig.projbuild").read_text(encoding="utf-8")
        cmake = (ROOT / "main" / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertRegex(kconfig, r"config BOARD_TYPE_HI_YAKA\s+bool")
        self.assertIn("depends on IDF_TARGET_ESP32C6", kconfig)
        self.assertIn("CONFIG_BOARD_TYPE_HI_YAKA", cmake)
        self.assertIn('set(BOARD_DIR "chatchat/hi-yaka")', cmake)

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
        source = (BOARD_DIR / "hi_yaka_pca9557.cc").read_text(encoding="utf-8")
        output_write = source.index("WriteRegisterLocked(kOutputPortRegister, output_shadow_)")
        direction_write = source.index(
            "WriteRegisterLocked(kConfigurationRegister, YAKA_PCA9557_SAFE_CONFIGURATION)"
        )
        self.assertLess(output_write, direction_write)
        board = (BOARD_DIR / "hi_yaka_board.cc").read_text(encoding="utf-8")
        self.assertIn("pca9557_->InitializeSafe()", board)
        self.assertEqual(board.count("DECLARE_BOARD("), 1)

    def test_ota_confirmation_waits_for_board_safe_state(self):
        board = (BOARD_DIR / "hi_yaka_board.cc").read_text(encoding="utf-8")
        application = (ROOT / "main" / "application.cc").read_text(encoding="utf-8")
        ota = (ROOT / "main" / "ota.cc").read_text(encoding="utf-8")
        self.assertIn("pca_safe_state_verified_ = true", board)
        self.assertIn("hi_yaka_ota_first_boot_self_test", application)
        self.assertLess(
            application.index("hi_yaka_ota_first_boot_self_test"),
            application.index("AudioServiceCallbacks callbacks"),
        )
        self.assertIn("esp_ota_mark_app_valid_cancel_rollback", ota)
        self.assertIn("esp_ota_mark_app_invalid_rollback_and_reboot", ota)

    def test_safe_build_does_not_enable_unvalidated_peripherals(self):
        config = json.loads((BOARD_DIR / "config.json").read_text(encoding="utf-8"))
        safe_options = set(config["builds"][0]["sdkconfig_append"])
        self.assertFalse(any(option.startswith("CONFIG_HI_YAKA_ENABLE_") for option in safe_options))
        source = (BOARD_DIR / "hi_yaka_board.cc").read_text(encoding="utf-8")
        self.assertIn("CONFIG_IDF_TARGET_ESP32C6", source)
        self.assertIn("defined(CONFIG_SPIRAM) && CONFIG_SPIRAM", source)
        self.assertIn("DummyAudioCodec", source)
        self.assertIn("PA safety interlock active", source)
        self.assertIn("ES7210_SEL_MIC1 | ES7210_SEL_MIC3, false", source)

    def test_lcd_does_not_consume_the_only_user_spi_controller(self):
        lcd_io = (BOARD_DIR / "hi_yaka_expander_lcd_io.cc").read_text(encoding="utf-8")
        self.assertIn("parlio_new_tx_unit", lcd_io)
        self.assertNotIn("spi_bus_initialize", lcd_io)
        self.assertNotIn("spi_bus_add_device", lcd_io)
        self.assertIn("YAKA_TF_SCLK_GPIO GPIO_NUM_19", (BOARD_DIR / "config.h").read_text(encoding="utf-8"))

    def test_recording_commit_is_rejected_after_loss_or_write_failure(self):
        runtime = (ROOT / "main" / "yaka" / "runtime_adapter.cc").read_text(
            encoding="utf-8"
        )
        self.assertIn("ports.storage = this", runtime)
        self.assertIn("recording_fault_.store(true)", runtime)
        self.assertIn("dropped_blocks_.load() != 0", runtime)
        self.assertIn("port->AbortRecording()", runtime)


if __name__ == "__main__":
    unittest.main()
