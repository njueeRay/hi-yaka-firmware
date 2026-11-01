#ifndef HI_YAKA_PRODUCT_TYPES_H_
#define HI_YAKA_PRODUCT_TYPES_H_

#include <cstddef>
#include <cstdint>

namespace hi_yaka {

enum class PowerPhase : uint8_t {
    kStarting,
    kRunning,
    kPreparingShutdown,
    kWaitingForPowerOff,
    kRecovering,
};

enum class ForegroundMode : uint8_t {
    kBoot,
    kHome,
    kProvisioning,
    kDialogue,
    kRecording,
    kNfc,
    kSettings,
    kUpdate,
    kError,
};

enum class Connectivity : uint8_t {
    kOffline,
    kProvisioning,
    kConnecting,
    kOnline,
    kServiceUnavailable,
};

enum class DialoguePhase : uint8_t {
    kInactive,
    kConnecting,
    kListening,
    kProcessing,
    kSpeaking,
    kInterrupted,
    kFailed,
};

enum class UpdatePhase : uint8_t {
    kInactive,
    kAwaitingConfirmation,
    kDownloading,
    kPendingReboot,
    kFailed,
};

enum class CapabilityId : uint8_t {
    kDisplay,
    kInput,
    kAudioCapture,
    kAudioPlayback,
    kStorage,
    kNetwork,
    kNfc,
    kImu,
    kFeedback,
    kPower,
    kUpdate,
    kCount,
};

enum class CapabilityState : uint8_t {
    kUnavailable,
    kInitializing,
    kReady,
    kDegraded,
    kFailed,
};

enum class ErrorCode : uint16_t {
    kNone = 0,
    kEventQueueFull = 1001,
    kEventExpired = 1002,
    kInvalidTransition = 1101,
    kOperationTimedOut = 1102,
    kCapabilityUnavailable = 1201,
    kCapabilityFailed = 1202,
    kPortRejected = 1203,
    kNetworkUnavailable = 2001,
    kAudioUnavailable = 3001,
    kStorageUnavailable = 4001,
    kNfcUnavailable = 5001,
    kUpdateUnavailable = 6001,
};

enum class EventSource : uint8_t {
    kSystem,
    kInput,
    kNetwork,
    kAudio,
    kStorage,
    kNfc,
    kImu,
    kPower,
    kUpdate,
    kTest,
};

enum class ProductEventType : uint8_t {
    kBootCompleted,
    kProvisioningRequested,
    kNetworkConnecting,
    kNetworkOnline,
    kNetworkOffline,
    kDialogueRequested,
    kDialogueListening,
    kDialogueProcessing,
    kDialogueSpeaking,
    kDialogueEnded,
    kRecordingRequested,
    kRecordingStopped,
    kNfcRequested,
    kNfcCompleted,
    kSettingsRequested,
    kUpdateRequested,
    kUpdateConfirmed,
    kUpdateCompleted,
    kCancel,
    kOperationTimedOut,
    kCapabilityChanged,
    kRecover,
    kShutdownRequested,
    kShutdownReady,
};

struct ProductEvent {
    ProductEventType type{ProductEventType::kBootCompleted};
    EventSource source{EventSource::kSystem};
    uint64_t timestamp_ms{0};
    uint64_t deadline_ms{0};
    uint32_t correlation_id{0};
    CapabilityId capability{CapabilityId::kCount};
    CapabilityState capability_state{CapabilityState::kUnavailable};
    ErrorCode error{ErrorCode::kNone};
};

struct ProductSnapshot {
    PowerPhase power{PowerPhase::kStarting};
    ForegroundMode foreground{ForegroundMode::kBoot};
    Connectivity connectivity{Connectivity::kOffline};
    DialoguePhase dialogue{DialoguePhase::kInactive};
    UpdatePhase update{UpdatePhase::kInactive};
    ErrorCode active_error{ErrorCode::kNone};
    uint32_t generation{0};
};

struct AudioBlockDescriptor {
    const int16_t* samples{nullptr};
    std::size_t sample_count{0};
    uint64_t timestamp_us{0};
    uint32_t sequence{0};
};

enum class StorageWriteKind : uint8_t {
    kAudioBlock,
    kCheckpoint,
    kCommit,
    kAbort,
};

struct StorageWriteRequest {
    StorageWriteKind kind{StorageWriteKind::kAudioBlock};
    AudioBlockDescriptor audio{};
    uint32_t recording_id{0};
};

constexpr std::size_t kCapabilityCount = static_cast<std::size_t>(CapabilityId::kCount);
constexpr std::size_t kProductEventQueueCapacity = 32;
constexpr std::size_t kAudioBlockQueueCapacity = 12;
constexpr std::size_t kStorageWriteQueueCapacity = 8;

}  // namespace hi_yaka

#endif  // HI_YAKA_PRODUCT_TYPES_H_
