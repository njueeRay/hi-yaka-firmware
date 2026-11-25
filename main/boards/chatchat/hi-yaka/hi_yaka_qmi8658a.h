#ifndef HI_YAKA_QMI8658A_H_
#define HI_YAKA_QMI8658A_H_

#include <cstddef>
#include <cstdint>

#include <driver/i2c_master.h>
#include <esp_err.h>

struct HiYaKaImuSample {
    int16_t accel_raw[3]{};
    int16_t gyro_raw[3]{};
    float acceleration_g[3]{};
    float angular_rate_dps[3]{};
    float temperature_c = 0.0f;
    int64_t timestamp_us = 0;
};

class HiYaKaQmi8658a final {
public:
    explicit HiYaKaQmi8658a(i2c_master_bus_handle_t bus);
    ~HiYaKaQmi8658a();

    esp_err_t Initialize();
    esp_err_t ReadSample(HiYaKaImuSample* sample, uint32_t timeout_ms = 100);
    esp_err_t Shutdown();

    bool ready() const { return ready_; }
    uint8_t revision_id() const { return revision_id_; }

private:
    esp_err_t ReadRegister(uint8_t address, uint8_t* value);
    esp_err_t ReadRegisters(uint8_t address, uint8_t* values, std::size_t count);
    esp_err_t WriteRegister(uint8_t address, uint8_t value);

    i2c_master_bus_handle_t bus_ = nullptr;
    i2c_master_dev_handle_t device_ = nullptr;
    uint8_t revision_id_ = 0;
    bool ready_ = false;
};

#endif  // HI_YAKA_QMI8658A_H_
