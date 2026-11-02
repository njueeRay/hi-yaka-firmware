#ifndef HI_YAKA_ADC_H_
#define HI_YAKA_ADC_H_

#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_err.h>

class HiYaKaAdc {
public:
    HiYaKaAdc();
    ~HiYaKaAdc();

    esp_err_t Initialize();
    esp_err_t ReadMillivolts(adc_channel_t channel, int* millivolts);
    bool calibrated() const { return calibration_ != nullptr; }
    bool ready() const { return unit_ != nullptr; }

private:
    adc_oneshot_unit_handle_t unit_ = nullptr;
    adc_cali_handle_t calibration_ = nullptr;
};

#endif  // HI_YAKA_ADC_H_
