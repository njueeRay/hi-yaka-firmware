#ifndef HI_YAKA_BUSINESS_FLOWS_H_
#define HI_YAKA_BUSINESS_FLOWS_H_

#include <array>
#include <cstddef>
#include <cstdint>

#include "product_types.h"

namespace hi_yaka {

enum class PhysicalKey : uint8_t {
    kUp,
    kDown,
    kLeft,
    kRight,
    kConfirm,
    kBack,
    kPrimary,
    kSecondary,
};

enum class KeyGesture : uint8_t {
    kPress,
    kLongPress,
};

enum class ProductAction : uint8_t {
    kNone,
    kFocusPrevious,
    kFocusNext,
    kActivate,
    kBack,
    kStartDialogue,
    kStartRecording,
    kStopRecording,
    kOpenNfc,
    kOpenSettings,
    kConfirmUpdate,
    kCancel,
};

struct InputBinding {
    PhysicalKey key{PhysicalKey::kConfirm};
    KeyGesture gesture{KeyGesture::kPress};
    ForegroundMode context{ForegroundMode::kHome};
    ProductAction action{ProductAction::kNone};
};

class InputActionMap {
public:
    static constexpr std::size_t kMaxBindings = 32;

    bool Configure(const InputBinding* bindings, std::size_t count);
    ProductAction Resolve(PhysicalKey key, KeyGesture gesture, ForegroundMode context) const;
    std::size_t Size() const { return count_; }

private:
    std::array<InputBinding, kMaxBindings> bindings_{};
    std::size_t count_{0};
};

enum class PageId : uint8_t {
    kBoot,
    kHome,
    kProvisioning,
    kDialogue,
    kRecording,
    kNfc,
    kSettings,
    kUpdate,
    kError,
    kShutdown,
};

struct PageViewModel {
    static constexpr uint16_t kWidth = 200;
    static constexpr uint16_t kHeight = 320;

    PageId page{PageId::kBoot};
    uint8_t focus_index{0};
    uint8_t focus_count{1};
    bool busy{true};
    ErrorCode error{ErrorCode::kNone};
};

class PageFlowModel {
public:
    void Present(const ProductSnapshot& snapshot);
    void MoveFocus(int direction);
    ProductAction Activate() const;
    ProductAction Back() const;
    PageViewModel View() const { return view_; }

private:
    static PageId ResolvePage(const ProductSnapshot& snapshot);
    static uint8_t FocusCount(PageId page);

    PageViewModel view_{};
};

class DialogueDeadlineTracker {
public:
    void Observe(const ProductSnapshot& snapshot, uint64_t now_ms);
    bool Poll(uint64_t now_ms, ProductEvent& timeout_event);
    void Reset();

    uint64_t DeadlineMs() const { return deadline_ms_; }

private:
    static uint32_t TimeoutFor(DialoguePhase phase);

    DialoguePhase phase_{DialoguePhase::kInactive};
    uint64_t deadline_ms_{0};
    uint32_t next_correlation_id_{0xD1000000U};
};

}  // namespace hi_yaka

#endif  // HI_YAKA_BUSINESS_FLOWS_H_
