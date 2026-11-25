#include "hi_yaka_qmi8658a.h"

#include "config.h"

#include <array>

#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {
constexpr char kTag[] = "HiYaKaQMI8658A";

constexpr uint8_t kWhoAmIRegister = 0x00;
constexpr uint8_t kRevisionRegister = 0x01;
constexpr uint8_t kCtrl1Register = 0x02;
constexpr uint8_t kCtrl2Register = 0x03;
constexpr uint8_t kCtrl3Register = 0x04;
constexpr uint8_t kCtrl7Register = 0x08;
constexpr uint8_t kStatus0Register = 0x2E;
constexpr uint8_t kTemperatureLowRegister = 0x33;
constexpr uint8_t kResetRegister = 0x60;

constexpr uint8_t kExpectedWhoAmI = 0x05;
constexpr uint8_t kResetCommand = 0xB0;
constexpr uint8_t kAddressAutoIncrement = 1U << 6;
constexpr uint8_t kAccelAndGyroDataReady = 0x03;
constexpr uint8_t kEnableAccelAndGyro = 0x03;

// QMI8658A Rev A table 22. In 6-DoF mode ODR code 0x6 is about 112.1 Hz.
// Accelerometer full scale 001 = +/-4 g; gyroscope full scale 101 = +/-512 dps.
constexpr uint8_t kAccel4g112Hz = 0x16;
constexpr uint8_t kGyro512dps112Hz = 0x56;
constexpr float kAccelCountsPerG = 8192.0f;
constexpr float kGyroCountsPerDps = 64.0f;
constexpr int kTransferTimeoutMs = 100;

int16_t ReadLittleEndian16(const uint8_t* bytes) {
    return static_cast<int16_t>(static_cast<uint16_t>(bytes[0]) |
                                (static_cast<uint16_t>(bytes[1]) << 8));
}
}  // namespace

HiYaKaQmi8658a::HiYaKaQmi8658a(i2c_master_bus_handle_t bus) : bus_(bus) {}

HiYaKaQmi8658a::~HiYaKaQmi8658a() {
    Shutdown();
    if (device_ != nullptr) {
        i2c_master_bus_rm_device(device_);
    }
}

esp_err_t HiYaKaQmi8658a::ReadRegister(uint8_t address, uint8_t* value) {
    return ReadRegisters(address, value, 1);
}

