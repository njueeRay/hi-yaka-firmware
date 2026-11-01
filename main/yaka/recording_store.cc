#include "recording_store.h"

namespace hi_yaka {

bool RecordingStore::Begin(uint32_t recording_id) {
    if (state_ != RecordingStoreState::kIdle || recording_id == 0 ||
        !backend_.CreatePartial(recording_id)) {
        return false;
    }
    if (!backend_.WriteCheckpoint(recording_id, 0)) {
        backend_.Discard(recording_id);
        return false;
    }
    state_ = RecordingStoreState::kOpen;
    recording_id_ = recording_id;
    bytes_written_ = 0;
    checkpoint_bytes_ = 0;
    return true;
}

bool RecordingStore::Append(const uint8_t* data, std::size_t size) {
    if (state_ != RecordingStoreState::kOpen || (data == nullptr && size != 0)) {
        return false;
    }
    if (size == 0) {
        return true;
    }
    if (!backend_.Append(recording_id_, data, size)) {
        state_ = RecordingStoreState::kFaulted;
        return false;
    }
    bytes_written_ += size;
    if (bytes_written_ - checkpoint_bytes_ >= kCheckpointIntervalBytes && !Checkpoint()) {
        state_ = RecordingStoreState::kFaulted;
        return false;
    }
    return true;
}

bool RecordingStore::Checkpoint() {
    if (state_ != RecordingStoreState::kOpen || !backend_.Sync(recording_id_) ||
        !backend_.WriteCheckpoint(recording_id_, bytes_written_)) {
        return false;
    }
    checkpoint_bytes_ = bytes_written_;
    return true;
}

bool RecordingStore::Commit() {
    if (state_ != RecordingStoreState::kOpen || !Checkpoint() ||
        !backend_.Finalize(recording_id_, false)) {
        state_ = RecordingStoreState::kFaulted;
        return false;
    }
    backend_.RemoveCheckpoint(recording_id_);
    state_ = RecordingStoreState::kIdle;
    recording_id_ = 0;
    bytes_written_ = 0;
    checkpoint_bytes_ = 0;
    return true;
}

void RecordingStore::Abort() {
    if (state_ != RecordingStoreState::kIdle) {
        backend_.Discard(recording_id_);
    }
    state_ = RecordingStoreState::kIdle;
    recording_id_ = 0;
    bytes_written_ = 0;
    checkpoint_bytes_ = 0;
}

RecordingRecoveryMetrics RecordingStore::RecoverPending() {
    RecordingRecoveryMetrics metrics;
    std::array<uint32_t, kMaxPendingRecordings> ids{};
    const std::size_t count = backend_.ListPending(ids.data(), ids.size());
    for (std::size_t i = 0; i < count; ++i) {
        std::size_t partial_size = 0;
        std::size_t checkpoint_size = 0;
        if (!backend_.PartialSize(ids[i], partial_size) ||
            !backend_.ReadCheckpoint(ids[i], checkpoint_size)) {
            backend_.Discard(ids[i]);
            ++metrics.discarded;
            continue;
        }
        if (checkpoint_size == 0 || checkpoint_size > partial_size) {
            backend_.Discard(ids[i]);
            ++metrics.discarded;
            continue;
        }
        if ((partial_size > checkpoint_size && !backend_.Truncate(ids[i], checkpoint_size)) ||
            !backend_.Sync(ids[i]) || !backend_.Finalize(ids[i], true)) {
            ++metrics.failed;
            continue;
        }
        backend_.RemoveCheckpoint(ids[i]);
        ++metrics.recovered;
    }
    return metrics;
}

}  // namespace hi_yaka
