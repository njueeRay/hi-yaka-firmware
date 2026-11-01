#ifndef HI_YAKA_RECORDING_STORE_H_
#define HI_YAKA_RECORDING_STORE_H_

#include <array>
#include <cstddef>
#include <cstdint>

#include "product_ports.h"

namespace hi_yaka {

class RecordingBackend {
public:
    virtual ~RecordingBackend() = default;
    virtual bool CreatePartial(uint32_t recording_id) = 0;
    virtual bool Append(uint32_t recording_id, const uint8_t* data, std::size_t size) = 0;
    virtual bool Sync(uint32_t recording_id) = 0;
    virtual bool WriteCheckpoint(uint32_t recording_id, std::size_t committed_bytes) = 0;
    virtual bool ReadCheckpoint(uint32_t recording_id, std::size_t& committed_bytes) = 0;
    virtual bool PartialSize(uint32_t recording_id, std::size_t& size) = 0;
    virtual bool Truncate(uint32_t recording_id, std::size_t size) = 0;
    virtual bool Finalize(uint32_t recording_id, bool recovered) = 0;
    virtual bool RemoveCheckpoint(uint32_t recording_id) = 0;
    virtual bool Discard(uint32_t recording_id) = 0;
    virtual std::size_t ListPending(uint32_t* recording_ids, std::size_t capacity) = 0;
};

enum class RecordingStoreState : uint8_t {
    kIdle,
    kOpen,
    kFaulted,
};

struct RecordingRecoveryMetrics {
    uint32_t recovered{0};
    uint32_t discarded{0};
    uint32_t failed{0};
};

class RecordingStore {
public:
    static constexpr std::size_t kCheckpointIntervalBytes = 4096;
    static constexpr std::size_t kMaxPendingRecordings = 32;

    explicit RecordingStore(RecordingBackend& backend) : backend_(backend) {}

    bool Begin(uint32_t recording_id);
    bool Append(const uint8_t* data, std::size_t size);
    bool Checkpoint();
    bool Commit();
    void Abort();
    RecordingRecoveryMetrics RecoverPending();

    RecordingStoreState State() const { return state_; }
    uint32_t ActiveId() const { return recording_id_; }
    std::size_t BytesWritten() const { return bytes_written_; }

private:
    RecordingBackend& backend_;
    RecordingStoreState state_{RecordingStoreState::kIdle};
    uint32_t recording_id_{0};
    std::size_t bytes_written_{0};
    std::size_t checkpoint_bytes_{0};
};

class RecordingStoragePortAdapter final : public StoragePort {
public:
    explicit RecordingStoragePortAdapter(RecordingStore& store) : store_(store) {}

    bool OpenRecording() override {
        if (!store_.Begin(next_recording_id_)) {
            return false;
        }
        ++next_recording_id_;
        return true;
    }
    bool CommitRecording() override { return store_.Commit(); }
    void AbortRecording() override { store_.Abort(); }
    bool AppendAudio(const uint8_t* data, std::size_t size) { return store_.Append(data, size); }
    void SetNextRecordingId(uint32_t recording_id) { next_recording_id_ = recording_id; }

private:
    RecordingStore& store_;
    uint32_t next_recording_id_{1};
};

}  // namespace hi_yaka

#endif  // HI_YAKA_RECORDING_STORE_H_
