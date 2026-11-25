#ifndef HI_YAKA_PN532_H_
#define HI_YAKA_PN532_H_

#include <array>
#include <cstdint>

#include <driver/i2c_master.h>
#include <esp_err.h>
#include <pn532.h>

enum class HiYaKaNfcPollResult : uint8_t {
    kFound,
    kNoTarget,
    kTimeout,
    kTransportError,
    kProtocolError,
    kNoMemory,
    kInvalidState,
};

struct HiYaKaNfcTag {
    std::array<uint8_t, 10> uid{};
    uint8_t uid_length = 0;
    uint8_t target_count = 0;
    uint8_t sak = 0;
    uint16_t atqa = 0;
};

class HiYaKaPn532 final {
public:
    explicit HiYaKaPn532(i2c_master_bus_handle_t bus);
    ~HiYaKaPn532();

    esp_err_t Initialize();
    HiYaKaNfcPollResult PollIso14443A(HiYaKaNfcTag* tag, uint16_t timeout_ms = 250);
    esp_err_t SetRfOff();

    bool ready() const { return device_ != nullptr; }
    uint32_t firmware_version() const { return firmware_version_; }

private:
    i2c_master_bus_handle_t bus_handle_ = nullptr;
    pn532_bus_t* bus_ = nullptr;
    pn532_t* device_ = nullptr;
    uint32_t firmware_version_ = 0;
};

#endif  // HI_YAKA_PN532_H_
