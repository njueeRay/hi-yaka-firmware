#include "wifi_board.h"
#include "backlight.h"
#include "application.h"
#include "codecs/box_audio_codec.h"
#include "codecs/dummy_audio_codec.h"
#include "config.h"
#include "display/lcd_display.h"
#include "hi_yaka_adc.h"
#include "hi_yaka_expander_lcd_io.h"
#include "hi_yaka_feedback.h"
#include "hi_yaka_input.h"
#include "hi_yaka_pca9557.h"
#include "hi_yaka_pn532.h"
#include "hi_yaka_qmi8658a.h"
#include "hi_yaka_storage.h"
#include "hi_yaka_wav_writer.h"
#include "recording_store.h"
#include "runtime_adapter.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <inttypes.h>
#include <memory>
#include <vector>

#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <esp_err.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>

#define TAG "HiYaKa"

#if !defined(CONFIG_IDF_TARGET_ESP32C6) || !CONFIG_IDF_TARGET_ESP32C6
#error "hi-yaka is defined only for ESP32-C6"
#endif

#if defined(CONFIG_SPIRAM) && CONFIG_SPIRAM
#error "ESP32-C6 does not support external PSRAM; hi-yaka must remain a no-PSRAM build"
#endif

namespace {

struct ExpectedI2cDevice {
    uint8_t address;
    const char* name;
};

constexpr std::array<ExpectedI2cDevice, 5> kExpectedI2cDevices = {{
    {YAKA_ES8311_I2C_ADDRESS, "ES8311"},
    {YAKA_PCA9557_I2C_ADDRESS, "PCA9557"},
    {YAKA_PN532_I2C_ADDRESS, "PN532"},
    {YAKA_ES7210_I2C_ADDRESS, "ES7210"},
    {YAKA_QMI8658A_I2C_ADDRESS, "QMI8658A"},
}};

const char* PhysicalKeyName(hi_yaka::PhysicalKey key) {
    constexpr std::array<const char*, 8> kNames = {{
        "UP", "DOWN", "LEFT", "RIGHT", "CONFIRM", "BACK", "KEY_A", "KEY_B",
    }};
    const auto index = static_cast<std::size_t>(key);
    return index < kNames.size() ? kNames[index] : "UNKNOWN";
}

const char* NfcPollResultName(HiYaKaNfcPollResult result) {
    switch (result) {
        case HiYaKaNfcPollResult::kFound:
            return "found";
        case HiYaKaNfcPollResult::kNoTarget:
            return "no-target";
        case HiYaKaNfcPollResult::kTimeout:
            return "timeout";
        case HiYaKaNfcPollResult::kTransportError:
            return "transport-error";
        case HiYaKaNfcPollResult::kProtocolError:
            return "protocol-error";
        case HiYaKaNfcPollResult::kNoMemory:
            return "no-memory";
        case HiYaKaNfcPollResult::kInvalidState:
            return "invalid-state";
    }
    return "unknown";
}

uint32_t NfcUidFingerprint(const HiYaKaNfcTag& tag) {
    // FNV-1a is deliberately non-reversible in normal logs. The full UID is
    // retained only in RAM for the future approved business mapping.
    uint32_t hash = 2166136261U;
    for (uint8_t index = 0; index < tag.uid_length; ++index) {
        hash = (hash ^ tag.uid[index]) * 16777619U;
    }
    return hash;
}

class ExpanderBacklight final : public Backlight {
public:
    explicit ExpanderBacklight(HiYaKaPca9557& expander) : expander_(expander) {}

protected:
    void SetBrightnessImpl(uint8_t brightness) override {
        // Hardware provides a binary backlight gate, not a PWM connection.
        const esp_err_t result = expander_.SetOutputBit(YAKA_PCA9557_LCD_BL_BIT,
                                                        brightness > 0);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "backlight update failed: %s", esp_err_to_name(result));
        }
    }

private:
    HiYaKaPca9557& expander_;
};

#if CONFIG_HI_YAKA_BRINGUP_DIAGNOSTICS
class HiYaKaDiagnosticAudioCodec final : public BoxAudioCodec {
public:
    using BoxAudioCodec::BoxAudioCodec;

    void OutputData(std::vector<int16_t>& data) override {
        BoxAudioCodec::OutputData(data);
        if (capture_complete_ || data.empty()) {
            return;
        }
        if (!capture_attempted_) {
            capture_attempted_ = true;
            if (!writer_.Open("/sdcard/diagnostics/decoded-output.wav",
                              output_sample_rate(), output_channels())) {
                capture_complete_ = true;
                return;
            }
        }
        constexpr uint32_t kMaximumSeconds = 30;
        const uint32_t maximum_bytes =
            output_sample_rate() * output_channels() * sizeof(int16_t) * kMaximumSeconds;
        const uint32_t remaining_bytes = maximum_bytes - writer_.data_bytes();
        const std::size_t samples_to_write =
            std::min<std::size_t>(data.size(), remaining_bytes / sizeof(int16_t));
        if (!writer_.Append(data.data(), samples_to_write) ||
            writer_.data_bytes() >= maximum_bytes) {
            writer_.Close();
            capture_complete_ = true;
        }
    }

