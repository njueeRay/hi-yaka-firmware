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
#include "hi_yaka_storage.h"
#include "recording_store.h"
#include "runtime_adapter.h"

#include <algorithm>
#include <array>
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
    std::unique_ptr<hi_yaka::RecordingStore> recording_store_;
    std::unique_ptr<hi_yaka::RecordingStoragePortAdapter> recording_port_;
    bool pca_safe_state_verified_ = false;

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
        ESP_LOGW(TAG, "LCD candidate enabled: orientation/color/inversion require L1 verification");
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

        // First L1 candidate intentionally selects MIC1 (voice) plus MIC3
        // (playback reference). ES7210 TDM slot order and the second physical
        // microphone remain explicit M3 measurements, not assumptions.
        audio_codec_ = std::make_unique<BoxAudioCodec>(
            i2c_bus_, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            YAKA_AUDIO_MCLK_GPIO, YAKA_AUDIO_BCLK_GPIO, YAKA_AUDIO_WS_GPIO,
            YAKA_AUDIO_DOUT_GPIO, YAKA_AUDIO_DIN_GPIO, YAKA_AUDIO_PA_ENABLE_GPIO,
            YAKA_ES8311_I2C_ADDRESS, YAKA_ES7210_I2C_ADDRESS, true, 24.0f, 2, 0.0f,
            YAKA_I2C_PORT, ES7210_SEL_MIC1 | ES7210_SEL_MIC3, false);
        ESP_LOGW(TAG,
                 "capture-only audio candidate enabled; speaker/PA output is hard-disabled");
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
            [](hi_yaka::PhysicalKey key, hi_yaka::KeyGesture gesture) {
                Application::GetInstance().Schedule([key, gesture]() {
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
    void StartBringupDiagnostics() {
        const BaseType_t created = xTaskCreate(
            [](void* context) {
                auto* self = static_cast<HiYaKaBoard*>(context);
                vTaskDelay(pdMS_TO_TICKS(4000));
                ESP_LOGI(TAG, "BRINGUP BEGIN: bounded unified peripheral diagnostics");

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
                    std::vector<int16_t> samples(1600, 0);
                    self->audio_codec_->EnableInput(true);
                    vTaskDelay(pdMS_TO_TICKS(150));
                    const bool captured = self->audio_codec_->InputData(samples);
                    self->audio_codec_->EnableInput(false);
                    int peak = 0;
                    uint64_t energy = 0;
                    for (const int16_t sample : samples) {
                        const int value = sample < 0 ? -static_cast<int>(sample)
                                                     : static_cast<int>(sample);
                        peak = std::max(peak, value);
                        energy += static_cast<uint64_t>(value) * value;
                    }
                    ESP_LOGI(TAG,
                             "BRINGUP microphone capture: captured=%d samples=%u peak=%d mean_square=%" PRIu64,
                             captured, static_cast<unsigned>(samples.size()), peak,
                             energy / samples.size());
                }

                ESP_LOGI(TAG,
                         "BRINGUP READY: press UP/DOWN/LEFT/RIGHT/CONFIRM/BACK/KEY_A/KEY_B one at a time");
                vTaskDelete(nullptr);
            },
            "hi_yaka_diag", 4096, this, 4, nullptr);
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
            case hi_yaka::CapabilityId::kFeedback:
                return feedback_ != nullptr;
            case hi_yaka::CapabilityId::kPower:
                return true;
            case hi_yaka::CapabilityId::kNfc:
            case hi_yaka::CapabilityId::kImu:
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

RecordingStoragePortAdapter* BoardRecordingPort() {
    return static_cast<HiYaKaBoard&>(Board::GetInstance()).RecordingPort();
}

FeedbackPort* BoardFeedbackPort() {
    return static_cast<HiYaKaBoard&>(Board::GetInstance()).FeedbackPort();
}

}  // namespace hi_yaka
