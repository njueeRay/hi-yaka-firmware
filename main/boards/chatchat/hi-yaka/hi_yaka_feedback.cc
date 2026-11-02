#include "hi_yaka_feedback.h"

#include "application.h"
#include "config.h"

#include <algorithm>
#include <esp_log.h>

namespace {
constexpr char kTag[] = "HiYaKaFeedback";
constexpr uint8_t kMaximumChannel = 8;
constexpr uint32_t kMaximumMotorPulseMs = 120;
}  // namespace

HiYaKaFeedback::HiYaKaFeedback(HiYaKaPca9557& expander) : expander_(expander) {}

HiYaKaFeedback::~HiYaKaFeedback() {
    if (motor_timer_ != nullptr) {
        esp_timer_stop(motor_timer_);
        esp_timer_delete(motor_timer_);
    }
    if (status_rgb_ != nullptr) {
        led_strip_del(status_rgb_);
    }
    if (led_array_ != nullptr) {
        led_strip_del(led_array_);
    }
}

esp_err_t HiYaKaFeedback::Initialize() {
    led_strip_rmt_config_t rmt_config = {};
    rmt_config.resolution_hz = 10 * 1000 * 1000;

    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = YAKA_STATUS_RGB_GPIO;
    strip_config.max_leds = 1;
    strip_config.led_model = LED_MODEL_WS2812;
    strip_config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB;
    esp_err_t result = led_strip_new_rmt_device(&strip_config, &rmt_config, &status_rgb_);
    if (result != ESP_OK) {
        return result;
    }

    strip_config.strip_gpio_num = YAKA_LED_ARRAY_GPIO;
    strip_config.max_leds = YAKA_LED_ARRAY_COUNT;
    result = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_array_);
    if (result != ESP_OK) {
        return result;
    }
    led_strip_clear(status_rgb_);
    led_strip_clear(led_array_);
    expander_.SetOutputBit(YAKA_PCA9557_MOTOR_BIT, false);

    esp_timer_create_args_t timer_config = {};
    timer_config.callback = [](void* context) {
        auto* self = static_cast<HiYaKaFeedback*>(context);
        self->expander_.SetOutputBit(YAKA_PCA9557_MOTOR_BIT, false);
    };
    timer_config.arg = this;
    timer_config.dispatch_method = ESP_TIMER_TASK;
    timer_config.name = "hi_yaka_motor";
    timer_config.skip_unhandled_events = true;
    result = esp_timer_create(&timer_config, &motor_timer_);
    if (result == ESP_OK) {
        ESP_LOGI(kTag, "feedback safe-off verified; RGB channel limit=%u", kMaximumChannel);
    }
    return result;
}

void HiYaKaFeedback::SetColor(uint8_t red, uint8_t green, uint8_t blue) {
    red = std::min(red, kMaximumChannel);
    green = std::min(green, kMaximumChannel);
    blue = std::min(blue, kMaximumChannel);
    if (status_rgb_ != nullptr) {
        led_strip_set_pixel(status_rgb_, 0, red, green, blue);
        led_strip_refresh(status_rgb_);
    }
    if (led_array_ != nullptr) {
        for (uint16_t index = 0; index < YAKA_LED_ARRAY_COUNT; ++index) {
            led_strip_set_pixel(led_array_, index, red, green, blue);
        }
        led_strip_refresh(led_array_);
    }
}

void HiYaKaFeedback::PulseMotor(uint32_t duration_ms) {
    if (motor_timer_ == nullptr) {
        return;
    }
    duration_ms = std::min(duration_ms, kMaximumMotorPulseMs);
    esp_timer_stop(motor_timer_);
    if (expander_.SetOutputBit(YAKA_PCA9557_MOTOR_BIT, true) == ESP_OK) {
        esp_timer_start_once(motor_timer_, duration_ms * 1000U);
    }
}

void HiYaKaFeedback::OnStateChanged() {
    switch (Application::GetInstance().GetDeviceState()) {
        case kDeviceStateStarting:
        case kDeviceStateWifiConfiguring:
        case kDeviceStateConnecting:
            SetColor(0, 0, 4);
            break;
        case kDeviceStateListening:
        case kDeviceStateAudioTesting:
            SetColor(6, 1, 0);
            break;
        case kDeviceStateSpeaking:
        case kDeviceStateNotifying:
            SetColor(0, 5, 0);
            break;
        case kDeviceStateUpgrading:
            SetColor(4, 4, 0);
            break;
        case kDeviceStateIdle:
        default:
            SetColor(0, 0, 0);
            break;
    }
}

void HiYaKaFeedback::Emit(hi_yaka::FeedbackSignal signal) {
    switch (signal) {
        case hi_yaka::FeedbackSignal::kAccepted:
            SetColor(0, 3, 3);
            PulseMotor(35);
            break;
        case hi_yaka::FeedbackSignal::kCompleted:
            SetColor(0, 5, 0);
            PulseMotor(55);
            break;
        case hi_yaka::FeedbackSignal::kCancelled:
            SetColor(2, 2, 2);
            break;
        case hi_yaka::FeedbackSignal::kWarning:
            SetColor(6, 3, 0);
            PulseMotor(80);
            break;
        case hi_yaka::FeedbackSignal::kError:
            SetColor(8, 0, 0);
            PulseMotor(kMaximumMotorPulseMs);
            break;
    }
}
