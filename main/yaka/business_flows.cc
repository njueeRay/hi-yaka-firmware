#include "business_flows.h"

#include <algorithm>

namespace hi_yaka {

bool InputActionMap::Configure(const InputBinding* bindings, std::size_t count) {
    if ((bindings == nullptr && count != 0) || count > bindings_.size()) {
        return false;
    }
    for (std::size_t i = 0; i < count; ++i) {
        for (std::size_t j = i + 1; j < count; ++j) {
            if (bindings[i].key == bindings[j].key &&
                bindings[i].gesture == bindings[j].gesture &&
                bindings[i].context == bindings[j].context) {
                return false;
            }
        }
    }
    if (count != 0) {
        std::copy(bindings, bindings + count, bindings_.begin());
    }
    count_ = count;
    return true;
}

ProductAction InputActionMap::Resolve(PhysicalKey key, KeyGesture gesture,
                                      ForegroundMode context) const {
    for (std::size_t i = 0; i < count_; ++i) {
        const auto& binding = bindings_[i];
        if (binding.key == key && binding.gesture == gesture && binding.context == context) {
            return binding.action;
        }
    }
    return ProductAction::kNone;
}

void PageFlowModel::Present(const ProductSnapshot& snapshot) {
    const PageId page = ResolvePage(snapshot);
    if (page != view_.page) {
        view_.page = page;
        view_.focus_index = 0;
    }
    view_.focus_count = FocusCount(page);
    if (view_.focus_index >= view_.focus_count) {
        view_.focus_index = 0;
    }
    view_.busy = page == PageId::kBoot || page == PageId::kProvisioning ||
                 page == PageId::kDialogue || page == PageId::kRecording ||
                 page == PageId::kNfc || page == PageId::kUpdate ||
                 page == PageId::kShutdown;
    view_.error = snapshot.active_error;
}

void PageFlowModel::MoveFocus(int direction) {
    if (view_.focus_count <= 1 || direction == 0) {
        return;
    }
    if (direction > 0) {
        view_.focus_index = static_cast<uint8_t>((view_.focus_index + 1U) % view_.focus_count);
    } else {
        view_.focus_index = static_cast<uint8_t>(
            (view_.focus_index + view_.focus_count - 1U) % view_.focus_count);
    }
}

ProductAction PageFlowModel::Activate() const {
    if (view_.page == PageId::kHome) {
        constexpr std::array<ProductAction, 4> kHomeActions{
            ProductAction::kStartDialogue,
            ProductAction::kStartRecording,
            ProductAction::kOpenNfc,
            ProductAction::kOpenSettings,
        };
        return kHomeActions[view_.focus_index];
    }
    if (view_.page == PageId::kUpdate) {
        return view_.focus_index == 0 ? ProductAction::kConfirmUpdate : ProductAction::kCancel;
    }
    if (view_.page == PageId::kError) {
        return view_.focus_index == 0 ? ProductAction::kBack : ProductAction::kCancel;
    }
    return ProductAction::kNone;
}

ProductAction PageFlowModel::Back() const {
    return view_.page == PageId::kHome || view_.page == PageId::kBoot
               ? ProductAction::kNone
               : ProductAction::kBack;
}

PageId PageFlowModel::ResolvePage(const ProductSnapshot& snapshot) {
    if (snapshot.power == PowerPhase::kPreparingShutdown ||
        snapshot.power == PowerPhase::kWaitingForPowerOff) {
        return PageId::kShutdown;
    }
    switch (snapshot.foreground) {
        case ForegroundMode::kBoot:
            return PageId::kBoot;
        case ForegroundMode::kHome:
            return PageId::kHome;
        case ForegroundMode::kProvisioning:
            return PageId::kProvisioning;
        case ForegroundMode::kDialogue:
            return PageId::kDialogue;
        case ForegroundMode::kRecording:
            return PageId::kRecording;
        case ForegroundMode::kNfc:
            return PageId::kNfc;
        case ForegroundMode::kSettings:
            return PageId::kSettings;
        case ForegroundMode::kUpdate:
            return PageId::kUpdate;
        case ForegroundMode::kError:
            return PageId::kError;
    }
    return PageId::kError;
}

uint8_t PageFlowModel::FocusCount(PageId page) {
    switch (page) {
        case PageId::kHome:
            return 4;
        case PageId::kSettings:
            return 5;
        case PageId::kUpdate:
        case PageId::kError:
            return 2;
        default:
            return 1;
    }
}

void DialogueDeadlineTracker::Observe(const ProductSnapshot& snapshot, uint64_t now_ms) {
    if (snapshot.foreground != ForegroundMode::kDialogue ||
        snapshot.dialogue == DialoguePhase::kInactive ||
        snapshot.dialogue == DialoguePhase::kInterrupted ||
        snapshot.dialogue == DialoguePhase::kFailed) {
        Reset();
        return;
    }
    if (snapshot.dialogue != phase_) {
        phase_ = snapshot.dialogue;
        deadline_ms_ = now_ms + TimeoutFor(phase_);
    }
}

bool DialogueDeadlineTracker::Poll(uint64_t now_ms, ProductEvent& timeout_event) {
    if (deadline_ms_ == 0 || now_ms <= deadline_ms_) {
        return false;
    }
    timeout_event = ProductEvent{};
    timeout_event.type = ProductEventType::kOperationTimedOut;
    timeout_event.source = EventSource::kSystem;
    timeout_event.timestamp_ms = now_ms;
    timeout_event.correlation_id = ++next_correlation_id_;
    timeout_event.error = ErrorCode::kOperationTimedOut;
    Reset();
    return true;
}

void DialogueDeadlineTracker::Reset() {
    phase_ = DialoguePhase::kInactive;
    deadline_ms_ = 0;
}

uint32_t DialogueDeadlineTracker::TimeoutFor(DialoguePhase phase) {
    switch (phase) {
        case DialoguePhase::kConnecting:
            return 15000;
        case DialoguePhase::kListening:
            return 30000;
        case DialoguePhase::kProcessing:
            return 45000;
        case DialoguePhase::kSpeaking:
            return 120000;
        default:
            return 0;
    }
}

}  // namespace hi_yaka
