#ifndef HI_YAKA_PCA9557_H_
#define HI_YAKA_PCA9557_H_

#include <cstdint>
#include <atomic>
#include <mutex>

#include <driver/i2c_master.h>
#include <esp_err.h>

class HiYaKaPca9557 {
public:
    explicit HiYaKaPca9557(i2c_master_bus_handle_t bus);
    ~HiYaKaPca9557();

    esp_err_t InitializeSafe();
    esp_err_t SetOutputBit(uint8_t bit, bool high);
    esp_err_t SetLcdControl(bool selected, bool data_mode);
    esp_err_t ReadBack(uint8_t* output, uint8_t* configuration);
    esp_err_t ReadPinLevels(uint8_t* levels);

    bool ready() const { return ready_; }
    uint8_t output_shadow() const { return output_shadow_; }
    uint32_t first_write_failures() const { return first_write_failures_.load(); }
    uint32_t exhausted_writes() const { return exhausted_writes_.load(); }

private:
    esp_err_t WriteRegisterLocked(uint8_t reg, uint8_t value);
    esp_err_t ReadRegisterLocked(uint8_t reg, uint8_t* value);
    esp_err_t WriteOutputLocked(uint8_t output);

    i2c_master_dev_handle_t device_ = nullptr;
    mutable std::mutex mutex_;
    uint8_t output_shadow_ = 0;
    bool ready_ = false;
    std::atomic<uint32_t> first_write_failures_{0};
    std::atomic<uint32_t> exhausted_writes_{0};
};

#endif  // HI_YAKA_PCA9557_H_
