#include "product_controller.h"

#include <algorithm>

namespace hi_yaka {

ProductController::ProductController(ProductPorts ports, CapabilityRegistry& capabilities)
    : ports_(ports), capabilities_(capabilities) {
    Publish();
}

QueuePushResult ProductController::Post(const ProductEvent& event) {
    return events_.TryPush(event);
}

ProcessResult ProductController::ProcessNext(uint64_t now_ms) {
    ProductEvent event;
    if (!events_.TryPop(event)) {
        return ProcessResult::kNoEvent;
    }
    if (event.deadline_ms != 0 && now_ms > event.deadline_ms) {
        ++metrics_.expired_events;
        metrics_.last_error = ErrorCode::kEventExpired;
        return ProcessResult::kExpired;
    }
    if (event.correlation_id != 0 && IsDuplicateCorrelation(event.correlation_id)) {
        ++metrics_.duplicate_events;
        return ProcessResult::kIgnoredDuplicate;
    }

    const auto result = Handle(event);
    RememberCorrelation(event.correlation_id);
    if (result == HandleResult::kChanged) {
        ++metrics_.handled_events;
        ++snapshot_.generation;
        Publish();
        return ProcessResult::kHandled;
    }
    if (result == HandleResult::kIgnored) {
        ++metrics_.duplicate_events;
        return ProcessResult::kIgnoredDuplicate;
    }
    ++metrics_.rejected_events;
    if (snapshot_.active_error != ErrorCode::kNone) {
        ++snapshot_.generation;
        Publish();
    }
    return ProcessResult::kRejected;
}

std::size_t ProductController::ProcessAll(uint64_t now_ms, std::size_t limit) {
    std::size_t processed = 0;
    while (processed < limit && ProcessNext(now_ms) != ProcessResult::kNoEvent) {
        ++processed;
    }
    return processed;
}

ProductController::HandleResult ProductController::Handle(const ProductEvent& event) {
    switch (event.type) {
        case ProductEventType::kBootCompleted:
            if (snapshot_.power != PowerPhase::kStarting) {
                return HandleResult::kIgnored;
            }
            snapshot_.power = PowerPhase::kRunning;
            snapshot_.foreground = ForegroundMode::kHome;
            snapshot_.active_error = ErrorCode::kNone;
            return HandleResult::kChanged;

        case ProductEventType::kProvisioningRequested:
            if (snapshot_.foreground == ForegroundMode::kProvisioning) {
                return HandleResult::kIgnored;
            }
            if (snapshot_.foreground != ForegroundMode::kHome &&
                snapshot_.foreground != ForegroundMode::kSettings) {
                EnterError(ErrorCode::kInvalidTransition);
                return HandleResult::kRejected;
            }
            if (!Require(CapabilityId::kNetwork, ErrorCode::kNetworkUnavailable)) {
                return HandleResult::kRejected;
            }
            if (ports_.network == nullptr || !ports_.network->BeginProvisioning()) {
                EnterError(ErrorCode::kPortRejected);
                return HandleResult::kRejected;
            }
            snapshot_.foreground = ForegroundMode::kProvisioning;
            snapshot_.connectivity = Connectivity::kProvisioning;
            return HandleResult::kChanged;

        case ProductEventType::kNetworkConnecting:
            if (snapshot_.connectivity == Connectivity::kConnecting) {
                return HandleResult::kIgnored;
            }
            snapshot_.connectivity = Connectivity::kConnecting;
            return HandleResult::kChanged;

        case ProductEventType::kNetworkOnline:
            if (snapshot_.connectivity == Connectivity::kOnline) {
                return HandleResult::kIgnored;
            }
            snapshot_.connectivity = Connectivity::kOnline;
            if (snapshot_.foreground == ForegroundMode::kProvisioning) {
                snapshot_.foreground = ForegroundMode::kHome;
            } else if (snapshot_.foreground == ForegroundMode::kDialogue &&
                       snapshot_.dialogue == DialoguePhase::kInterrupted) {
                snapshot_.dialogue = DialoguePhase::kConnecting;
            }
            return HandleResult::kChanged;

        case ProductEventType::kNetworkOffline:
            if (snapshot_.connectivity == Connectivity::kOffline) {
                return HandleResult::kIgnored;
            }
            snapshot_.connectivity = Connectivity::kOffline;
            if (snapshot_.foreground == ForegroundMode::kDialogue) {
                if (ports_.audio != nullptr) {
                    ports_.audio->InterruptDialogueOutput();
                }
                snapshot_.dialogue = DialoguePhase::kInterrupted;
            }
            return HandleResult::kChanged;

        case ProductEventType::kDialogueRequested:
            return RequestDialogue();
        case ProductEventType::kDialogueListening:
            if (snapshot_.foreground != ForegroundMode::kDialogue ||
                snapshot_.dialogue == DialoguePhase::kListening) {
                return HandleResult::kIgnored;
            }
            snapshot_.dialogue = DialoguePhase::kListening;
            return HandleResult::kChanged;
        case ProductEventType::kDialogueProcessing:
            if (snapshot_.foreground != ForegroundMode::kDialogue ||
                snapshot_.dialogue == DialoguePhase::kProcessing) {
                return HandleResult::kIgnored;
            }
            snapshot_.dialogue = DialoguePhase::kProcessing;
            return HandleResult::kChanged;
        case ProductEventType::kDialogueSpeaking:
            if (snapshot_.foreground != ForegroundMode::kDialogue ||
                snapshot_.dialogue == DialoguePhase::kSpeaking) {
                return HandleResult::kIgnored;
            }
            snapshot_.dialogue = DialoguePhase::kSpeaking;
            return HandleResult::kChanged;
        case ProductEventType::kDialogueInterrupted:
            if (snapshot_.foreground != ForegroundMode::kDialogue ||
                snapshot_.dialogue == DialoguePhase::kInterrupted) {
                return HandleResult::kIgnored;
            }
            if (ports_.audio != nullptr) {
                ports_.audio->InterruptDialogueOutput();
            }
            snapshot_.dialogue = DialoguePhase::kInterrupted;
            return HandleResult::kChanged;
        case ProductEventType::kDialogueEnded:
            if (snapshot_.foreground != ForegroundMode::kDialogue) {
                return HandleResult::kIgnored;
            }
            ReturnHome(FeedbackSignal::kCompleted);
            return HandleResult::kChanged;

        case ProductEventType::kRecordingRequested:
            return RequestRecording();
        case ProductEventType::kRecordingStopped:
            if (snapshot_.foreground != ForegroundMode::kRecording) {
                return HandleResult::kIgnored;
            }
            if (ports_.audio != nullptr) {
                ports_.audio->StopRecordingCapture();
            }
            if (ports_.storage == nullptr || !ports_.storage->CommitRecording()) {
                EnterError(ErrorCode::kStorageUnavailable);
                return HandleResult::kRejected;
            }
            ReturnHome(FeedbackSignal::kCompleted);
            return HandleResult::kChanged;

        case ProductEventType::kNfcRequested:
            return RequestNfc();
        case ProductEventType::kNfcCompleted:
            if (snapshot_.foreground != ForegroundMode::kNfc) {
                return HandleResult::kIgnored;
            }
            ReturnHome(FeedbackSignal::kCompleted);
            return HandleResult::kChanged;

        case ProductEventType::kSettingsRequested:
            if (snapshot_.foreground == ForegroundMode::kSettings) {
                return HandleResult::kIgnored;
            }
            if (snapshot_.foreground != ForegroundMode::kHome) {
                EnterError(ErrorCode::kInvalidTransition);
                return HandleResult::kRejected;
            }
            snapshot_.foreground = ForegroundMode::kSettings;
            return HandleResult::kChanged;

        case ProductEventType::kUpdateRequested:
            return RequestUpdate();
        case ProductEventType::kUpdateConfirmed:
            return ConfirmUpdate();
        case ProductEventType::kUpdateCompleted:
            if (snapshot_.foreground != ForegroundMode::kUpdate) {
                return HandleResult::kIgnored;
            }
            snapshot_.update = UpdatePhase::kPendingReboot;
            if (ports_.feedback != nullptr) {
                ports_.feedback->Emit(FeedbackSignal::kCompleted);
            }
            return HandleResult::kChanged;

        case ProductEventType::kCancel:
            return CancelActiveMode();
        case ProductEventType::kOperationTimedOut:
            CancelActiveMode();
            EnterError(event.error == ErrorCode::kNone ? ErrorCode::kOperationTimedOut : event.error);
            return HandleResult::kChanged;
        case ProductEventType::kCapabilityChanged:
            return HandleCapabilityChange(event);
        case ProductEventType::kRecover:
            if (snapshot_.foreground != ForegroundMode::kError) {
                return HandleResult::kIgnored;
            }
            snapshot_.power = PowerPhase::kRecovering;
            ReturnHome(FeedbackSignal::kCompleted);
            snapshot_.power = PowerPhase::kRunning;
            return HandleResult::kChanged;
        case ProductEventType::kShutdownRequested:
            return RequestShutdown();
        case ProductEventType::kShutdownReady:
            if (snapshot_.power != PowerPhase::kPreparingShutdown) {
                return HandleResult::kIgnored;
            }
            snapshot_.power = PowerPhase::kWaitingForPowerOff;
            return HandleResult::kChanged;
    }
    EnterError(ErrorCode::kInvalidTransition);
    return HandleResult::kRejected;
}

ProductController::HandleResult ProductController::RequestDialogue() {
    if (snapshot_.foreground == ForegroundMode::kDialogue) {
        return HandleResult::kIgnored;
    }
    if (snapshot_.foreground != ForegroundMode::kHome ||
        snapshot_.connectivity != Connectivity::kOnline) {
        EnterError(ErrorCode::kInvalidTransition);
        return HandleResult::kRejected;
    }
    if (!Require(CapabilityId::kNetwork, ErrorCode::kNetworkUnavailable) ||
        !Require(CapabilityId::kAudioCapture, ErrorCode::kAudioUnavailable) ||
        !Require(CapabilityId::kAudioPlayback, ErrorCode::kAudioUnavailable)) {
        return HandleResult::kRejected;
    }
    if (ports_.network == nullptr || ports_.audio == nullptr ||
        !ports_.network->BeginDialogueSession()) {
        EnterError(ErrorCode::kPortRejected);
        return HandleResult::kRejected;
    }
    if (!ports_.audio->BeginDialogue()) {
        ports_.network->CancelOperation();
        EnterError(ErrorCode::kPortRejected);
        return HandleResult::kRejected;
    }
    snapshot_.foreground = ForegroundMode::kDialogue;
    snapshot_.dialogue = DialoguePhase::kConnecting;
    return HandleResult::kChanged;
}

ProductController::HandleResult ProductController::RequestRecording() {
    if (snapshot_.foreground == ForegroundMode::kRecording) {
        return HandleResult::kIgnored;
    }
    if (snapshot_.foreground != ForegroundMode::kHome) {
        EnterError(ErrorCode::kInvalidTransition);
        return HandleResult::kRejected;
    }
    if (!Require(CapabilityId::kStorage, ErrorCode::kStorageUnavailable) ||
        !Require(CapabilityId::kAudioCapture, ErrorCode::kAudioUnavailable)) {
        return HandleResult::kRejected;
    }
    if (ports_.storage == nullptr || ports_.audio == nullptr ||
        !ports_.storage->OpenRecording() || !ports_.audio->StartRecordingCapture()) {
        if (ports_.storage != nullptr) {
            ports_.storage->AbortRecording();
        }
        EnterError(ErrorCode::kPortRejected);
        return HandleResult::kRejected;
    }
    snapshot_.foreground = ForegroundMode::kRecording;
    return HandleResult::kChanged;
}

ProductController::HandleResult ProductController::RequestNfc() {
    if (snapshot_.foreground == ForegroundMode::kNfc) {
        return HandleResult::kIgnored;
    }
    if (snapshot_.foreground != ForegroundMode::kHome) {
        EnterError(ErrorCode::kInvalidTransition);
        return HandleResult::kRejected;
    }
    if (!Require(CapabilityId::kNfc, ErrorCode::kNfcUnavailable)) {
        return HandleResult::kRejected;
    }
    if (ports_.nfc == nullptr || !ports_.nfc->BeginScan()) {
        EnterError(ErrorCode::kPortRejected);
        return HandleResult::kRejected;
    }
    snapshot_.foreground = ForegroundMode::kNfc;
    return HandleResult::kChanged;
}

ProductController::HandleResult ProductController::RequestUpdate() {
    if (snapshot_.foreground == ForegroundMode::kUpdate) {
        return HandleResult::kIgnored;
    }
    if (snapshot_.foreground != ForegroundMode::kHome &&
        snapshot_.foreground != ForegroundMode::kSettings) {
        EnterError(ErrorCode::kInvalidTransition);
        return HandleResult::kRejected;
    }
    if (!Require(CapabilityId::kUpdate, ErrorCode::kUpdateUnavailable) ||
        !Require(CapabilityId::kNetwork, ErrorCode::kNetworkUnavailable)) {
        return HandleResult::kRejected;
    }
    snapshot_.foreground = ForegroundMode::kUpdate;
    snapshot_.update = UpdatePhase::kAwaitingConfirmation;
    return HandleResult::kChanged;
}

ProductController::HandleResult ProductController::ConfirmUpdate() {
    if (snapshot_.foreground != ForegroundMode::kUpdate ||
        snapshot_.update != UpdatePhase::kAwaitingConfirmation) {
        return HandleResult::kIgnored;
    }
    if (ports_.update == nullptr || !ports_.update->BeginUpdate()) {
        snapshot_.update = UpdatePhase::kFailed;
        EnterError(ErrorCode::kPortRejected);
        return HandleResult::kRejected;
    }
    snapshot_.update = UpdatePhase::kDownloading;
    return HandleResult::kChanged;
}

ProductController::HandleResult ProductController::CancelActiveMode() {
    switch (snapshot_.foreground) {
        case ForegroundMode::kDialogue:
            if (ports_.audio != nullptr) {
                ports_.audio->CancelDialogue();
            }
            if (ports_.network != nullptr) {
                ports_.network->CancelOperation();
            }
            break;
        case ForegroundMode::kRecording:
            if (ports_.audio != nullptr) {
                ports_.audio->StopRecordingCapture();
            }
            if (ports_.storage != nullptr) {
                ports_.storage->AbortRecording();
            }
            break;
        case ForegroundMode::kNfc:
            if (ports_.nfc != nullptr) {
                ports_.nfc->CancelScan();
            }
            break;
        case ForegroundMode::kUpdate:
            if (ports_.update != nullptr) {
                ports_.update->CancelUpdate();
            }
            break;
        case ForegroundMode::kProvisioning:
            if (ports_.network != nullptr) {
                ports_.network->CancelOperation();
            }
            snapshot_.connectivity = Connectivity::kOffline;
            break;
        case ForegroundMode::kSettings:
        case ForegroundMode::kError:
            break;
        case ForegroundMode::kBoot:
        case ForegroundMode::kHome:
            return HandleResult::kIgnored;
    }
    ReturnHome(FeedbackSignal::kCancelled);
    return HandleResult::kChanged;
}

ProductController::HandleResult ProductController::HandleCapabilityChange(const ProductEvent& event) {
    if (event.capability == CapabilityId::kCount) {
        EnterError(ErrorCode::kInvalidTransition);
        return HandleResult::kRejected;
    }
    const bool changed = capabilities_.Update(event.capability, event.capability_state, event.error,
                                              event.timestamp_ms);
    if (!changed) {
        return HandleResult::kIgnored;
    }
    if ((event.capability_state == CapabilityState::kFailed ||
         event.capability_state == CapabilityState::kUnavailable) &&
        IsCriticalForCurrentMode(event.capability)) {
        ++metrics_.capability_failures;
        EnterError(event.error == ErrorCode::kNone ? ErrorCode::kCapabilityFailed : event.error);
    }
    return HandleResult::kChanged;
}

ProductController::HandleResult ProductController::RequestShutdown() {
    if (snapshot_.power == PowerPhase::kPreparingShutdown ||
        snapshot_.power == PowerPhase::kWaitingForPowerOff) {
        return HandleResult::kIgnored;
    }
    CancelActiveMode();
    if (ports_.power == nullptr || !ports_.power->PrepareShutdown()) {
        EnterError(ErrorCode::kPortRejected);
        return HandleResult::kRejected;
    }
    snapshot_.power = PowerPhase::kPreparingShutdown;
    snapshot_.foreground = ForegroundMode::kHome;
    return HandleResult::kChanged;
}

bool ProductController::Require(CapabilityId id, ErrorCode error) {
    if (capabilities_.IsUsable(id)) {
        return true;
    }
    ++metrics_.capability_failures;
    EnterError(error);
    return false;
}

bool ProductController::IsCriticalForCurrentMode(CapabilityId id) const {
    switch (snapshot_.foreground) {
        case ForegroundMode::kDialogue:
            return id == CapabilityId::kNetwork || id == CapabilityId::kAudioCapture ||
                   id == CapabilityId::kAudioPlayback;
        case ForegroundMode::kRecording:
            return id == CapabilityId::kStorage || id == CapabilityId::kAudioCapture;
        case ForegroundMode::kNfc:
            return id == CapabilityId::kNfc;
        case ForegroundMode::kUpdate:
            return id == CapabilityId::kUpdate || id == CapabilityId::kNetwork;
        case ForegroundMode::kProvisioning:
            return id == CapabilityId::kNetwork;
        default:
            return false;
    }
}

bool ProductController::IsDuplicateCorrelation(uint32_t correlation_id) const {
    return std::find(recent_correlations_.begin(), recent_correlations_.end(), correlation_id) !=
           recent_correlations_.end();
}

void ProductController::RememberCorrelation(uint32_t correlation_id) {
    if (correlation_id == 0) {
        return;
    }
    recent_correlations_[next_correlation_slot_] = correlation_id;
    next_correlation_slot_ = (next_correlation_slot_ + 1) % recent_correlations_.size();
}

void ProductController::EnterError(ErrorCode error) {
    snapshot_.foreground = ForegroundMode::kError;
    snapshot_.active_error = error;
    metrics_.last_error = error;
    if (ports_.feedback != nullptr) {
        ports_.feedback->Emit(FeedbackSignal::kError);
    }
}

void ProductController::ReturnHome(FeedbackSignal signal) {
    snapshot_.foreground = ForegroundMode::kHome;
    snapshot_.dialogue = DialoguePhase::kInactive;
    snapshot_.update = UpdatePhase::kInactive;
    snapshot_.active_error = ErrorCode::kNone;
    if (ports_.feedback != nullptr) {
        ports_.feedback->Emit(signal);
    }
}

void ProductController::Publish() {
    if (ports_.display != nullptr) {
        ports_.display->Present(snapshot_);
    }
}

}  // namespace hi_yaka
