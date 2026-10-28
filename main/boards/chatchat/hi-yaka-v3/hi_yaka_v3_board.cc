#include "wifi_board.h"
#include "codecs/dummy_audio_codec.h"
#include "config.h"

#include <array>
#include <cstdint>
#include <inttypes.h>

#include <driver/i2c_master.h>
#include <esp_err.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sdkconfig.h>

#define TAG "HiYakaV3"

#if defined(CONFIG_SPIRAM) && CONFIG_SPIRAM
#error "ESP32-C6 does not support external PSRAM; hi-yaka-v3 must remain a no-PSRAM build"
#endif

namespace {

constexpr uint8_t kPcaOutputPortRegister = 0x01;
constexpr uint8_t kPcaConfigurationRegister = 0x03;

struct ExpectedI2cDevice {
    uint8_t address;
    const char* name;
};

constexpr std::array<ExpectedI2cDevice, 5> kExpectedI2cDevices = {{
    {YAKA_ES8311_I2C_ADDRESS, "ES8311"},
    {YAKA_PCA9557_I2C_ADDRESS, "PCA9557"},
    {YAKA_PN532_I2C_ADDRESS, "PN532"},
    {YAKA_ES7210_I2C_ADDRESS, "ES7210"},
    {YAKA_QMI8658A_I2C_ADDRESS, "QMI8658A"},
}};

}  // namespace

class HiYakaV3Board : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    i2c_master_dev_handle_t pca9557_ = nullptr;

    void LogPlatformBaseline() {
        uint32_t flash_id = 0;
        uint32_t flash_size = 0;
        esp_err_t id_result = esp_flash_read_id(nullptr, &flash_id);
        esp_err_t size_result = esp_flash_get_size(nullptr, &flash_size);

        ESP_LOGI(TAG, "M1 board=hi-yaka-v3 target=esp32c6 psram=unsupported");
        ESP_LOGI(TAG, "Reset reason=%d", static_cast<int>(esp_reset_reason()));
        if (id_result == ESP_OK && size_result == ESP_OK) {
            ESP_LOGI(TAG, "Flash id=0x%08" PRIx32 " size=%" PRIu32 " bytes", flash_id,
                     flash_size);
        } else {
            ESP_LOGE(TAG, "Flash query failed: id=%s size=%s", esp_err_to_name(id_result),
                     esp_err_to_name(size_result));
        }

        ESP_LOGI(TAG, "Internal heap free=%u largest=%u",
                 static_cast<unsigned>(
                     heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                 static_cast<unsigned>(
                     heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));
    }

    void InitializeI2c() {
        i2c_master_bus_config_t config = {};
        config.i2c_port = YAKA_I2C_PORT;
        config.sda_io_num = YAKA_I2C_SDA_GPIO;
        config.scl_io_num = YAKA_I2C_SCL_GPIO;
        config.clk_source = I2C_CLK_SRC_DEFAULT;
        config.glitch_ignore_cnt = 7;
        config.intr_priority = 0;
        config.trans_queue_depth = 0;
        config.flags.enable_internal_pullup = 0;
        ESP_ERROR_CHECK(i2c_new_master_bus(&config, &i2c_bus_));
        ESP_LOGI(TAG, "I2C initialized: port=%d SDA=%d SCL=%d frequency=%d Hz",
                 static_cast<int>(YAKA_I2C_PORT), static_cast<int>(YAKA_I2C_SDA_GPIO),
                 static_cast<int>(YAKA_I2C_SCL_GPIO), YAKA_I2C_FREQUENCY_HZ);
    }

    esp_err_t WritePca9557Register(uint8_t reg, uint8_t value) {
        const uint8_t data[] = {reg, value};
        return i2c_master_transmit(pca9557_, data, sizeof(data), 100);
    }

    esp_err_t ReadPca9557Register(uint8_t reg, uint8_t* value) {
        return i2c_master_transmit_receive(pca9557_, &reg, sizeof(reg), value, 1, 100);
    }

    void InitializePca9557SafeState() {
        esp_err_t probe_result =
            i2c_master_probe(i2c_bus_, YAKA_PCA9557_I2C_ADDRESS, 100);
        if (probe_result != ESP_OK) {
            ESP_LOGE(TAG, "PCA9557 probe failed at 0x%02X: %s",
                     YAKA_PCA9557_I2C_ADDRESS, esp_err_to_name(probe_result));
            return;
        }

        i2c_device_config_t device_config = {};
        device_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        device_config.device_address = YAKA_PCA9557_I2C_ADDRESS;
        device_config.scl_speed_hz = YAKA_I2C_FREQUENCY_HZ;
        ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_, &device_config, &pca9557_));

        // The output latch is written before any pin becomes an output. Reversing
        // these writes can pulse the backlight, motor, or LCD control signals.
        esp_err_t result =
            WritePca9557Register(kPcaOutputPortRegister, YAKA_PCA9557_SAFE_OUTPUT);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "PCA9557 safe output write failed: %s", esp_err_to_name(result));
            return;
        }

        result = WritePca9557Register(kPcaConfigurationRegister,
                                      YAKA_PCA9557_SAFE_CONFIGURATION);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "PCA9557 direction write failed: %s", esp_err_to_name(result));
            return;
        }

        uint8_t output = 0;
        uint8_t configuration = 0;
        esp_err_t output_result = ReadPca9557Register(kPcaOutputPortRegister, &output);
        esp_err_t configuration_result =
            ReadPca9557Register(kPcaConfigurationRegister, &configuration);
        if (output_result == ESP_OK && configuration_result == ESP_OK &&
            output == YAKA_PCA9557_SAFE_OUTPUT &&
            configuration == YAKA_PCA9557_SAFE_CONFIGURATION) {
            ESP_LOGI(TAG, "PCA9557 safe state verified: output=0x%02X config=0x%02X", output,
                     configuration);
        } else {
            ESP_LOGE(TAG,
                     "PCA9557 safe-state readback failed: output=0x%02X/%s config=0x%02X/%s",
                     output, esp_err_to_name(output_result), configuration,
                     esp_err_to_name(configuration_result));
        }
    }

    void ProbeExpectedI2cDevices() {
        for (const auto& device : kExpectedI2cDevices) {
            esp_err_t result = i2c_master_probe(i2c_bus_, device.address, 100);
            if (result == ESP_OK) {
                ESP_LOGI(TAG, "I2C present: %s at 0x%02X", device.name, device.address);
            } else {
                ESP_LOGW(TAG, "I2C missing: %s at 0x%02X (%s)", device.name, device.address,
                         esp_err_to_name(result));
            }
        }
    }

public:
    HiYakaV3Board() {
        LogPlatformBaseline();
        InitializeI2c();
        InitializePca9557SafeState();
        ProbeExpectedI2cDevices();
        ESP_LOGW(TAG, "M1 only: display, storage, NFC polling, IMU and real audio are disabled");
    }

    AudioCodec* GetAudioCodec() override {
        static DummyAudioCodec codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE);
        return &codec;
    }
};

DECLARE_BOARD(HiYakaV3Board);
