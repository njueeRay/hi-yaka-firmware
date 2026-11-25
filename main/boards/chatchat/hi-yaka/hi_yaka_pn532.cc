#include "hi_yaka_pn532.h"

#include "config.h"

#include <algorithm>
#include <cstdlib>

#include <esp_log.h>
#include <pn532.h>

namespace {
constexpr char kTag[] = "HiYaKaPN532";
}

HiYaKaPn532::HiYaKaPn532(i2c_master_bus_handle_t bus) : bus_handle_(bus) {}

HiYaKaPn532::~HiYaKaPn532() {
    if (device_ != nullptr) {
        pn532_deinit(device_, true);
    } else if (bus_ != nullptr) {
        pn532_bus_destroy(bus_);
    }
}

esp_err_t HiYaKaPn532::Initialize() {
    if (bus_handle_ == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (device_ != nullptr) {
        return ESP_OK;
    }

    // The PN532 shares I2C0 with the codecs, expander and IMU. IRQ and
    // RES&OFF are not routed to the MCU, so this board must use bounded polling.
    bus_ = pn532_i2c_attach(YAKA_I2C_PORT, YAKA_PN532_I2C_ADDRESS,
                            YAKA_I2C_FREQUENCY_HZ);
    if (bus_ == nullptr) {
        return ESP_FAIL;
    }
    device_ = pn532_init(bus_, GPIO_NUM_NC, GPIO_NUM_NC);
    if (device_ == nullptr) {
        pn532_bus_destroy(bus_);
        bus_ = nullptr;
        return ESP_ERR_NOT_FOUND;
    }

    firmware_version_ = pn532_get_firmware_version(device_);
    if (firmware_version_ == 0 || (firmware_version_ >> 24) != 0x32) {
        pn532_deinit(device_, true);
        device_ = nullptr;
        bus_ = nullptr;
        return ESP_ERR_INVALID_RESPONSE;
    }
    ESP_LOGI(kTag, "ready: IC=0x%02lX version=%lu.%lu support=0x%02lX polling-only",
             static_cast<unsigned long>((firmware_version_ >> 24) & 0xFF),
             static_cast<unsigned long>((firmware_version_ >> 16) & 0xFF),
             static_cast<unsigned long>((firmware_version_ >> 8) & 0xFF),
             static_cast<unsigned long>(firmware_version_ & 0xFF));
    return ESP_OK;
}

HiYaKaNfcPollResult HiYaKaPn532::PollIso14443A(HiYaKaNfcTag* tag,
                                                uint16_t timeout_ms) {
    if (device_ == nullptr || tag == nullptr) {
        return HiYaKaNfcPollResult::kInvalidState;
    }
    *tag = {};
    device_->timeout_ms = timeout_ms;
    pn532_poll_status_t status = PN532_POLL_INVALID_ARGUMENT;
    pn532_uids_array_t* targets = pn532_14443_get_all_uids_ex(device_, &status);
    if (targets != nullptr && status == PN532_POLL_FOUND && targets->uids_count > 0) {
        const pn532_uid_t& first = targets->uids[0];
        tag->target_count = targets->uids_count;
        tag->uid_length = static_cast<uint8_t>(
            std::clamp<int>(first.uid_length, 0, static_cast<int>(tag->uid.size())));
        std::copy_n(first.uid, tag->uid_length, tag->uid.begin());
        tag->sak = first.sak;
        tag->atqa = first.atqa;
        std::free(targets);
        return HiYaKaNfcPollResult::kFound;
    }
    std::free(targets);
    switch (status) {
        case PN532_POLL_NO_TARGET:
            return HiYaKaNfcPollResult::kNoTarget;
        case PN532_POLL_TIMEOUT:
            return HiYaKaNfcPollResult::kTimeout;
        case PN532_POLL_TRANSPORT_ERROR:
            return HiYaKaNfcPollResult::kTransportError;
        case PN532_POLL_PROTOCOL_ERROR:
            return HiYaKaNfcPollResult::kProtocolError;
        case PN532_POLL_NO_MEMORY:
            return HiYaKaNfcPollResult::kNoMemory;
        case PN532_POLL_INVALID_ARGUMENT:
            return HiYaKaNfcPollResult::kInvalidState;
        case PN532_POLL_FOUND:
            return HiYaKaNfcPollResult::kProtocolError;
    }
    return HiYaKaNfcPollResult::kProtocolError;
}

esp_err_t HiYaKaPn532::SetRfOff() {
    if (device_ == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    return pn532_set_rf_off(device_) ? ESP_OK : ESP_FAIL;
}
