#include "runtime_adapter.h"

#include "application.h"
#include "board.h"
#include "hi_yaka_board_api.h"
#include "wifi_board.h"

#include <algorithm>

#include <esp_log.h>
#include <esp_timer.h>

namespace hi_yaka {
namespace {
constexpr char kTag[] = "HiYaKaRuntime";
constexpr UBaseType_t kRecordingQueueCapacity = 12;
}  // namespace

RuntimeAdapter& RuntimeAdapter::Get() {
    static RuntimeAdapter instance;
    return instance;
}

uint64_t RuntimeAdapter::NowMs() const {
    return static_cast<uint64_t>(esp_timer_get_time() / 1000);
}

void RuntimeAdapter::Initialize() {
    if (initialized_.exchange(true)) {
        return;
    }

    recording_queue_ = xQueueCreate(kRecordingQueueCapacity, sizeof(PcmBlock));
    if (recording_queue_ != nullptr) {
        xTaskCreate(WriterTask, "hi_yaka_writer", 4096, this, 3, &writer_task_);
    }

    ProductPorts ports{};
    ports.display = this;
    ports.input = this;
    ports.audio = this;
    ports.storage = this;
    ports.network = this;
    ports.nfc = this;
    ports.imu = this;
    ports.feedback = BoardFeedbackPort();
    ports.power = this;
    ports.update = this;
    controller_ = new ProductController(ports, capabilities_);

    for (std::size_t index = 0; index < kCapabilityCount; ++index) {
        const auto capability = static_cast<CapabilityId>(index);
        const bool ready = BoardCapabilityReady(capability);
        CapabilityState state = ready ? CapabilityState::kReady
                                      : CapabilityState::kUnavailable;
        if (capability == CapabilityId::kNetwork) {
            state = CapabilityState::kInitializing;
        }
        capabilities_.Update(capability, state, ErrorCode::kNone, NowMs());
    }

    Application::GetInstance().GetAudioService().SetInputPcmCallback(
        [this](const int16_t* samples, std::size_t count, int channels, int sample_rate) {
            CapturePcm(samples, count, channels, sample_rate);
        });
    Post(ProductEventType::kBootCompleted);
    ESP_LOGI(kTag, "product runtime bound to application and physical board ports");
}

void RuntimeAdapter::Post(ProductEventType type, EventSource source) {
    if (controller_ == nullptr) {
        return;
    }
    ProductEvent event{};
    event.type = type;
    event.source = source;
    event.timestamp_ms = NowMs();
    event.deadline_ms = event.timestamp_ms + 1000;
    event.correlation_id = ++next_correlation_id_;
    if (controller_->Post(event) != QueuePushResult::kAccepted) {
        ESP_LOGE(kTag, "product event queue full type=%u", static_cast<unsigned>(type));
        return;
    }
    controller_->ProcessAll(event.timestamp_ms);
}

void RuntimeAdapter::Present(const ProductSnapshot& snapshot) {
    pages_.Present(snapshot);
    ESP_LOGI(kTag,
             "snapshot generation=%lu power=%u foreground=%u connectivity=%u dialogue=%u error=%u",
             static_cast<unsigned long>(snapshot.generation),
             static_cast<unsigned>(snapshot.power), static_cast<unsigned>(snapshot.foreground),
             static_cast<unsigned>(snapshot.connectivity),
             static_cast<unsigned>(snapshot.dialogue),
             static_cast<unsigned>(snapshot.active_error));
}

void RuntimeAdapter::SetEnabled(bool enabled) { SetBoardInputEnabled(enabled); }

bool RuntimeAdapter::BeginDialogue() {
    if (!BoardCapabilityReady(CapabilityId::kAudioCapture) ||
        !BoardCapabilityReady(CapabilityId::kAudioPlayback)) {
        return false;
    }
    Application::GetInstance().StartListening();
    return true;
}

void RuntimeAdapter::InterruptDialogueOutput() {
    Application::GetInstance().AbortSpeaking(kAbortReasonNone);
}

void RuntimeAdapter::CancelDialogue() { Application::GetInstance().StopListening(); }

bool RuntimeAdapter::StartRecordingCapture() {
    if (recording_queue_ == nullptr || BoardRecordingPort() == nullptr ||
        !BoardCapabilityReady(CapabilityId::kAudioCapture)) {
        return false;
    }
    xQueueReset(recording_queue_);
    dropped_blocks_.store(0);
    recording_fault_.store(false);
    recording_active_.store(true);
    Application::GetInstance().GetAudioService().EnableLocalRecording(true);
    return true;
}

void RuntimeAdapter::StopRecordingCapture() {
    Application::GetInstance().GetAudioService().EnableLocalRecording(false);
    recording_active_.store(false);
    const int64_t deadline = esp_timer_get_time() + 500 * 1000;
    while ((uxQueueMessagesWaiting(recording_queue_) != 0 || writer_busy_.load()) &&
           esp_timer_get_time() < deadline) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (uxQueueMessagesWaiting(recording_queue_) != 0 || writer_busy_.load()) {
        recording_fault_.store(true);
    }
    ESP_LOGI(kTag, "recording capture stopped: dropped_blocks=%lu queued=%u",
             static_cast<unsigned long>(dropped_blocks_.load()),
             static_cast<unsigned>(uxQueueMessagesWaiting(recording_queue_)));
}

void RuntimeAdapter::CapturePcm(const int16_t* samples, std::size_t sample_count, int channels,
                                int sample_rate) {
    if (!recording_active_.load() || recording_queue_ == nullptr || samples == nullptr ||
        channels <= 0 || sample_rate != 16000) {
        return;
    }
    const std::size_t frames = sample_count / static_cast<std::size_t>(channels);
    for (std::size_t offset = 0; offset < frames; offset += 160) {
        PcmBlock block{};
        block.sample_count = static_cast<uint16_t>(std::min<std::size_t>(160, frames - offset));
        for (std::size_t index = 0; index < block.sample_count; ++index) {
            block.samples[index] = samples[(offset + index) * channels];
        }
        if (xQueueSend(recording_queue_, &block, 0) != pdTRUE) {
            ++dropped_blocks_;
            recording_fault_.store(true);
            break;
        }
    }
}

void RuntimeAdapter::WriterTask(void* context) {
    static_cast<RuntimeAdapter*>(context)->RunWriter();
}

void RuntimeAdapter::RunWriter() {
    PcmBlock block{};
    while (true) {
        if (xQueueReceive(recording_queue_, &block, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        writer_busy_.store(true);
        auto* port = BoardRecordingPort();
        if (port == nullptr ||
            !port->AppendAudio(reinterpret_cast<const uint8_t*>(block.samples),
                               block.sample_count * sizeof(int16_t))) {
            ++dropped_blocks_;
            recording_fault_.store(true);
        }
        writer_busy_.store(false);
    }
}

bool RuntimeAdapter::OpenRecording() {
    auto* port = BoardRecordingPort();
    if (port == nullptr) {
        return false;
    }
    recording_fault_.store(false);
    dropped_blocks_.store(0);
    return port->OpenRecording();
}

bool RuntimeAdapter::CommitRecording() {
    auto* port = BoardRecordingPort();
    const bool drained = recording_queue_ != nullptr &&
                         uxQueueMessagesWaiting(recording_queue_) == 0 &&
                         !writer_busy_.load() && !recording_active_.load();
    if (port == nullptr || !drained || recording_fault_.load() ||
        dropped_blocks_.load() != 0) {
        if (port != nullptr) {
            port->AbortRecording();
        }
        ESP_LOGE(kTag,
                 "recording commit rejected: drained=%d fault=%d dropped_blocks=%lu",
                 drained, recording_fault_.load(),
                 static_cast<unsigned long>(dropped_blocks_.load()));
        return false;
    }
    return port->CommitRecording();
}

void RuntimeAdapter::AbortRecording() {
    recording_active_.store(false);
    Application::GetInstance().GetAudioService().EnableLocalRecording(false);
    if (recording_queue_ != nullptr) {
        xQueueReset(recording_queue_);
    }
    auto* port = BoardRecordingPort();
    if (port != nullptr) {
        port->AbortRecording();
    }
}

bool RuntimeAdapter::BeginProvisioning() {
    static_cast<WifiBoard&>(Board::GetInstance()).EnterWifiConfigMode();
    return true;
}

bool RuntimeAdapter::BeginDialogueSession() {
    return capabilities_.IsUsable(CapabilityId::kNetwork);
}

void RuntimeAdapter::CancelOperation() {}
bool RuntimeAdapter::BeginScan() { return false; }
void RuntimeAdapter::CancelScan() {}
void RuntimeAdapter::SetMonitoring(bool) {}
bool RuntimeAdapter::PrepareShutdown() { return true; }
bool RuntimeAdapter::BeginUpdate() { return false; }
void RuntimeAdapter::CancelUpdate() {}

void RuntimeAdapter::ApplyAction(ProductAction action) {
    switch (action) {
        case ProductAction::kFocusPrevious:
            pages_.MoveFocus(-1);
            break;
        case ProductAction::kFocusNext:
            pages_.MoveFocus(1);
            break;
        case ProductAction::kActivate:
            ApplyAction(pages_.Activate());
            break;
        case ProductAction::kBack:
            ApplyAction(pages_.Back());
            break;
        case ProductAction::kStartDialogue:
            Post(ProductEventType::kDialogueRequested, EventSource::kInput);
            break;
        case ProductAction::kStartRecording:
            Post(ProductEventType::kRecordingRequested, EventSource::kInput);
            break;
        case ProductAction::kStopRecording:
            Post(ProductEventType::kRecordingStopped, EventSource::kInput);
            break;
        case ProductAction::kOpenNfc:
            Post(ProductEventType::kNfcRequested, EventSource::kInput);
            break;
        case ProductAction::kOpenSettings:
            Post(ProductEventType::kSettingsRequested, EventSource::kInput);
            break;
        case ProductAction::kConfirmUpdate:
            Post(ProductEventType::kUpdateConfirmed, EventSource::kInput);
            break;
        case ProductAction::kCancel:
            Post(ProductEventType::kCancel, EventSource::kInput);
            break;
        case ProductAction::kNone:
            break;
    }
}

void RuntimeAdapter::OnPhysicalInput(PhysicalKey key, KeyGesture gesture) {
    if (!initialized_.load() || gesture != KeyGesture::kPress) {
        return;
    }
    switch (key) {
        case PhysicalKey::kUp:
        case PhysicalKey::kLeft:
            ApplyAction(ProductAction::kFocusPrevious);
            break;
        case PhysicalKey::kDown:
        case PhysicalKey::kRight:
            ApplyAction(ProductAction::kFocusNext);
            break;
        case PhysicalKey::kConfirm:
            ApplyAction(ProductAction::kActivate);
            break;
        case PhysicalKey::kBack:
            ApplyAction(ProductAction::kBack);
            break;
        case PhysicalKey::kPrimary:
        case PhysicalKey::kSecondary:
            // Product semantics for the two action keys are intentionally not
            // frozen before the PRD decision.
            ESP_LOGI(kTag, "action key ignored until binding is approved: %u",
                     static_cast<unsigned>(key));
            break;
    }
}

void RuntimeAdapter::OnNetworkEvent(NetworkEvent event) {
    if (!initialized_.load()) {
        return;
    }
    switch (event) {
        case NetworkEvent::Scanning:
        case NetworkEvent::Connecting:
            Post(ProductEventType::kNetworkConnecting, EventSource::kNetwork);
            break;
        case NetworkEvent::Connected:
            capabilities_.Update(CapabilityId::kNetwork, CapabilityState::kReady,
                                 ErrorCode::kNone, NowMs());
            Post(ProductEventType::kNetworkOnline, EventSource::kNetwork);
            break;
        case NetworkEvent::Disconnected:
            capabilities_.Update(CapabilityId::kNetwork, CapabilityState::kDegraded,
                                 ErrorCode::kNetworkUnavailable, NowMs());
            Post(ProductEventType::kNetworkOffline, EventSource::kNetwork);
            break;
        default:
            break;
    }
}

void RuntimeAdapter::OnDeviceState(DeviceState state) {
    if (!initialized_.load()) {
        return;
    }
    switch (state) {
        case kDeviceStateListening:
            Post(ProductEventType::kDialogueListening, EventSource::kAudio);
            break;
        case kDeviceStateSpeaking:
            Post(ProductEventType::kDialogueSpeaking, EventSource::kAudio);
            break;
        case kDeviceStateIdle:
            if (controller_ != nullptr &&
                controller_->Snapshot().foreground == ForegroundMode::kDialogue) {
                Post(ProductEventType::kDialogueEnded, EventSource::kAudio);
            }
            break;
        default:
            break;
    }
}

}  // namespace hi_yaka
