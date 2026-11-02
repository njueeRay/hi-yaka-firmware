#ifndef HI_YAKA_STORAGE_H_
#define HI_YAKA_STORAGE_H_

#include "recording_store.h"

#include <cstdio>
#include <string>

#include <sdmmc_cmd.h>

class HiYaKaStorage final : public hi_yaka::RecordingBackend {
public:
    HiYaKaStorage() = default;
    ~HiYaKaStorage() override;

    esp_err_t Mount();
    bool ready() const { return card_ != nullptr; }

    bool CreatePartial(uint32_t recording_id) override;
    bool Append(uint32_t recording_id, const uint8_t* data, std::size_t size) override;
    bool Sync(uint32_t recording_id) override;
    bool WriteCheckpoint(uint32_t recording_id, std::size_t committed_bytes) override;
    bool ReadCheckpoint(uint32_t recording_id, std::size_t& committed_bytes) override;
    bool PartialSize(uint32_t recording_id, std::size_t& size) override;
    bool Truncate(uint32_t recording_id, std::size_t size) override;
    bool Finalize(uint32_t recording_id, bool recovered) override;
    bool RemoveCheckpoint(uint32_t recording_id) override;
    bool Discard(uint32_t recording_id) override;
    std::size_t ListPending(uint32_t* recording_ids, std::size_t capacity) override;

private:
    std::string Path(uint32_t recording_id, const char* suffix) const;
    void CloseActive();

    static constexpr const char* kMountPoint = "/sdcard";
    static constexpr const char* kRecordingDirectory = "/sdcard/recordings";
    sdmmc_card_t* card_ = nullptr;
    FILE* active_file_ = nullptr;
    uint32_t active_id_ = 0;
    bool spi_bus_initialized_ = false;
};

#endif  // HI_YAKA_STORAGE_H_
