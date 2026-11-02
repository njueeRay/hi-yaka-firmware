#ifndef HI_YAKA_RUNTIME_ADAPTER_H_
#define HI_YAKA_RUNTIME_ADAPTER_H_

#include "business_flows.h"
#include "board.h"
#include "capability_registry.h"
#include "device_state.h"
#include "product_controller.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

namespace hi_yaka {

class RuntimeAdapter final : public DisplayPort,
                             public InputPort,
                             public AudioPort,
                             public StoragePort,
                             public NetworkPort,
                             public NfcPort,
                             public ImuPort,
                             public PowerPort,
                             public UpdatePort {
public:
    static RuntimeAdapter& Get();

    void Initialize();
    void OnPhysicalInput(PhysicalKey key, KeyGesture gesture);
    void OnNetworkEvent(NetworkEvent event);
    void OnDeviceState(DeviceState state);

    void Present(const ProductSnapshot& snapshot) override;
    void SetEnabled(bool enabled) override;
    bool BeginDialogue() override;
    void InterruptDialogueOutput() override;
    void CancelDialogue() override;
    bool StartRecordingCapture() override;
    void StopRecordingCapture() override;
    bool OpenRecording() override;
    bool CommitRecording() override;
    void AbortRecording() override;
    bool BeginProvisioning() override;
    bool BeginDialogueSession() override;
    void CancelOperation() override;
    bool BeginScan() override;
    void CancelScan() override;
    void SetMonitoring(bool enabled) override;
    bool PrepareShutdown() override;
    bool BeginUpdate() override;
    void CancelUpdate() override;

private:
    struct PcmBlock {
        uint16_t sample_count = 0;
        int16_t samples[160]{};
    };

    RuntimeAdapter() = default;
    static void WriterTask(void* context);
    void RunWriter();
    void CapturePcm(const int16_t* samples, std::size_t sample_count, int channels,
                    int sample_rate);
    void Post(ProductEventType type, EventSource source = EventSource::kSystem);
    void ApplyAction(ProductAction action);
    uint64_t NowMs() const;

    CapabilityRegistry capabilities_;
    ProductController* controller_ = nullptr;
    PageFlowModel pages_;
    QueueHandle_t recording_queue_ = nullptr;
    TaskHandle_t writer_task_ = nullptr;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> recording_active_{false};
    std::atomic<bool> writer_busy_{false};
    std::atomic<bool> recording_fault_{false};
    std::atomic<uint32_t> dropped_blocks_{0};
    uint32_t next_correlation_id_ = 0x48000000U;
};

}  // namespace hi_yaka

#endif  // HI_YAKA_RUNTIME_ADAPTER_H_