    void EnableOutput(bool enable) override {
        BoxAudioCodec::EnableOutput(enable);
        if (!enable && writer_.open()) {
            writer_.Close();
            capture_complete_ = true;
        }
    }

private:
    HiYaKaWavWriter writer_;
    bool capture_attempted_ = false;
    bool capture_complete_ = false;
};
#endif

}  // namespace

class HiYaKaBoard : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    std::unique_ptr<HiYaKaPca9557> pca9557_;
    std::unique_ptr<SpiLcdDisplay> display_;
    std::unique_ptr<ExpanderBacklight> backlight_;
    std::unique_ptr<HiYaKaAdc> adc_;
    std::unique_ptr<HiYaKaInput> input_;
    std::unique_ptr<HiYaKaFeedback> feedback_;
    std::unique_ptr<BoxAudioCodec> audio_codec_;
    std::unique_ptr<HiYaKaStorage> storage_;
    std::unique_ptr<HiYaKaPn532> pn532_;
    std::unique_ptr<HiYaKaQmi8658a> qmi8658a_;
    std::unique_ptr<hi_yaka::RecordingStore> recording_store_;
    std::unique_ptr<hi_yaka::RecordingStoragePortAdapter> recording_port_;
    bool pca_safe_state_verified_ = false;
    std::atomic<bool> nfc_scan_running_{false};
    std::atomic<bool> nfc_scan_cancelled_{false};
    std::atomic<bool> imu_monitoring_{false};
    std::atomic<bool> imu_task_running_{false};

    void ForcePaDisabled() {
        gpio_config_t config = {};
        config.pin_bit_mask = 1ULL << YAKA_AUDIO_PA_ENABLE_GPIO;
        config.mode = GPIO_MODE_OUTPUT;
        config.pull_up_en = GPIO_PULLUP_DISABLE;
        config.pull_down_en = GPIO_PULLDOWN_ENABLE;
        config.intr_type = GPIO_INTR_DISABLE;
        const esp_err_t result = gpio_config(&config);
        if (result == ESP_OK) {
            gpio_set_level(YAKA_AUDIO_PA_ENABLE_GPIO, 0);
            ESP_LOGI(TAG, "PA safety interlock active: GPIO%d forced low",
                     static_cast<int>(YAKA_AUDIO_PA_ENABLE_GPIO));
        } else {
            ESP_LOGE(TAG, "PA safety interlock failed: %s", esp_err_to_name(result));
        }
    }

    void LogPlatformBaseline() {
        uint32_t flash_id = 0;
        uint32_t flash_size = 0;
        esp_err_t id_result = esp_flash_read_id(nullptr, &flash_id);
        esp_err_t size_result = esp_flash_get_size(nullptr, &flash_size);

        ESP_LOGI(TAG, "M1 board=hi-yaka target=esp32c6 psram=unsupported");
        ESP_LOGI(TAG, "Reset reason=%d", static_cast<int>(esp_reset_reason()));
        if (id_result == ESP_OK && size_result == ESP_OK) {
            ESP_LOGI(TAG, "Flash id=0x%08" PRIx32 " size=%" PRIu32 " bytes", flash_id,
                     flash_size);
        } else {
            ESP_LOGE(TAG, "Flash query failed: id=%s size=%s", esp_err_to_name(id_result),
                     esp_err_to_name(size_result));
        }

        ESP_LOGI(TAG, "Internal heap free=%u largest=%u",
                 static_cast<unsigned>(
                     heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                 static_cast<unsigned>(
                     heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));
    }

    void InitializeI2c() {
        i2c_master_bus_config_t config = {};
        config.i2c_port = YAKA_I2C_PORT;
        config.sda_io_num = YAKA_I2C_SDA_GPIO;
        config.scl_io_num = YAKA_I2C_SCL_GPIO;
        config.clk_source = I2C_CLK_SRC_DEFAULT;
        config.glitch_ignore_cnt = 7;
        config.intr_priority = 0;
        config.trans_queue_depth = 0;
        config.flags.enable_internal_pullup = 0;
        ESP_ERROR_CHECK(i2c_new_master_bus(&config, &i2c_bus_));
        ESP_LOGI(TAG, "I2C initialized: port=%d SDA=%d SCL=%d frequency=%d Hz",
                 static_cast<int>(YAKA_I2C_PORT), static_cast<int>(YAKA_I2C_SDA_GPIO),
                 static_cast<int>(YAKA_I2C_SCL_GPIO), YAKA_I2C_FREQUENCY_HZ);
    }

    void InitializePca9557SafeState() {
        esp_err_t probe_result =
            i2c_master_probe(i2c_bus_, YAKA_PCA9557_I2C_ADDRESS, 100);
        if (probe_result != ESP_OK) {
            ESP_LOGE(TAG, "PCA9557 probe failed at 0x%02X: %s",
                     YAKA_PCA9557_I2C_ADDRESS, esp_err_to_name(probe_result));
            return;
        }

        pca9557_ = std::make_unique<HiYaKaPca9557>(i2c_bus_);
        esp_err_t result = pca9557_->InitializeSafe();
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "PCA9557 safe initialization failed: %s", esp_err_to_name(result));
            pca9557_.reset();
            return;
        }

        uint8_t output = 0;
        uint8_t configuration = 0;
        esp_err_t readback_result = pca9557_->ReadBack(&output, &configuration);
        if (readback_result == ESP_OK &&
            output == YAKA_PCA9557_SAFE_OUTPUT &&
            configuration == YAKA_PCA9557_SAFE_CONFIGURATION) {
            ESP_LOGI(TAG, "PCA9557 safe state verified: output=0x%02X config=0x%02X", output,
                     configuration);
            pca_safe_state_verified_ = true;
        } else {
            ESP_LOGE(TAG, "PCA9557 safe-state readback failed: output=0x%02X config=0x%02X (%s)",
                     output, configuration, esp_err_to_name(readback_result));
        }
    }

