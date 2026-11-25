#include "hi_yaka_input.h"

#include "config.h"

#include <driver/gpio.h>
#include <esp_log.h>
#include <utility>

namespace {

constexpr char kTag[] = "HiYaKaInput";
constexpr int64_t kLongPressUs = 1200 * 1000;
constexpr uint8_t kDebounceSamples = 3;

struct LadderWindow {
    int minimum_mv;
    int maximum_mv;
};

// Schematic-derived nominal centers: 0/82/185/326/478/721 mV. Midpoint
// windows are intentionally provisional; every board/batch needs an L1
// histogram before release thresholds can be frozen.
constexpr std::array<LadderWindow, 6> kLadderWindows{{
    {0, 41}, {42, 133}, {134, 255}, {256, 402}, {403, 599}, {600, 1100},
}};

constexpr std::array<hi_yaka::PhysicalKey, 8> kPhysicalKeys{{
    hi_yaka::PhysicalKey::kUp,
    hi_yaka::PhysicalKey::kDown,
    hi_yaka::PhysicalKey::kLeft,
    hi_yaka::PhysicalKey::kRight,
    hi_yaka::PhysicalKey::kConfirm,
    hi_yaka::PhysicalKey::kBack,
    hi_yaka::PhysicalKey::kPrimary,
    hi_yaka::PhysicalKey::kSecondary,
}};

}  // namespace

HiYaKaInput::HiYaKaInput(HiYaKaAdc& adc) : adc_(adc) {}

HiYaKaInput::~HiYaKaInput() {
    if (timer_ != nullptr) {
        esp_timer_stop(timer_);
        esp_timer_delete(timer_);
    }
}

esp_err_t HiYaKaInput::Initialize(Callback callback) {
    callback_ = std::move(callback);
    gpio_config_t input_config = {};
    input_config.pin_bit_mask = (1ULL << YAKA_KEY_A_GPIO) | (1ULL << YAKA_KEY_B_GPIO);
    input_config.mode = GPIO_MODE_INPUT;
    input_config.pull_up_en = GPIO_PULLUP_ENABLE;
    input_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    input_config.intr_type = GPIO_INTR_DISABLE;
    esp_err_t result = gpio_config(&input_config);
    if (result != ESP_OK) {
        return result;
    }

    esp_timer_create_args_t timer_config = {};
    timer_config.callback = [](void* context) { static_cast<HiYaKaInput*>(context)->Poll(); };
    timer_config.arg = this;
    timer_config.dispatch_method = ESP_TIMER_TASK;
    timer_config.name = "hi_yaka_input";
    timer_config.skip_unhandled_events = true;
    result = esp_timer_create(&timer_config, &timer_);
    if (result == ESP_OK) {
        result = esp_timer_start_periodic(timer_, 20 * 1000);
    }
    if (result == ESP_OK) {
        ESP_LOGW(kTag, "ADC key windows are provisional until L1 histogram calibration");
    }
    return result;
}

int HiYaKaInput::DecodeLadderKey(int millivolts) const {
    for (std::size_t index = 0; index < kLadderWindows.size(); ++index) {
        if (millivolts >= kLadderWindows[index].minimum_mv &&
            millivolts <= kLadderWindows[index].maximum_mv) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

void HiYaKaInput::Observe(std::size_t index, bool down, int64_t now_us) {
    auto& state = keys_[index];
    if (down != state.candidate_down) {
        state.candidate_down = down;
        state.consecutive_samples = 1;
        return;
    }
    if (state.consecutive_samples < kDebounceSamples) {
        ++state.consecutive_samples;
    }
    if (state.consecutive_samples < kDebounceSamples || down == state.stable_down) {
        return;
    }

    state.stable_down = down;
    if (down) {
        state.pressed_at_us = now_us;
        return;
    }
    const int64_t duration = now_us - state.pressed_at_us;
    state.pressed_at_us = 0;
    if (enabled_ && callback_) {
        callback_(kPhysicalKeys[index], duration >= kLongPressUs
                                            ? hi_yaka::KeyGesture::kLongPress
                                            : hi_yaka::KeyGesture::kPress);
    }
}

void HiYaKaInput::Poll() {
    const int64_t now_us = esp_timer_get_time();
    int ladder_mv = 0;
    const int active_ladder =
        adc_.ReadMillivolts(YAKA_DPAD_ADC_CHANNEL, &ladder_mv) == ESP_OK
            ? DecodeLadderKey(ladder_mv)
            : -1;
    for (std::size_t index = 0; index < 6; ++index) {
        Observe(index, static_cast<int>(index) == active_ladder, now_us);
    }
    // External inversion makes the two direct key signals active-low at the MCU.
    const int key_a_level = gpio_get_level(YAKA_KEY_A_GPIO);
    const int key_b_level = gpio_get_level(YAKA_KEY_B_GPIO);
    Observe(6, key_a_level == 0, now_us);
    Observe(7, key_b_level == 0, now_us);
#if CONFIG_HI_YAKA_BRINGUP_DIAGNOSTICS
    const int ladder_delta = last_ladder_mv_ < 0
                                 ? 1000
                                 : (ladder_mv > last_ladder_mv_
                                        ? ladder_mv - last_ladder_mv_
                                        : last_ladder_mv_ - ladder_mv);
    if (ladder_delta >= 50 || key_a_level != last_key_a_level_ ||
        key_b_level != last_key_b_level_) {
        ESP_LOGI(kTag,
                 "BRINGUP input raw: ladder=%d mV decoded=%d key_a=%d key_b=%d",
                 ladder_mv, active_ladder, key_a_level, key_b_level);
        last_ladder_mv_ = ladder_mv;
        last_key_a_level_ = key_a_level;
        last_key_b_level_ = key_b_level;
    }
#endif
}
