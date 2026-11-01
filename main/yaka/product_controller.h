#ifndef HI_YAKA_PRODUCT_CONTROLLER_H_
#define HI_YAKA_PRODUCT_CONTROLLER_H_

#include <array>
#include <cstddef>
#include <cstdint>

#include "bounded_queue.h"
#include "capability_registry.h"
#include "product_ports.h"
#include "product_types.h"

namespace hi_yaka {

using ProductEventQueue = BoundedQueue<ProductEvent, kProductEventQueueCapacity>;
using AudioBlockQueue = BoundedQueue<AudioBlockDescriptor, kAudioBlockQueueCapacity>;
using StorageWriteQueue = BoundedQueue<StorageWriteRequest, kStorageWriteQueueCapacity>;

enum class ProcessResult : uint8_t {
    kNoEvent,
    kHandled,
    kIgnoredDuplicate,
    kExpired,
    kRejected,
};

struct ControllerMetrics {
    uint64_t handled_events{0};
    uint64_t duplicate_events{0};
    uint64_t expired_events{0};
    uint64_t rejected_events{0};
    uint64_t capability_failures{0};
    ErrorCode last_error{ErrorCode::kNone};
};

class ProductController {
public:
    ProductController(ProductPorts ports, CapabilityRegistry& capabilities);

    QueuePushResult Post(const ProductEvent& event);
    ProcessResult ProcessNext(uint64_t now_ms);
    std::size_t ProcessAll(uint64_t now_ms, std::size_t limit = kProductEventQueueCapacity);

    ProductSnapshot Snapshot() const { return snapshot_; }
    ControllerMetrics Metrics() const { return metrics_; }
    QueueMetrics EventQueueMetrics() const { return events_.Metrics(); }

private:
    enum class HandleResult : uint8_t {
        kChanged,
        kIgnored,
        kRejected,
    };

    HandleResult Handle(const ProductEvent& event);
    HandleResult RequestDialogue();
    HandleResult RequestRecording();
    HandleResult RequestNfc();
    HandleResult RequestUpdate();
    HandleResult ConfirmUpdate();
    HandleResult CancelActiveMode();
    HandleResult HandleCapabilityChange(const ProductEvent& event);
    HandleResult RequestShutdown();

    bool Require(CapabilityId id, ErrorCode error);
    bool IsCriticalForCurrentMode(CapabilityId id) const;
    bool IsDuplicateCorrelation(uint32_t correlation_id) const;
    void RememberCorrelation(uint32_t correlation_id);
    void EnterError(ErrorCode error);
    void ReturnHome(FeedbackSignal signal);
    void Publish();

    ProductPorts ports_;
    CapabilityRegistry& capabilities_;
    ProductEventQueue events_{QueueFullPolicy::kRejectNewest};
    ProductSnapshot snapshot_{};
    ControllerMetrics metrics_{};
    std::array<uint32_t, 16> recent_correlations_{};
    std::size_t next_correlation_slot_{0};
};

}  // namespace hi_yaka

#endif  // HI_YAKA_PRODUCT_CONTROLLER_H_