#if CONFIG_HI_YAKA_ENABLE_DISPLAY
    void InitializeDisplay() {
        if (pca9557_ == nullptr || !pca9557_->ready()) {
            ESP_LOGE(TAG, "display skipped: PCA9557 is not ready");
            return;
        }

        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_err_t result = hi_yaka_new_expander_lcd_io(
            pca9557_.get(), YAKA_LCD_PIXEL_CLOCK_HZ, &panel_io);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "LCD expander IO initialization failed: %s", esp_err_to_name(result));
            return;
        }

        // Hold reset low, then release it. Safe initialization already placed it low.
        result = pca9557_->SetOutputBit(YAKA_PCA9557_LCD_RST_BIT, false);
        if (result == ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(20));
            result = pca9557_->SetOutputBit(YAKA_PCA9557_LCD_RST_BIT, true);
            vTaskDelay(pdMS_TO_TICKS(120));
        }
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "LCD reset sequence failed: %s", esp_err_to_name(result));
            esp_lcd_panel_io_del(panel_io);
            return;
        }

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = GPIO_NUM_NC;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel = 16;
        esp_lcd_panel_handle_t panel = nullptr;
        result = esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel);
        if (result == ESP_OK) {
            result = esp_lcd_panel_reset(panel);
        }
        if (result == ESP_OK) {
            result = esp_lcd_panel_init(panel);
        }
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "ST7789 initialization failed: %s", esp_err_to_name(result));
            if (panel != nullptr) {
                esp_lcd_panel_del(panel);
            }
            esp_lcd_panel_io_del(panel_io);
            return;
        }

        // Geometry is frozen by the display module. Orientation, offsets,
        // inversion and color order are deliberately provisional until L1.
        display_ = std::make_unique<SpiLcdDisplay>(
            panel_io, panel, YAKA_LCD_WIDTH, YAKA_LCD_HEIGHT, YAKA_LCD_OFFSET_X,
            YAKA_LCD_OFFSET_Y, false, false, false);
        backlight_ = std::make_unique<ExpanderBacklight>(*pca9557_);
        backlight_->RestoreBrightness();
        uint8_t output = 0;
        uint8_t configuration = 0;
        const esp_err_t backlight_readback =
            pca9557_->ReadBack(&output, &configuration);
        ESP_LOGI(TAG,
                 "LCD backlight latch: result=%s output=0x%02X config=0x%02X BL=%u",
                 esp_err_to_name(backlight_readback), output, configuration,
                 static_cast<unsigned>((output >> YAKA_PCA9557_LCD_BL_BIT) & 1U));
        ESP_LOGW(TAG, "LCD candidate enabled: orientation/color/inversion require L1 verification");
    }
#endif

#if CONFIG_HI_YAKA_ENABLE_AUXILIARY_SENSORS
    void InitializeAuxiliarySensors() {
        const esp_err_t imu_probe =
            i2c_master_probe(i2c_bus_, YAKA_QMI8658A_I2C_ADDRESS, 100);
        if (imu_probe == ESP_OK) {
            qmi8658a_ = std::make_unique<HiYaKaQmi8658a>(i2c_bus_);
            const esp_err_t result = qmi8658a_->Initialize();
            if (result != ESP_OK) {
                ESP_LOGE(TAG, "QMI8658A initialization failed: %s", esp_err_to_name(result));
                qmi8658a_.reset();
            }
        } else {
            ESP_LOGW(TAG, "QMI8658A unavailable at 0x%02X: %s",
                     YAKA_QMI8658A_I2C_ADDRESS, esp_err_to_name(imu_probe));
        }

        // A PN532 leaving low-power may NACK the address-only probe that wakes
        // it. Do not gate the real transport on that probe: the driver performs
        // the required wake delay, retries and firmware-version transaction.
        pn532_ = std::make_unique<HiYaKaPn532>(i2c_bus_);
        const esp_err_t nfc_result = pn532_->Initialize();
        if (nfc_result != ESP_OK) {
            ESP_LOGE(TAG, "PN532 initialization failed: %s",
                     esp_err_to_name(nfc_result));
            pn532_.reset();
        }
    }
