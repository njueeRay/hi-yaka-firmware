#include "hi_yaka_storage.h"

#include "config.h"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <driver/sdspi_host.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <esp_vfs_fat.h>

namespace {
constexpr char kTag[] = "HiYaKaStorage";
constexpr const char kPartialSuffix[] = ".pcm.part";
constexpr const char kCheckpointSuffix[] = ".ckp";
}  // namespace

HiYaKaStorage::~HiYaKaStorage() {
    CloseActive();
    if (card_ != nullptr) {
        esp_vfs_fat_sdcard_unmount(kMountPoint, card_);
    }
    if (spi_bus_initialized_) {
        spi_bus_free(SPI2_HOST);
    }
}

esp_err_t HiYaKaStorage::Mount() {
    spi_bus_config_t bus_config = {};
    bus_config.mosi_io_num = YAKA_TF_MOSI_GPIO;
    bus_config.miso_io_num = YAKA_TF_MISO_GPIO;
    bus_config.sclk_io_num = YAKA_TF_SCLK_GPIO;
    bus_config.quadwp_io_num = GPIO_NUM_NC;
    bus_config.quadhd_io_num = GPIO_NUM_NC;
    bus_config.max_transfer_sz = 16 * 1024;
    esp_err_t result = spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (result != ESP_OK) {
        return result;
    }
    spi_bus_initialized_ = true;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;
    host.max_freq_khz = 10000;
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.host_id = SPI2_HOST;
    slot_config.gpio_cs = YAKA_TF_CS_GPIO;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 5;
    mount_config.allocation_unit_size = 16 * 1024;
    result = esp_vfs_fat_sdspi_mount(kMountPoint, &host, &slot_config, &mount_config, &card_);
    if (result != ESP_OK) {
        card_ = nullptr;
        spi_bus_free(SPI2_HOST);
        spi_bus_initialized_ = false;
        ESP_LOGW(kTag, "TF unavailable (%s); startup continues without recording",
                 esp_err_to_name(result));
        return result;
    }

    if (mkdir(kRecordingDirectory, 0775) != 0 && errno != EEXIST) {
        result = ESP_FAIL;
    }
    if (result == ESP_OK) {
        ESP_LOGI(kTag, "TF mounted at %s using dedicated SPI2", kMountPoint);
    }
    return result;
}

std::string HiYaKaStorage::Path(uint32_t recording_id, const char* suffix) const {
    char path[96];
    std::snprintf(path, sizeof(path), "%s/rec-%08lu%s", kRecordingDirectory,
                  static_cast<unsigned long>(recording_id), suffix);
    return path;
}

void HiYaKaStorage::CloseActive() {
    if (active_file_ != nullptr) {
        std::fflush(active_file_);
        fsync(fileno(active_file_));
        std::fclose(active_file_);
        active_file_ = nullptr;
        active_id_ = 0;
    }
}

bool HiYaKaStorage::CreatePartial(uint32_t recording_id) {
    if (!ready() || active_file_ != nullptr || recording_id == 0) {
        return false;
    }
    active_file_ = std::fopen(Path(recording_id, kPartialSuffix).c_str(), "wb");
    if (active_file_ == nullptr) {
        return false;
    }
    active_id_ = recording_id;
    return true;
}

bool HiYaKaStorage::Append(uint32_t recording_id, const uint8_t* data, std::size_t size) {
    return active_file_ != nullptr && active_id_ == recording_id &&
           (size == 0 || std::fwrite(data, 1, size, active_file_) == size);
}

bool HiYaKaStorage::Sync(uint32_t recording_id) {
    return active_file_ != nullptr && active_id_ == recording_id &&
           std::fflush(active_file_) == 0 && fsync(fileno(active_file_)) == 0;
}

bool HiYaKaStorage::WriteCheckpoint(uint32_t recording_id, std::size_t committed_bytes) {
    const std::string final_path = Path(recording_id, kCheckpointSuffix);
    const std::string temporary_path = final_path + ".tmp";
    FILE* checkpoint = std::fopen(temporary_path.c_str(), "wb");
    if (checkpoint == nullptr) {
        return false;
    }
    const bool written = std::fprintf(checkpoint, "%u\n", static_cast<unsigned>(committed_bytes)) > 0;
    const bool synced = written && std::fflush(checkpoint) == 0 && fsync(fileno(checkpoint)) == 0;
    const bool closed = std::fclose(checkpoint) == 0;
    if (!(synced && closed && rename(temporary_path.c_str(), final_path.c_str()) == 0)) {
        unlink(temporary_path.c_str());
        return false;
    }
    return true;
}

bool HiYaKaStorage::ReadCheckpoint(uint32_t recording_id, std::size_t& committed_bytes) {
    FILE* checkpoint = std::fopen(Path(recording_id, kCheckpointSuffix).c_str(), "rb");
    if (checkpoint == nullptr) {
        return false;
    }
    unsigned parsed = 0;
    const bool ok = std::fscanf(checkpoint, "%u", &parsed) == 1;
    std::fclose(checkpoint);
    committed_bytes = parsed;
    return ok;
}

bool HiYaKaStorage::PartialSize(uint32_t recording_id, std::size_t& size) {
    struct stat status = {};
    if (stat(Path(recording_id, kPartialSuffix).c_str(), &status) != 0) {
        return false;
    }
    size = static_cast<std::size_t>(status.st_size);
    return true;
}

bool HiYaKaStorage::Truncate(uint32_t recording_id, std::size_t size) {
    return truncate(Path(recording_id, kPartialSuffix).c_str(), size) == 0;
}

bool HiYaKaStorage::Finalize(uint32_t recording_id, bool recovered) {
    if (active_id_ == recording_id) {
        CloseActive();
    }
    return rename(Path(recording_id, kPartialSuffix).c_str(),
                  Path(recording_id, recovered ? ".recovered.pcm" : ".pcm").c_str()) == 0;
}

bool HiYaKaStorage::RemoveCheckpoint(uint32_t recording_id) {
    const int result = unlink(Path(recording_id, kCheckpointSuffix).c_str());
    return result == 0 || errno == ENOENT;
}

bool HiYaKaStorage::Discard(uint32_t recording_id) {
    if (active_id_ == recording_id) {
        CloseActive();
    }
    const int partial_result = unlink(Path(recording_id, kPartialSuffix).c_str());
    const bool partial_ok = partial_result == 0 || errno == ENOENT;
    const int checkpoint_result = unlink(Path(recording_id, kCheckpointSuffix).c_str());
    const bool checkpoint_ok = checkpoint_result == 0 || errno == ENOENT;
    return partial_ok && checkpoint_ok;
}

std::size_t HiYaKaStorage::ListPending(uint32_t* recording_ids, std::size_t capacity) {
    if (recording_ids == nullptr || capacity == 0) {
        return 0;
    }
    DIR* directory = opendir(kRecordingDirectory);
    if (directory == nullptr) {
        return 0;
    }
    std::size_t count = 0;
    while (count < capacity) {
        const dirent* entry = readdir(directory);
        if (entry == nullptr) {
            break;
        }
        unsigned long id = 0;
        char suffix[16] = {};
        if (std::sscanf(entry->d_name, "rec-%08lu%15s", &id, suffix) == 2 &&
            std::strcmp(suffix, kPartialSuffix) == 0) {
            recording_ids[count++] = static_cast<uint32_t>(id);
        }
    }
    closedir(directory);
    return count;
}
