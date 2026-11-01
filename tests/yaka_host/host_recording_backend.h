#ifndef HI_YAKA_TEST_HOST_RECORDING_BACKEND_H_
#define HI_YAKA_TEST_HOST_RECORDING_BACKEND_H_

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include "recording_store.h"

namespace hi_yaka::test {

class HostRecordingBackend final : public RecordingBackend {
public:
    explicit HostRecordingBackend(std::filesystem::path root,
                                  std::size_t quota = std::numeric_limits<std::size_t>::max())
        : root_(std::move(root)), quota_(quota) {
        std::error_code error;
        std::filesystem::create_directories(root_, error);
    }

    bool CreatePartial(uint32_t recording_id) override {
        if (Exists(Partial(recording_id)) || Exists(Final(recording_id, false)) ||
            Exists(Final(recording_id, true))) {
            return false;
        }
        std::ofstream file(Partial(recording_id), std::ios::binary | std::ios::trunc);
        return file.good();
    }

    bool Append(uint32_t recording_id, const uint8_t* data, std::size_t size) override {
        std::size_t current_size = 0;
        if (!PartialSize(recording_id, current_size) || current_size + size > quota_) {
            return false;
        }
        std::ofstream file(Partial(recording_id), std::ios::binary | std::ios::app);
        file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
        return file.good();
    }

    bool Sync(uint32_t recording_id) override { return Exists(Partial(recording_id)); }

    bool WriteCheckpoint(uint32_t recording_id, std::size_t committed_bytes) override {
        const auto temporary = Checkpoint(recording_id).string() + ".tmp";
        {
            std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
            const uint64_t value = committed_bytes;
            file.write(reinterpret_cast<const char*>(&value), sizeof(value));
            if (!file.good()) {
                return false;
            }
        }
        std::error_code error;
        std::filesystem::remove(Checkpoint(recording_id), error);
        error.clear();
        std::filesystem::rename(temporary, Checkpoint(recording_id), error);
        return !error;
    }

    bool ReadCheckpoint(uint32_t recording_id, std::size_t& committed_bytes) override {
        uint64_t value = 0;
        std::ifstream file(Checkpoint(recording_id), std::ios::binary);
        file.read(reinterpret_cast<char*>(&value), sizeof(value));
        if (!file.good()) {
            return false;
        }
        committed_bytes = static_cast<std::size_t>(value);
        return true;
    }

    bool PartialSize(uint32_t recording_id, std::size_t& size) override {
        std::error_code error;
        const auto value = std::filesystem::file_size(Partial(recording_id), error);
        if (error) {
            return false;
        }
        size = static_cast<std::size_t>(value);
        return true;
    }

    bool Truncate(uint32_t recording_id, std::size_t size) override {
        std::error_code error;
        std::filesystem::resize_file(Partial(recording_id), size, error);
        return !error;
    }

    bool Finalize(uint32_t recording_id, bool recovered) override {
        const auto destination = Final(recording_id, recovered);
        if (Exists(destination)) {
            return false;
        }
        std::error_code error;
        std::filesystem::rename(Partial(recording_id), destination, error);
        return !error;
    }

    bool RemoveCheckpoint(uint32_t recording_id) override {
        std::error_code error;
        std::filesystem::remove(Checkpoint(recording_id), error);
        return !error;
    }

    bool Discard(uint32_t recording_id) override {
        std::error_code first_error;
        std::filesystem::remove(Partial(recording_id), first_error);
        std::error_code second_error;
        std::filesystem::remove(Checkpoint(recording_id), second_error);
        return !first_error && !second_error;
    }

    std::size_t ListPending(uint32_t* recording_ids, std::size_t capacity) override {
        std::vector<uint32_t> pending;
        std::error_code error;
        for (std::filesystem::directory_iterator iterator(root_, error), end;
             !error && iterator != end; iterator.increment(error)) {
            const std::string name = iterator->path().filename().string();
            constexpr char kPrefix[] = "recording-";
            constexpr char kSuffix[] = ".part";
            if (name.rfind(kPrefix, 0) != 0 ||
                name.size() <= sizeof(kPrefix) + sizeof(kSuffix) - 2 ||
                name.compare(name.size() - sizeof(kSuffix) + 1, sizeof(kSuffix) - 1, kSuffix) != 0) {
                continue;
            }
            const std::string digits = name.substr(sizeof(kPrefix) - 1,
                                                   name.size() - sizeof(kPrefix) - sizeof(kSuffix) + 2);
            try {
                pending.push_back(static_cast<uint32_t>(std::stoul(digits)));
            } catch (...) {
                continue;
            }
        }
        std::sort(pending.begin(), pending.end());
        const std::size_t count = std::min(capacity, pending.size());
        std::copy_n(pending.begin(), count, recording_ids);
        return count;
    }

    bool FinalExists(uint32_t recording_id, bool recovered) const {
        return Exists(Final(recording_id, recovered));
    }

    std::size_t FinalSize(uint32_t recording_id, bool recovered) const {
        std::error_code error;
        const auto size = std::filesystem::file_size(Final(recording_id, recovered), error);
        return error ? 0 : static_cast<std::size_t>(size);
    }

private:
    static bool Exists(const std::filesystem::path& path) {
        std::error_code error;
        return std::filesystem::exists(path, error) && !error;
    }

    std::filesystem::path Partial(uint32_t id) const {
        return root_ / ("recording-" + std::to_string(id) + ".part");
    }
    std::filesystem::path Checkpoint(uint32_t id) const {
        return root_ / ("recording-" + std::to_string(id) + ".checkpoint");
    }
    std::filesystem::path Final(uint32_t id, bool recovered) const {
        return root_ / ("recording-" + std::to_string(id) +
                        (recovered ? "-recovered.pcm" : ".pcm"));
    }

    std::filesystem::path root_;
    std::size_t quota_;
};

}  // namespace hi_yaka::test

#endif  // HI_YAKA_TEST_HOST_RECORDING_BACKEND_H_