#endif

#if CONFIG_HI_YAKA_ENABLE_STORAGE
    void InitializeStorage() {
        storage_ = std::make_unique<HiYaKaStorage>();
        const esp_err_t result = storage_->Mount();
        if (result != ESP_OK) {
            storage_.reset();
            return;
        }
        recording_store_ = std::make_unique<hi_yaka::RecordingStore>(*storage_);
        const auto recovery = recording_store_->RecoverPending();
        ESP_LOGI(TAG, "recording recovery: recovered=%lu discarded=%lu failed=%lu",
                 static_cast<unsigned long>(recovery.recovered),
                 static_cast<unsigned long>(recovery.discarded),
                 static_cast<unsigned long>(recovery.failed));
        recording_port_ =
            std::make_unique<hi_yaka::RecordingStoragePortAdapter>(*recording_store_);
    }
#endif

#if CONFIG_HI_YAKA_ENABLE_AUDIO
    void InitializeAudio() {
        const esp_err_t output_probe =
            i2c_master_probe(i2c_bus_, YAKA_ES8311_I2C_ADDRESS, 100);
        const esp_err_t input_probe =
            i2c_master_probe(i2c_bus_, YAKA_ES7210_I2C_ADDRESS, 100);
        if (output_probe != ESP_OK || input_probe != ESP_OK) {
            ESP_LOGE(TAG, "real audio skipped: ES8311=%s ES7210=%s",
                     esp_err_to_name(output_probe), esp_err_to_name(input_probe));
            return;
        }

        // Lab builds isolate both physical microphones on a standard stereo
        // frame. This distinguishes a TDM/clock mismatch from a dead SDOUT1
        // path; the production candidate retains its three-channel TDM plan.
#if CONFIG_HI_YAKA_BRINGUP_DIAGNOSTICS
        audio_codec_ = std::make_unique<HiYaKaDiagnosticAudioCodec>(
#else
        audio_codec_ = std::make_unique<BoxAudioCodec>(
#endif
            i2c_bus_, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            YAKA_AUDIO_MCLK_GPIO, YAKA_AUDIO_BCLK_GPIO, YAKA_AUDIO_WS_GPIO,
            YAKA_AUDIO_DOUT_GPIO, YAKA_AUDIO_DIN_GPIO, YAKA_AUDIO_PA_ENABLE_GPIO,
            YAKA_ES8311_CODEC_ADDRESS, YAKA_ES7210_CODEC_ADDRESS,
#if CONFIG_HI_YAKA_BRINGUP_DIAGNOSTICS
            // Capture both ES7210 slots in the lab image. A single selected
            // slot makes esp_codec_dev reconfigure the RX interface to mono,
            // which hides whether MIC2 appears in the other physical slot.
            true, 24.0f, 1, 24.0f, YAKA_I2C_PORT,
            ES7210_SEL_MIC1 | ES7210_SEL_MIC2, false, true);
#else
            true, 24.0f, 2, 0.0f, YAKA_I2C_PORT,
            ES7210_SEL_MIC1 | ES7210_SEL_MIC2 | ES7210_SEL_MIC3, false, false);
#endif
        ESP_LOGW(TAG,
                 "audio codec/I2S candidate enabled; speaker PA is hard-disabled");
    }
#endif

#if CONFIG_HI_YAKA_ENABLE_INPUT || CONFIG_HI_YAKA_ENABLE_BATTERY
    void InitializeAdc() {
        adc_ = std::make_unique<HiYaKaAdc>();
        const esp_err_t result = adc_->Initialize();
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "ADC initialization failed: %s", esp_err_to_name(result));
            adc_.reset();
            return;
        }
        ESP_LOGI(TAG, "ADC ready: calibration=%d", adc_->calibrated());
    }
#endif

#if CONFIG_HI_YAKA_ENABLE_INPUT
    void InitializeInput() {
        if (adc_ == nullptr) {
            ESP_LOGE(TAG, "input skipped: ADC unavailable");
            return;
        }
        input_ = std::make_unique<HiYaKaInput>(*adc_);
        const esp_err_t result = input_->Initialize(
            [this](hi_yaka::PhysicalKey key, hi_yaka::KeyGesture gesture) {
                Application::GetInstance().Schedule([this, key, gesture]() {
#if CONFIG_HI_YAKA_BRINGUP_DIAGNOSTICS
                    ESP_LOGI(TAG, "BRINGUP key: name=%s gesture=%s",
                             PhysicalKeyName(key),
                             gesture == hi_yaka::KeyGesture::kLongPress ? "LONG" : "PRESS");
#if CONFIG_HI_YAKA_ENABLE_FEEDBACK
                    if (key == hi_yaka::PhysicalKey::kPrimary &&
                        gesture == hi_yaka::KeyGesture::kPress && feedback_ != nullptr) {
                        ESP_LOGI(TAG, "BRINGUP motor: KEY_A -> warning pulse 80 ms");
                        feedback_->Emit(hi_yaka::FeedbackSignal::kWarning);
                    }
#endif
#endif
#if CONFIG_HI_YAKA_ENABLE_PRODUCT_RUNTIME
                    hi_yaka::RuntimeAdapter::Get().OnPhysicalInput(key, gesture);
#else
                    ESP_LOGI(TAG, "physical key=%s gesture=%s", PhysicalKeyName(key),
                             gesture == hi_yaka::KeyGesture::kLongPress ? "LONG" : "PRESS");
#endif
                });
            });
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "input initialization failed: %s", esp_err_to_name(result));
            input_.reset();
        }
    }
