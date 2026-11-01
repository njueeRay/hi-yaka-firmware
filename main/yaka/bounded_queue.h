#ifndef HI_YAKA_BOUNDED_QUEUE_H_
#define HI_YAKA_BOUNDED_QUEUE_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace hi_yaka {

enum class QueueFullPolicy : uint8_t {
    kRejectNewest,
    kDropOldest,
};

enum class QueuePushResult : uint8_t {
    kAccepted,
    kAcceptedAfterDroppingOldest,
    kRejectedFull,
};

struct QueueMetrics {
    std::size_t depth{0};
    std::size_t peak_depth{0};
    uint64_t accepted{0};
    uint64_t popped{0};
    uint64_t rejected{0};
    uint64_t dropped_oldest{0};
};

template <typename T, std::size_t Capacity>
class BoundedQueue {
    static_assert(Capacity > 0, "A bounded queue must have non-zero capacity");

public:
    explicit BoundedQueue(QueueFullPolicy full_policy) : full_policy_(full_policy) {}

    QueuePushResult TryPush(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        QueuePushResult result = QueuePushResult::kAccepted;
        if (size_ == Capacity) {
            if (full_policy_ == QueueFullPolicy::kRejectNewest) {
                ++metrics_.rejected;
                return QueuePushResult::kRejectedFull;
            }
            head_ = Next(head_);
            --size_;
            ++metrics_.dropped_oldest;
            result = QueuePushResult::kAcceptedAfterDroppingOldest;
        }

        storage_[tail_] = item;
        tail_ = Next(tail_);
        ++size_;
        ++metrics_.accepted;
        metrics_.depth = size_;
        if (size_ > metrics_.peak_depth) {
            metrics_.peak_depth = size_;
        }
        return result;
    }

    bool TryPop(T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (size_ == 0) {
            return false;
        }
        item = storage_[head_];
        head_ = Next(head_);
        --size_;
        ++metrics_.popped;
        metrics_.depth = size_;
        return true;
    }

    QueueMetrics Metrics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return metrics_;
    }

    constexpr std::size_t CapacityValue() const { return Capacity; }

private:
    static constexpr std::size_t Next(std::size_t index) { return (index + 1) % Capacity; }

    const QueueFullPolicy full_policy_;
    mutable std::mutex mutex_;
    std::array<T, Capacity> storage_{};
    std::size_t head_{0};
    std::size_t tail_{0};
    std::size_t size_{0};
    QueueMetrics metrics_{};
};

}  // namespace hi_yaka

#endif  // HI_YAKA_BOUNDED_QUEUE_H_
