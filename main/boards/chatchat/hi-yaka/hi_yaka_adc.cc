#include "hi_yaka_adc.h"

#include "config.h"

#include <esp_adc/adc_cali_scheme.h>
#include <esp_log.h>

namespace {
constexpr char kTag[] = "HiYaKaAdc";
constexpr adc_atten_t kAttenuation = ADC_ATTEN_DB_12;
}  // namespace

HiYaKaAdc::HiYaKaAdc() = default;

HiYaKaAdc::~HiYaKaAdc() {
    if (calibration_ != nullptr) {
        adc_cali_delete_scheme_curve_fitting(calibration_);
    }
    if (unit_ != nullptr) {
        adc_oneshot_del_unit(unit_);
    }
}

esp_err_t HiYaKaAdc::Initialize() {
    adc_oneshot_unit_init_cfg_t unit_config = {};
    unit_config.unit_id = YAKA_DPAD_ADC_UNIT;
    esp_err_t result = adc_oneshot_new_unit(&unit_config, &unit_);
    if (result != ESP_OK) {
        return result;
    }

    adc_oneshot_chan_cfg_t channel_config = {};
    channel_config.atten = kAttenuation;
    channel_config.bitwidth = ADC_BITWIDTH_DEFAULT;
    result = adc_oneshot_config_channel(unit_, YAKA_DPAD_ADC_CHANNEL, &channel_config);
    if (result == ESP_OK) {
        result = adc_oneshot_config_channel(unit_, YAKA_BATTERY_ADC_CHANNEL, &channel_config);
    }
    if (result != ESP_OK) {
        return result;
    }

    adc_cali_curve_fitting_config_t calibration_config = {};
    calibration_config.unit_id = YAKA_DPAD_ADC_UNIT;
    calibration_config.atten = kAttenuation;
    calibration_config.bitwidth = ADC_BITWIDTH_DEFAULT;
    const esp_err_t calibration_result =
        adc_cali_create_scheme_curve_fitting(&calibration_config, &calibration_);
    if (calibration_result != ESP_OK) {
        calibration_ = nullptr;
        ESP_LOGW(kTag, "ADC calibration unavailable (%s); raw fallback is L0 only",
                 esp_err_to_name(calibration_result));
    }
    return ESP_OK;
}

esp_err_t HiYaKaAdc::ReadMillivolts(adc_channel_t channel, int* millivolts) {
    if (unit_ == nullptr || millivolts == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    int raw = 0;
    esp_err_t result = adc_oneshot_read(unit_, channel, &raw);
    if (result != ESP_OK) {
        return result;
    }
    if (calibration_ != nullptr) {
        return adc_cali_raw_to_voltage(calibration_, raw, millivolts);
    }

    // Conservative engineering fallback for diagnostics only. Release builds
    // require calibrated physical-board histograms.
    constexpr int kAdcFullScale = 4095;
    constexpr int kApproximateFullScaleMillivolts = 3300;
    *millivolts = raw * kApproximateFullScaleMillivolts / kAdcFullScale;
    return ESP_OK;
}
