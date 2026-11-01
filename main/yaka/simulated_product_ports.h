#ifndef HI_YAKA_SIMULATED_PRODUCT_PORTS_H_
#define HI_YAKA_SIMULATED_PRODUCT_PORTS_H_

#include <array>

#include "product_ports.h"

namespace hi_yaka {

struct SimulatedPortCalls {
    uint32_t display_present{0};
    uint32_t input_enable{0};
    uint32_t dialogue_begin{0};
    uint32_t dialogue_interrupt{0};
    uint32_t dialogue_cancel{0};
    uint32_t recording_begin{0};
    uint32_t recording_stop{0};
    uint32_t storage_begin{0};
    uint32_t storage_commit{0};
    uint32_t storage_abort{0};
    uint32_t provisioning_begin{0};
    uint32_t session_begin{0};
    uint32_t network_cancel{0};
    uint32_t nfc_begin{0};
    uint32_t nfc_cancel{0};
    uint32_t imu_monitor{0};
    uint32_t feedback_emit{0};
    uint32_t feedback_rgb{0};
    uint32_t feedback_led_array{0};
    uint32_t feedback_vibration{0};
    uint32_t feedback_tone{0};
    uint32_t shutdown_prepare{0};
    uint32_t update_begin{0};
    uint32_t update_cancel{0};
};

class SimulatedProductPorts final : public DisplayPort,
                                    public InputPort,
                                    public AudioPort,
                                    public StoragePort,
                                    public NetworkPort,
                                    public NfcPort,
                                    public ImuPort,
                                    public FeedbackPort,
                                    public PowerPort,
                                    public UpdatePort {
public:
    ProductPorts Bind() {
        return ProductPorts{this, this, this, this, this, this, this, this, this, this};
    }

    void Present(const ProductSnapshot& snapshot) override {
        ++calls.display_present;
        last_snapshot = snapshot;
    }
    void SetEnabled(bool enabled) override {
        ++calls.input_enable;
        input_enabled = enabled;
    }
    bool BeginDialogue() override {
        ++calls.dialogue_begin;
        return accept_audio;
    }
    void InterruptDialogueOutput() override { ++calls.dialogue_interrupt; }
    void CancelDialogue() override { ++calls.dialogue_cancel; }
    bool StartRecordingCapture() override {
        ++calls.recording_begin;
        return accept_audio;
    }
    void StopRecordingCapture() override { ++calls.recording_stop; }
    bool OpenRecording() override {
        ++calls.storage_begin;
        return accept_storage;
    }
    bool CommitRecording() override {
        ++calls.storage_commit;
        return accept_storage;
    }
    void AbortRecording() override { ++calls.storage_abort; }
    bool BeginProvisioning() override {
        ++calls.provisioning_begin;
        return accept_network;
    }
    bool BeginDialogueSession() override {
        ++calls.session_begin;
        return accept_network;
    }
    void CancelOperation() override { ++calls.network_cancel; }
    bool BeginScan() override {
        ++calls.nfc_begin;
        return accept_nfc;
    }
    void CancelScan() override { ++calls.nfc_cancel; }
    void SetMonitoring(bool enabled) override {
        ++calls.imu_monitor;
        imu_enabled = enabled;
    }
    void Emit(FeedbackSignal signal) override {
        ++calls.feedback_emit;
        last_feedback = signal;
        if (feedback_channels[static_cast<std::size_t>(FeedbackChannel::kRgb)]) {
            ++calls.feedback_rgb;
        }
        if (feedback_channels[static_cast<std::size_t>(FeedbackChannel::kLedArray)]) {
            ++calls.feedback_led_array;
        }
        if (feedback_channels[static_cast<std::size_t>(FeedbackChannel::kVibration)]) {
            ++calls.feedback_vibration;
        }
        if (feedback_channels[static_cast<std::size_t>(FeedbackChannel::kTone)]) {
            ++calls.feedback_tone;
        }
    }
    void SetFeedbackChannelAvailable(FeedbackChannel channel, bool available) {
        if (channel != FeedbackChannel::kCount) {
            feedback_channels[static_cast<std::size_t>(channel)] = available;
        }
    }
    bool PrepareShutdown() override {
        ++calls.shutdown_prepare;
        return accept_power;
    }
    bool BeginUpdate() override {
        ++calls.update_begin;
        return accept_update;
    }
    void CancelUpdate() override { ++calls.update_cancel; }

    bool accept_audio{true};
    bool accept_storage{true};
    bool accept_network{true};
    bool accept_nfc{true};
    bool accept_power{true};
    bool accept_update{true};
    bool input_enabled{false};
    bool imu_enabled{false};
    ProductSnapshot last_snapshot{};
    FeedbackSignal last_feedback{FeedbackSignal::kAccepted};
    std::array<bool, static_cast<std::size_t>(FeedbackChannel::kCount)> feedback_channels{
        true, true, true, true};
    SimulatedPortCalls calls{};

};

}  // namespace hi_yaka

#endif  // HI_YAKA_SIMULATED_PRODUCT_PORTS_H_