#endif

#if CONFIG_HI_YAKA_ENABLE_FEEDBACK
    void InitializeFeedback() {
        if (pca9557_ == nullptr) {
            ESP_LOGE(TAG, "feedback skipped: PCA9557 unavailable");
            return;
        }
        feedback_ = std::make_unique<HiYaKaFeedback>(*pca9557_);
        const esp_err_t result = feedback_->Initialize();
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "feedback initialization failed: %s", esp_err_to_name(result));
            feedback_.reset();
        }
    }
#endif

    void ProbeExpectedI2cDevices() {
        for (const auto& device : kExpectedI2cDevices) {
            esp_err_t result = i2c_master_probe(i2c_bus_, device.address, 100);
            if (result == ESP_OK) {
                ESP_LOGI(TAG, "I2C present: %s at 0x%02X", device.name, device.address);
            } else {
                ESP_LOGW(TAG, "I2C missing: %s at 0x%02X (%s)", device.name, device.address,
                         esp_err_to_name(result));
            }
        }
    }

#if CONFIG_HI_YAKA_BRINGUP_DIAGNOSTICS
    void ScanI2cBusForDiagnostics() {
        unsigned found = 0;
        ESP_LOGI(TAG, "BRINGUP I2C scan begin: 0x08..0x77");
        for (uint8_t address = 0x08; address <= 0x77; ++address) {
            if (i2c_master_probe(i2c_bus_, address, 10) == ESP_OK) {
                ESP_LOGI(TAG, "BRINGUP I2C scan found: 0x%02X", address);
                ++found;
            }
        }
        ESP_LOGI(TAG, "BRINGUP I2C scan complete: found=%u", found);
    }
#endif

