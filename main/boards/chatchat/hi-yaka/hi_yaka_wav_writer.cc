#include "hi_yaka_wav_writer.h"

#include <cerrno>
#include <cstring>
#include <limits>
#include <sys/stat.h>
#include <unistd.h>

#include <esp_log.h>

namespace {
constexpr char kTag[] = "HiYaKaWav";
constexpr char kDiagnosticsDirectory[] = "/sdcard/diagnostics";

void PutLe16(uint8_t* destination, uint16_t value) {
    destination[0] = static_cast<uint8_t>(value & 0xFFU);
    destination[1] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
}

void PutLe32(uint8_t* destination, uint32_t value) {
    destination[0] = static_cast<uint8_t>(value & 0xFFU);
    destination[1] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
    destination[2] = static_cast<uint8_t>((value >> 16U) & 0xFFU);
    destination[3] = static_cast<uint8_t>((value >> 24U) & 0xFFU);
}
}  // namespace

HiYaKaWavWriter::~HiYaKaWavWriter() {
    Close();
}

bool HiYaKaWavWriter::Open(const char* path, uint32_t sample_rate, uint16_t channels) {
    if (file_ != nullptr || path == nullptr || sample_rate == 0 || channels == 0) {
        return false;
    }
    if (mkdir(kDiagnosticsDirectory, 0775) != 0 && errno != EEXIST) {
        ESP_LOGE(kTag, "cannot create %s: errno=%d", kDiagnosticsDirectory, errno);
        return false;
    }
    file_ = std::fopen(path, "wb");
    if (file_ == nullptr) {
        ESP_LOGE(kTag, "cannot open %s: errno=%d", path, errno);
        return false;
    }
    sample_rate_ = sample_rate;
    channels_ = channels;
    data_bytes_ = 0;
    if (!WriteHeader(0)) {
        std::fclose(file_);
        file_ = nullptr;
        return false;
    }
    if (std::fseek(file_, 44, SEEK_SET) != 0) {
        std::fclose(file_);
        file_ = nullptr;
        return false;
    }
    ESP_LOGI(kTag, "WAV capture opened: %s rate=%lu channels=%u", path,
             static_cast<unsigned long>(sample_rate_), channels_);
    return true;
}

bool HiYaKaWavWriter::Append(const int16_t* samples, std::size_t sample_count) {
    if (file_ == nullptr || (samples == nullptr && sample_count != 0)) {
        return false;
    }
    if (sample_count > (std::numeric_limits<uint32_t>::max() - data_bytes_) /
                           sizeof(int16_t)) {
        return false;
    }
    const std::size_t bytes = sample_count * sizeof(int16_t);
    if (bytes != 0 && std::fwrite(samples, 1, bytes, file_) != bytes) {
        ESP_LOGE(kTag, "WAV write failed: errno=%d", errno);
        return false;
    }
    data_bytes_ += static_cast<uint32_t>(bytes);
    return true;
}

bool HiYaKaWavWriter::Close() {
    if (file_ == nullptr) {
        return true;
    }
    const uint32_t completed_bytes = data_bytes_;
    bool ok = WriteHeader(completed_bytes);
    ok = std::fflush(file_) == 0 && ok;
    ok = fsync(fileno(file_)) == 0 && ok;
    ok = std::fclose(file_) == 0 && ok;
    file_ = nullptr;
    ESP_LOGI(kTag, "WAV capture closed: bytes=%lu status=%s",
             static_cast<unsigned long>(completed_bytes), ok ? "ok" : "failed");
    return ok;
}

bool HiYaKaWavWriter::WriteHeader(uint32_t data_bytes) {
    if (file_ == nullptr || data_bytes > std::numeric_limits<uint32_t>::max() - 36U) {
        return false;
    }
    uint8_t header[44] = {};
    std::memcpy(header, "RIFF", 4);
    PutLe32(header + 4, 36U + data_bytes);
    std::memcpy(header + 8, "WAVEfmt ", 8);
    PutLe32(header + 16, 16U);
    PutLe16(header + 20, 1U);
    PutLe16(header + 22, channels_);
    PutLe32(header + 24, sample_rate_);
    PutLe32(header + 28, sample_rate_ * channels_ * sizeof(int16_t));
    PutLe16(header + 32, static_cast<uint16_t>(channels_ * sizeof(int16_t)));
    PutLe16(header + 34, 16U);
    std::memcpy(header + 36, "data", 4);
    PutLe32(header + 40, data_bytes);

    const long restore_position = std::ftell(file_);
    if (restore_position < 0 || std::fseek(file_, 0, SEEK_SET) != 0 ||
        std::fwrite(header, 1, sizeof(header), file_) != sizeof(header) ||
        std::fseek(file_, restore_position, SEEK_SET) != 0) {
        return false;
    }
    return true;
}
