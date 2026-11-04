#ifndef HI_YAKA_FEEDBACK_H_
#define HI_YAKA_FEEDBACK_H_

#include "hi_yaka_pca9557.h"
#include "led/led.h"
#include "product_ports.h"

#include <esp_timer.h>
#include <led_strip.h>

class HiYaKaFeedback final : public Led, public hi_yaka::FeedbackPort {
public:
    explicit HiYaKaFeedback(HiYaKaPca9557& expander);
    ~HiYaKaFeedback() override;

    esp_err_t Initialize();
    void RunBringupSelfTest();
    void OnStateChanged() override;
    void Emit(hi_yaka::FeedbackSignal signal) override;

private:
    void SetColor(uint8_t red, uint8_t green, uint8_t blue);
    void PulseMotor(uint32_t duration_ms);

    HiYaKaPca9557& expander_;
    led_strip_handle_t status_rgb_ = nullptr;
    led_strip_handle_t led_array_ = nullptr;
    esp_timer_handle_t motor_timer_ = nullptr;
};

#endif  // HI_YAKA_FEEDBACK_H_