#if CONFIG_HI_YAKA_BRINGUP_DIAGNOSTICS
    void StartBringupDiagnostics() {
        const BaseType_t created = xTaskCreate(
            [](void* context) {
                auto* self = static_cast<HiYaKaBoard*>(context);
                vTaskDelay(pdMS_TO_TICKS(4000));
                ESP_LOGI(TAG, "BRINGUP BEGIN: bounded unified peripheral diagnostics");

                if (self->pca9557_ != nullptr) {
                    uint8_t output = 0;
                    uint8_t configuration = 0;
                    esp_err_t result = self->pca9557_->SetOutputBit(
                        YAKA_PCA9557_LCD_BL_BIT, false);
                    ESP_LOGI(TAG, "BRINGUP backlight OFF: write=%s", esp_err_to_name(result));
                    vTaskDelay(pdMS_TO_TICKS(500));
                    result = self->pca9557_->SetOutputBit(
                        YAKA_PCA9557_LCD_BL_BIT, true);
                    const esp_err_t readback =
                        self->pca9557_->ReadBack(&output, &configuration);
                    ESP_LOGI(TAG,
                             "BRINGUP backlight ON: write=%s read=%s output=0x%02X config=0x%02X BL=%u",
                             esp_err_to_name(result), esp_err_to_name(readback), output,
                             configuration,
                             static_cast<unsigned>((output >> YAKA_PCA9557_LCD_BL_BIT) & 1U));
                }

                ESP_LOGI(TAG, "BRINGUP PN532 delayed address probe: %s",
                         esp_err_to_name(i2c_master_probe(self->i2c_bus_,
                                                           YAKA_PN532_I2C_ADDRESS, 100)));

                if (self->adc_ != nullptr) {
                    int keys_mv = 0;
                    int battery_divider_mv = 0;
                    const esp_err_t keys_result =
                        self->adc_->ReadMillivolts(YAKA_DPAD_ADC_CHANNEL, &keys_mv);
                    const esp_err_t battery_result =
                        self->adc_->ReadMillivolts(YAKA_BATTERY_ADC_CHANNEL,
                                                  &battery_divider_mv);
                    ESP_LOGI(TAG,
                             "BRINGUP ADC idle: keys=%d mV (%s), battery-divider=%d mV (%s)",
                             keys_mv, esp_err_to_name(keys_result), battery_divider_mv,
                             esp_err_to_name(battery_result));
                }

                if (self->feedback_ != nullptr) {
                    self->feedback_->RunBringupSelfTest();
                }

                if (self->audio_codec_ != nullptr) {
                    constexpr int kCaptureSeconds = 5;
                    constexpr std::size_t kFramesPerChunk = 320;
                    const std::size_t channels = self->audio_codec_->input_channels();
                    const std::size_t total_samples =
                        self->audio_codec_->input_sample_rate() * channels * kCaptureSeconds;
                    std::vector<int16_t> samples(kFramesPerChunk * channels, 0);
                    HiYaKaWavWriter writer;
                    const bool recording = self->storage_ != nullptr && self->storage_->ready() &&
                        writer.Open("/sdcard/diagnostics/microphone-capture.wav",
                                    self->audio_codec_->input_sample_rate(), channels);
                    self->audio_codec_->EnableInput(true);
                    vTaskDelay(pdMS_TO_TICKS(150));
                    self->audio_codec_->LogInputDiagnostics();
                    ESP_LOGI(TAG, "BRINGUP microphone: speak now for %d seconds", kCaptureSeconds);
                    bool captured = false;
                    std::size_t captured_samples = 0;
                    int peak = 0;
                    uint64_t energy = 0;
                    while (captured_samples < total_samples) {
                        std::fill(samples.begin(), samples.end(), 0);
                        const bool chunk_captured = self->audio_codec_->InputData(samples);
                        captured = captured || chunk_captured;
                        if (!chunk_captured) {
                            ESP_LOGE(TAG, "BRINGUP microphone read failed after %u samples",
                                     static_cast<unsigned>(captured_samples));
                            break;
                        }
                        if (recording && !writer.Append(samples.data(), samples.size())) {
                            ESP_LOGE(TAG, "BRINGUP microphone WAV write failed");
                            break;
                        }
                        for (const int16_t sample : samples) {
                            const int value = sample < 0 ? -static_cast<int>(sample)
                                                         : static_cast<int>(sample);
                            peak = std::max(peak, value);
                            energy += static_cast<uint64_t>(value) * value;
                        }
                        captured_samples += samples.size();
                    }
                    writer.Close();
                    ESP_LOGI(TAG,
                             "BRINGUP microphone capture: captured=%d samples=%u peak=%d mean_square=%" PRIu64 " wav=%s",
                             captured, static_cast<unsigned>(captured_samples), peak,
                             captured_samples == 0 ? 0 : energy / captured_samples,
                             recording ? "/sdcard/diagnostics/microphone-capture.wav" : "unavailable");
                    // Keep ES7210 and MICBIAS active long enough for a safe
                    // voltmeter check. The five-second WAV evidence above is
                    // already frozen; this window belongs only to lab builds.
                    constexpr int kMicRailWindowSeconds = 120;
                    ESP_LOGI(TAG,
                             "BRINGUP MIC RAIL WINDOW BEGIN: %d seconds; measure MICBIAS12 at R42/R45",
                             kMicRailWindowSeconds);
                    vTaskDelay(pdMS_TO_TICKS(kMicRailWindowSeconds * 1000));
                    ESP_LOGI(TAG, "BRINGUP MIC RAIL WINDOW END");
                    self->audio_codec_->EnableInput(false);
                }

                if (self->qmi8658a_ != nullptr) {
                    HiYaKaImuSample sample{};
                    const esp_err_t result = self->qmi8658a_->ReadSample(&sample, 200);
                    ESP_LOGI(TAG,
                             "BRINGUP IMU: result=%s accel_g=[%.3f %.3f %.3f] gyro_dps=[%.2f %.2f %.2f] temp=%.2fC",
                             esp_err_to_name(result), sample.acceleration_g[0],
                             sample.acceleration_g[1], sample.acceleration_g[2],
                             sample.angular_rate_dps[0], sample.angular_rate_dps[1],
                             sample.angular_rate_dps[2], sample.temperature_c);
                }

                if (self->pn532_ != nullptr) {
                    HiYaKaNfcTag tag{};
                    const HiYaKaNfcPollResult result =
                        self->pn532_->PollIso14443A(&tag, 1000);
                    if (result == HiYaKaNfcPollResult::kFound) {
                        ESP_LOGI(TAG,
                                 "BRINGUP NFC: result=found targets=%u uid_length=%u fingerprint=%08" PRIx32 " ATQA=%04X SAK=%02X",
                                 tag.target_count, tag.uid_length, NfcUidFingerprint(tag),
                                 tag.atqa, tag.sak);
                    } else {
                        ESP_LOGI(TAG, "BRINGUP NFC: result=%s",
                                 NfcPollResultName(result));
                    }
                    self->pn532_->SetRfOff();
                }

                ESP_LOGI(TAG,
                         "BRINGUP READY: press UP/DOWN/LEFT/RIGHT/CONFIRM/BACK/KEY_A/KEY_B one at a time");
                vTaskDelete(nullptr);
            },
            "hi_yaka_diag", 6144, this, 4, nullptr);
        if (created != pdPASS) {
            ESP_LOGE(TAG, "failed to start bounded bring-up diagnostics");
        }
    }
#endif