esp_err_t HiYaKaQmi8658a::ReadRegisters(uint8_t address, uint8_t* values,
                                        std::size_t count) {
    if (device_ == nullptr || values == nullptr || count == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    return i2c_master_transmit_receive(device_, &address, 1, values, count,
                                       kTransferTimeoutMs);
}

esp_err_t HiYaKaQmi8658a::WriteRegister(uint8_t address, uint8_t value) {
    if (device_ == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    const uint8_t command[] = {address, value};
    return i2c_master_transmit(device_, command, sizeof(command), kTransferTimeoutMs);
}

esp_err_t HiYaKaQmi8658a::Initialize() {
    if (bus_ == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (ready_) {
        return ESP_OK;
    }

    if (device_ == nullptr) {
        i2c_device_config_t config = {};
        config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        config.device_address = YAKA_QMI8658A_I2C_ADDRESS;
        config.scl_speed_hz = YAKA_I2C_FREQUENCY_HZ;
        esp_err_t result = i2c_master_bus_add_device(bus_, &config, &device_);
        if (result != ESP_OK) {
            return result;
        }
    }

    uint8_t who_am_i = 0;
    esp_err_t result = ReadRegister(kWhoAmIRegister, &who_am_i);
    if (result != ESP_OK || who_am_i != kExpectedWhoAmI) {
        ESP_LOGE(kTag, "identity failed: result=%s WHO_AM_I=0x%02X expected=0x%02X",
                 esp_err_to_name(result), who_am_i, kExpectedWhoAmI);
        return result == ESP_OK ? ESP_ERR_INVALID_RESPONSE : result;
    }

    // The Rev A register table specifies 0xB0. One prose line says 0x0B;
    // QST-compatible Zephyr/RIOT drivers and the table agree on 0xB0.
    result = WriteRegister(kResetRegister, kResetCommand);
    if (result != ESP_OK) {
        return result;
    }
    vTaskDelay(pdMS_TO_TICKS(20));

    result = ReadRegister(kWhoAmIRegister, &who_am_i);
    if (result != ESP_OK || who_am_i != kExpectedWhoAmI) {
        return result == ESP_OK ? ESP_ERR_INVALID_RESPONSE : result;
    }
    result = ReadRegister(kRevisionRegister, &revision_id_);
    if (result != ESP_OK) {
        return result;
    }

    // Configure only while both sensors are disabled, as required by the data sheet.
    if ((result = WriteRegister(kCtrl7Register, 0x00)) != ESP_OK ||
        (result = WriteRegister(kCtrl1Register, kAddressAutoIncrement)) != ESP_OK ||
        (result = WriteRegister(kCtrl2Register, kAccel4g112Hz)) != ESP_OK ||
        (result = WriteRegister(kCtrl3Register, kGyro512dps112Hz)) != ESP_OK ||
        (result = WriteRegister(kCtrl7Register, kEnableAccelAndGyro)) != ESP_OK) {
        return result;
    }
    vTaskDelay(pdMS_TO_TICKS(20));

    std::array<uint8_t, 4> readback{};
    const std::array<uint8_t, 4> registers = {
        kCtrl1Register, kCtrl2Register, kCtrl3Register, kCtrl7Register};
    for (std::size_t index = 0; index < registers.size(); ++index) {
        result = ReadRegister(registers[index], &readback[index]);
        if (result != ESP_OK) {
            return result;
        }
    }
    if (readback[0] != kAddressAutoIncrement || readback[1] != kAccel4g112Hz ||
        readback[2] != kGyro512dps112Hz || readback[3] != kEnableAccelAndGyro) {
        ESP_LOGE(kTag, "configuration readback mismatch: %02X %02X %02X %02X",
                 readback[0], readback[1], readback[2], readback[3]);
        return ESP_ERR_INVALID_RESPONSE;
    }

    ready_ = true;
    ESP_LOGI(kTag,
             "ready: WHO_AM_I=0x%02X revision=0x%02X accel=+/-4g gyro=+/-512dps ODR~112Hz",
             who_am_i, revision_id_);
    return ESP_OK;
}

esp_err_t HiYaKaQmi8658a::ReadSample(HiYaKaImuSample* sample, uint32_t timeout_ms) {
    if (!ready_ || sample == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    const int64_t deadline_us = esp_timer_get_time() + static_cast<int64_t>(timeout_ms) * 1000;
    uint8_t status = 0;
    do {
        const esp_err_t result = ReadRegister(kStatus0Register, &status);
        if (result != ESP_OK) {
            return result;
        }
        if ((status & kAccelAndGyroDataReady) == kAccelAndGyroDataReady) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    } while (esp_timer_get_time() < deadline_us);
    if ((status & kAccelAndGyroDataReady) != kAccelAndGyroDataReady) {
        return ESP_ERR_TIMEOUT;
    }

    std::array<uint8_t, 14> bytes{};
    const esp_err_t result =
        ReadRegisters(kTemperatureLowRegister, bytes.data(), bytes.size());
    if (result != ESP_OK) {
        return result;
    }

    const int16_t temperature_raw = ReadLittleEndian16(bytes.data());
    sample->temperature_c = static_cast<float>(temperature_raw) / 256.0f;
    for (int axis = 0; axis < 3; ++axis) {
        sample->accel_raw[axis] = ReadLittleEndian16(&bytes[2 + axis * 2]);
        sample->gyro_raw[axis] = ReadLittleEndian16(&bytes[8 + axis * 2]);
        sample->acceleration_g[axis] =
            static_cast<float>(sample->accel_raw[axis]) / kAccelCountsPerG;
        sample->angular_rate_dps[axis] =
            static_cast<float>(sample->gyro_raw[axis]) / kGyroCountsPerDps;
    }
    sample->timestamp_us = esp_timer_get_time();
    return ESP_OK;
}

esp_err_t HiYaKaQmi8658a::Shutdown() {
    if (device_ == nullptr) {
        return ESP_OK;
    }
    const esp_err_t result = WriteRegister(kCtrl7Register, 0x00);
    if (result == ESP_OK) {
        ready_ = false;
    }
    return result;
}
