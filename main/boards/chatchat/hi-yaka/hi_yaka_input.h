#ifndef HI_YAKA_INPUT_H_
#define HI_YAKA_INPUT_H_

#include "hi_yaka_adc.h"
#include "business_flows.h"

#include <array>
#include <functional>

#include <esp_timer.h>

class HiYaKaInput {
public:
    using Callback =
        std::function<void(hi_yaka::PhysicalKey key, hi_yaka::KeyGesture gesture)>;

    explicit HiYaKaInput(HiYaKaAdc& adc);
    ~HiYaKaInput();

    esp_err_t Initialize(Callback callback);
    void SetEnabled(bool enabled) { enabled_ = enabled; }

private:
    struct KeyState {
        bool stable_down = false;
        bool candidate_down = false;
        uint8_t consecutive_samples = 0;
        int64_t pressed_at_us = 0;
    };

    void Poll();
    void Observe(std::size_t index, bool down, int64_t now_us);
    int DecodeLadderKey(int millivolts) const;

    HiYaKaAdc& adc_;
    Callback callback_;
    esp_timer_handle_t timer_ = nullptr;
    std::array<KeyState, 8> keys_{};
    bool enabled_ = true;
#if CONFIG_HI_YAKA_BRINGUP_DIAGNOSTICS
    int last_ladder_mv_ = -1;
    int last_key_a_level_ = -1;
    int last_key_b_level_ = -1;
#endif
};

#endif  // HI_YAKA_INPUT_H_