public:
    HiYaKaBoard() {
        ForcePaDisabled();
        LogPlatformBaseline();
        InitializeI2c();
        InitializePca9557SafeState();
        ProbeExpectedI2cDevices();
#if CONFIG_HI_YAKA_BRINGUP_DIAGNOSTICS
        ScanI2cBusForDiagnostics();
#endif
#if CONFIG_HI_YAKA_ENABLE_DISPLAY
        InitializeDisplay();
#endif
#if CONFIG_HI_YAKA_ENABLE_INPUT || CONFIG_HI_YAKA_ENABLE_BATTERY
        InitializeAdc();
#endif
#if CONFIG_HI_YAKA_ENABLE_INPUT
        InitializeInput();
#endif
#if CONFIG_HI_YAKA_ENABLE_FEEDBACK
        InitializeFeedback();
#endif
#if CONFIG_HI_YAKA_ENABLE_AUDIO
        InitializeAudio();
#endif
#if CONFIG_HI_YAKA_ENABLE_STORAGE
        InitializeStorage();
#endif
#if CONFIG_HI_YAKA_ENABLE_AUXILIARY_SENSORS
        InitializeAuxiliarySensors();
#endif
#if CONFIG_HI_YAKA_BRINGUP_DIAGNOSTICS
        StartBringupDiagnostics();
#endif
        ESP_LOGW(TAG,
                 "pre-board candidate: enabled modules are L0 only until physical-board verification");
    }

    Display* GetDisplay() override {
        return display_ != nullptr ? display_.get() : Board::GetDisplay();
    }

    Backlight* GetBacklight() override { return backlight_.get(); }

    Led* GetLed() override {
        return feedback_ != nullptr ? feedback_.get() : Board::GetLed();
    }

    bool GetBatteryLevel(int& level, bool& charging, bool& discharging) override {
#if CONFIG_HI_YAKA_ENABLE_BATTERY
        if (adc_ == nullptr) {
            return false;
        }
        int divider_mv = 0;
        if (adc_->ReadMillivolts(YAKA_BATTERY_ADC_CHANNEL, &divider_mv) != ESP_OK) {
            return false;
        }
        // Divider is 1 Mohm upper / 2 Mohm lower: ADC sees two thirds of BAT.
        const int battery_mv = divider_mv * 3 / 2;
        constexpr int kEmptyMillivolts = 3300;
        constexpr int kFullMillivolts = 4200;
        level = std::clamp((battery_mv - kEmptyMillivolts) * 100 /
                               (kFullMillivolts - kEmptyMillivolts),
                           0, 100);
        // No charging-state signal is connected to the MCU. Never infer a
        // charging/full state from voltage alone.
        charging = false;
        discharging = true;
        return true;
#else
        return Board::GetBatteryLevel(level, charging, discharging);
#endif
    }

    AudioCodec* GetAudioCodec() override {
#if CONFIG_HI_YAKA_ENABLE_AUDIO
        if (audio_codec_ != nullptr) {
            return audio_codec_.get();
        }
#endif
        static DummyAudioCodec codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE);
        return &codec;
    }

    bool CapabilityReady(hi_yaka::CapabilityId capability) const {
        switch (capability) {
            case hi_yaka::CapabilityId::kDisplay:
                return display_ != nullptr;
            case hi_yaka::CapabilityId::kInput:
                return input_ != nullptr;
            case hi_yaka::CapabilityId::kAudioCapture:
            case hi_yaka::CapabilityId::kAudioPlayback:
                return audio_codec_ != nullptr;
            case hi_yaka::CapabilityId::kStorage:
                return recording_port_ != nullptr;
            case hi_yaka::CapabilityId::kNetwork:
                return true;
            case hi_yaka::CapabilityId::kNfc:
                return pn532_ != nullptr && pn532_->ready();
            case hi_yaka::CapabilityId::kImu:
                return qmi8658a_ != nullptr && qmi8658a_->ready();
            case hi_yaka::CapabilityId::kFeedback:
                return feedback_ != nullptr;
            case hi_yaka::CapabilityId::kPower:
                return true;
            case hi_yaka::CapabilityId::kUpdate:
            case hi_yaka::CapabilityId::kCount:
                return false;
        }
        return false;
    }

    void SetInputEnabled(bool enabled) {
        if (input_ != nullptr) {
            input_->SetEnabled(enabled);
        }
    }

    bool BeginNfcScan() {
#if CONFIG_HI_YAKA_ENABLE_AUXILIARY_SENSORS
        if (pn532_ == nullptr || !pn532_->ready()) {
            return false;
        }
        bool expected = false;
        if (!nfc_scan_running_.compare_exchange_strong(expected, true)) {
            return false;
        }
        nfc_scan_cancelled_.store(false);
        const BaseType_t created = xTaskCreate(
            [](void* context) {
                auto* self = static_cast<HiYaKaBoard*>(context);
                bool found = false;
                HiYaKaNfcTag tag{};
                HiYaKaNfcPollResult result = HiYaKaNfcPollResult::kNoTarget;
                const int64_t deadline_us = esp_timer_get_time() + 10LL * 1000 * 1000;
                while (!self->nfc_scan_cancelled_.load() &&
                       esp_timer_get_time() < deadline_us) {
                    result = self->pn532_->PollIso14443A(&tag, 250);
                    if (result == HiYaKaNfcPollResult::kFound) {
                        found = true;
                        break;
                    }
                    if (result == HiYaKaNfcPollResult::kTransportError ||
                        result == HiYaKaNfcPollResult::kProtocolError ||
                        result == HiYaKaNfcPollResult::kNoMemory) {
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                self->pn532_->SetRfOff();
                const bool cancelled = self->nfc_scan_cancelled_.load();
                if (found) {
                    ESP_LOGI(TAG,
                             "NFC tag: targets=%u uid_length=%u fingerprint=%08" PRIx32 " ATQA=%04X SAK=%02X",
                             tag.target_count, tag.uid_length, NfcUidFingerprint(tag),
                             tag.atqa, tag.sak);
                } else {
                    ESP_LOGI(TAG, "NFC scan ended: result=%s cancelled=%d",
                             NfcPollResultName(result), cancelled);
                }
                self->nfc_scan_running_.store(false);
                if (!cancelled) {
                    Application::GetInstance().Schedule([found]() {
                        hi_yaka::RuntimeAdapter::Get().OnNfcScanCompleted(found);
                    });
                }
                vTaskDelete(nullptr);
            },
            "hi_yaka_nfc", 6144, this, 4, nullptr);
        if (created != pdPASS) {
            nfc_scan_running_.store(false);
            return false;
        }
        return true;
#else
        return false;
#endif
    }

    void CancelNfcScan() { nfc_scan_cancelled_.store(true); }

    void SetImuMonitoring(bool enabled) {
#if CONFIG_HI_YAKA_ENABLE_AUXILIARY_SENSORS
        imu_monitoring_.store(enabled);
        if (!enabled || qmi8658a_ == nullptr || !qmi8658a_->ready()) {
            return;
        }
        bool expected = false;
        if (!imu_task_running_.compare_exchange_strong(expected, true)) {
            return;
        }
        const BaseType_t created = xTaskCreate(
            [](void* context) {
                auto* self = static_cast<HiYaKaBoard*>(context);
                unsigned sample_index = 0;
                while (self->imu_monitoring_.load()) {
                    HiYaKaImuSample sample{};
                    const esp_err_t result = self->qmi8658a_->ReadSample(&sample, 100);
                    if (result != ESP_OK) {
                        ESP_LOGW(TAG, "IMU monitor read failed: %s", esp_err_to_name(result));
                    } else if ((sample_index++ % 10) == 0) {
                        ESP_LOGI(TAG,
                                 "IMU monitor: accel_g=[%.3f %.3f %.3f] gyro_dps=[%.2f %.2f %.2f]",
                                 sample.acceleration_g[0], sample.acceleration_g[1],
                                 sample.acceleration_g[2], sample.angular_rate_dps[0],
                                 sample.angular_rate_dps[1], sample.angular_rate_dps[2]);
                    }
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                self->imu_task_running_.store(false);
                vTaskDelete(nullptr);
            },
            "hi_yaka_imu", 4096, this, 3, nullptr);
        if (created != pdPASS) {
            imu_task_running_.store(false);
            imu_monitoring_.store(false);
        }
#else
        (void)enabled;
#endif
    }

    hi_yaka::RecordingStoragePortAdapter* RecordingPort() {
        return recording_port_.get();
    }

    hi_yaka::FeedbackPort* FeedbackPort() { return feedback_.get(); }

    bool OtaFirstBootSelfTest() const {
        const bool heap_available =
            heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) >= 16384;
        ESP_LOGI(TAG, "OTA first-boot self-test: i2c=%d pca_safe=%d heap=%d",
                 i2c_bus_ != nullptr, pca_safe_state_verified_, heap_available);
        return i2c_bus_ != nullptr && pca_safe_state_verified_ && heap_available;
    }
};

DECLARE_BOARD(HiYaKaBoard);

bool hi_yaka_ota_first_boot_self_test() {
    return static_cast<HiYaKaBoard&>(Board::GetInstance()).OtaFirstBootSelfTest();
}

namespace hi_yaka {

bool BoardCapabilityReady(CapabilityId capability) {
    return static_cast<HiYaKaBoard&>(Board::GetInstance()).CapabilityReady(capability);
}

void SetBoardInputEnabled(bool enabled) {
    static_cast<HiYaKaBoard&>(Board::GetInstance()).SetInputEnabled(enabled);
}

bool BoardBeginNfcScan() {
    return static_cast<HiYaKaBoard&>(Board::GetInstance()).BeginNfcScan();
}

void BoardCancelNfcScan() {
    static_cast<HiYaKaBoard&>(Board::GetInstance()).CancelNfcScan();
}

void SetBoardImuMonitoring(bool enabled) {
    static_cast<HiYaKaBoard&>(Board::GetInstance()).SetImuMonitoring(enabled);
}

RecordingStoragePortAdapter* BoardRecordingPort() {
    return static_cast<HiYaKaBoard&>(Board::GetInstance()).RecordingPort();
}

FeedbackPort* BoardFeedbackPort() {
    return static_cast<HiYaKaBoard&>(Board::GetInstance()).FeedbackPort();
}

}  // namespace hi_yaka
