#include "hi_yaka_pca9557.h"

#include "config.h"

#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {

constexpr char kTag[] = "HiYaKaPca9557";
constexpr uint8_t kInputPortRegister = 0x00;
constexpr uint8_t kOutputPortRegister = 0x01;
constexpr uint8_t kConfigurationRegister = 0x03;
constexpr int kRegisterWriteAttempts = 3;

}  // namespace

HiYaKaPca9557::HiYaKaPca9557(i2c_master_bus_handle_t bus) {
    i2c_device_config_t config = {};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = YAKA_PCA9557_I2C_ADDRESS;
    config.scl_speed_hz = YAKA_I2C_FREQUENCY_HZ;
    const esp_err_t result = i2c_master_bus_add_device(bus, &config, &device_);
    if (result != ESP_OK) {
        ESP_LOGE(kTag, "add device failed: %s", esp_err_to_name(result));
        device_ = nullptr;
    }
}

HiYaKaPca9557::~HiYaKaPca9557() {
    if (device_ != nullptr) {
        i2c_master_bus_rm_device(device_);
    }
}

esp_err_t HiYaKaPca9557::WriteRegisterLocked(uint8_t reg, uint8_t value) {
    if (device_ == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    const uint8_t data[] = {reg, value};
    esp_err_t result = ESP_FAIL;
    for (int attempt = 1; attempt <= kRegisterWriteAttempts; ++attempt) {
        result = i2c_master_transmit(device_, data, sizeof(data), 100);
        if (result == ESP_OK) {
            return ESP_OK;
        }
        if (attempt < kRegisterWriteAttempts) {
            ESP_LOGW(kTag, "register 0x%02X write failed (%s), retry %d/%d", reg,
                     esp_err_to_name(result), attempt + 1, kRegisterWriteAttempts);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    ESP_LOGE(kTag,
             "register 0x%02X write exhausted (%s): SDA(GPIO%d)=%d SCL(GPIO%d)=%d",
             reg, esp_err_to_name(result), static_cast<int>(YAKA_I2C_SDA_GPIO),
             gpio_get_level(YAKA_I2C_SDA_GPIO), static_cast<int>(YAKA_I2C_SCL_GPIO),
             gpio_get_level(YAKA_I2C_SCL_GPIO));
    return result;
}

esp_err_t HiYaKaPca9557::ReadRegisterLocked(uint8_t reg, uint8_t* value) {
    if (device_ == nullptr || value == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_transmit_receive(device_, &reg, 1, value, 1, 100);
}

esp_err_t HiYaKaPca9557::WriteOutputLocked(uint8_t output) {
    const esp_err_t result = WriteRegisterLocked(kOutputPortRegister, output);
    if (result == ESP_OK) {
        output_shadow_ = output;
    }
    return result;
}

esp_err_t HiYaKaPca9557::InitializeSafe() {
    std::lock_guard<std::mutex> lock(mutex_);
    ready_ = false;
    output_shadow_ = YAKA_PCA9557_SAFE_OUTPUT;

    // The output latch must be safe before any pin changes from input to output.
    esp_err_t result = WriteRegisterLocked(kOutputPortRegister, output_shadow_);
    if (result != ESP_OK) {
        return result;
    }
    result = WriteRegisterLocked(kConfigurationRegister, YAKA_PCA9557_SAFE_CONFIGURATION);
    if (result != ESP_OK) {
        return result;
    }

    uint8_t output = 0;
    uint8_t configuration = 0;
    result = ReadRegisterLocked(kOutputPortRegister, &output);
    if (result == ESP_OK) {
        result = ReadRegisterLocked(kConfigurationRegister, &configuration);
    }
    if (result != ESP_OK) {
        return result;
    }
    if (output != YAKA_PCA9557_SAFE_OUTPUT ||
        configuration != YAKA_PCA9557_SAFE_CONFIGURATION) {
        ESP_LOGE(kTag, "safe-state mismatch output=0x%02X config=0x%02X", output,
                 configuration);
        return ESP_ERR_INVALID_RESPONSE;
    }

    output_shadow_ = output;
    ready_ = true;
    ESP_LOGI(kTag, "safe state verified output=0x%02X config=0x%02X", output,
             configuration);
    return ESP_OK;
}

esp_err_t HiYaKaPca9557::SetOutputBit(uint8_t bit, bool high) {
    if (bit > 7) {
        return ESP_ERR_INVALID_ARG;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ready_) {
        return ESP_ERR_INVALID_STATE;
    }
    const uint8_t mask = static_cast<uint8_t>(1U << bit);
    const uint8_t next = high ? static_cast<uint8_t>(output_shadow_ | mask)
                              : static_cast<uint8_t>(output_shadow_ & ~mask);
    if (next == output_shadow_) {
        return ESP_OK;
    }
    return WriteOutputLocked(next);
}

esp_err_t HiYaKaPca9557::SetLcdControl(bool selected, bool data_mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ready_) {
        return ESP_ERR_INVALID_STATE;
    }
    const uint8_t cs_mask = static_cast<uint8_t>(1U << YAKA_PCA9557_LCD_CS_BIT);
    const uint8_t dc_mask = static_cast<uint8_t>(1U << YAKA_PCA9557_LCD_DC_BIT);
    uint8_t next = output_shadow_;
    next = selected ? static_cast<uint8_t>(next & ~cs_mask)
                    : static_cast<uint8_t>(next | cs_mask);
    next = data_mode ? static_cast<uint8_t>(next | dc_mask)
                     : static_cast<uint8_t>(next & ~dc_mask);
    if (next == output_shadow_) {
        return ESP_OK;
    }
    return WriteOutputLocked(next);
}

esp_err_t HiYaKaPca9557::ReadBack(uint8_t* output, uint8_t* configuration) {
    if (output == nullptr || configuration == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    esp_err_t result = ReadRegisterLocked(kOutputPortRegister, output);
    if (result == ESP_OK) {
        result = ReadRegisterLocked(kConfigurationRegister, configuration);
    }
    return result;
}

esp_err_t HiYaKaPca9557::ReadPinLevels(uint8_t* levels) {
    if (levels == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    return ReadRegisterLocked(kInputPortRegister, levels);
}
