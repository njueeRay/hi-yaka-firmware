#include "box_audio_codec.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <driver/i2s_tdm.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <esp_timer.h>
#include <esp_rom_sys.h>
#include <freertos/semphr.h>

#define TAG "BoxAudioCodec"

namespace {
constexpr int kEs7210MaxAttempts = 3;
constexpr uint32_t kEs7210RetryDelayUs = 1500;

struct CtrlFailure {
    uint32_t sequence;
    int64_t time_us;
    int reg;
    int reg_len;
    int data_len;
    int result;
    int final_result;
    int attempt;
    bool terminal;
    uint8_t value[4];
    bool write;
    bool blocked;
};

struct ObservedCtrl {
    audio_codec_ctrl_if_t base;
    const audio_codec_ctrl_if_t* inner;
    const char* codec_name;
    uint8_t addr;
    uint32_t sequence;
    uint32_t failures;
    uint32_t recovered_ops;
    uint32_t terminal_ops;
    bool capture_active;
    bool read_fault_latched;
    bool terminal_fault_latched;
    SemaphoreHandle_t mutex;
    CtrlFailure events[64];
};

ObservedCtrl* observed(const audio_codec_ctrl_if_t* base) {
    return reinterpret_cast<ObservedCtrl*>(const_cast<audio_codec_ctrl_if_t*>(base));
}

void record_failure(ObservedCtrl* ctrl, bool write, bool blocked, int reg,
                    int reg_len, const void* data, int data_len, int result,
                    int attempt, bool terminal) {
    const uint32_t index = ctrl->failures++;
    if (index >= std::size(ctrl->events)) return;
    CtrlFailure& event = ctrl->events[index];
    event.sequence = ctrl->sequence;
    event.time_us = esp_timer_get_time();
    event.reg = reg;
    event.reg_len = reg_len;
    event.data_len = data_len;
    event.result = result;
    event.final_result = result;
    event.attempt = attempt;
    event.terminal = terminal;
    event.write = write;
    event.blocked = blocked;
    if (data != nullptr && data_len > 0) {
        std::memcpy(event.value, data, std::min<int>(data_len, sizeof(event.value)));
    }
}

void finalize_attempts(ObservedCtrl* ctrl, int final_result) {
    const uint32_t count = std::min<uint32_t>(ctrl->failures, std::size(ctrl->events));
    for (uint32_t i = 0; i < count; ++i) {
        if (ctrl->events[i].sequence == ctrl->sequence) {
            ctrl->events[i].final_result = final_result;
        }
    }
}

int observed_open(const audio_codec_ctrl_if_t* base, void* cfg, int cfg_size) {
    auto* ctrl = observed(base);
    if (ctrl->inner == nullptr) return ESP_CODEC_DEV_WRONG_STATE;
    if (ctrl->inner->is_open(ctrl->inner)) return ESP_CODEC_DEV_OK;
    return ctrl->inner->open(ctrl->inner, cfg, cfg_size);
}

bool observed_is_open(const audio_codec_ctrl_if_t* base) {
    const auto* ctrl = observed(base);
    return ctrl->inner != nullptr && ctrl->inner->is_open(ctrl->inner);
}

int observed_read(const audio_codec_ctrl_if_t* base, int reg, int reg_len,
                  void* data, int data_len) {
    auto* ctrl = observed(base);
    xSemaphoreTake(ctrl->mutex, portMAX_DELAY);
    const bool capture = ctrl->capture_active;
    if (capture) ++ctrl->sequence;
    int result = ESP_CODEC_DEV_READ_FAIL;
    for (int attempt = 1; attempt <= kEs7210MaxAttempts; ++attempt) {
        result = ctrl->inner->read_reg(ctrl->inner, reg, reg_len, data, data_len);
        if (result == ESP_CODEC_DEV_OK) {
            if (capture && attempt > 1) ++ctrl->recovered_ops;
            break;
        }
        const bool terminal = attempt == kEs7210MaxAttempts;
        if (capture) record_failure(ctrl, false, false, reg, reg_len, nullptr,
                                    data_len, result, attempt, terminal);
        if (terminal && capture) {
            ++ctrl->terminal_ops;
            ctrl->read_fault_latched = true;
            ctrl->terminal_fault_latched = true;
        } else {
            esp_rom_delay_us(kEs7210RetryDelayUs);
        }
    }
    if (capture) finalize_attempts(ctrl, result);
    xSemaphoreGive(ctrl->mutex);
    return result;
}

int observed_write(const audio_codec_ctrl_if_t* base, int reg, int reg_len,
                   void* data, int data_len) {
    auto* ctrl = observed(base);
    xSemaphoreTake(ctrl->mutex, portMAX_DELAY);
    const bool capture = ctrl->capture_active;
    if (capture) ++ctrl->sequence;
    if (capture && ctrl->terminal_fault_latched) {
        record_failure(ctrl, true, true, reg, reg_len, data, data_len,
                       ESP_CODEC_DEV_WRITE_FAIL, 0, true);
        xSemaphoreGive(ctrl->mutex);
        return ESP_CODEC_DEV_WRITE_FAIL;
    }
    int result = ESP_CODEC_DEV_WRITE_FAIL;
    for (int attempt = 1; attempt <= kEs7210MaxAttempts; ++attempt) {
        result = ctrl->inner->write_reg(ctrl->inner, reg, reg_len, data, data_len);
        if (result == ESP_CODEC_DEV_OK) {
            if (capture && attempt > 1) ++ctrl->recovered_ops;
            break;
        }
        const bool terminal = attempt == kEs7210MaxAttempts;
        if (capture) record_failure(ctrl, true, false, reg, reg_len, data,
                                    data_len, result, attempt, terminal);
        if (terminal && capture) {
            ++ctrl->terminal_ops;
            ctrl->terminal_fault_latched = true;
        } else {
            esp_rom_delay_us(kEs7210RetryDelayUs);
        }
    }
    if (capture) finalize_attempts(ctrl, result);
    xSemaphoreGive(ctrl->mutex);
    return result;
}

int observed_get_info(const audio_codec_ctrl_if_t* base, audio_codec_ctrl_info_t* info) {
    auto* ctrl = observed(base);
    return ctrl->inner->get_info(ctrl->inner, info);
}

int observed_close(const audio_codec_ctrl_if_t* base) {
    auto* ctrl = observed(base);
    if (ctrl->inner == nullptr) return ESP_CODEC_DEV_OK;
    const int result = audio_codec_delete_ctrl_if(ctrl->inner);
    ctrl->inner = nullptr;
    vSemaphoreDelete(ctrl->mutex);
    ctrl->mutex = nullptr;
    return result;
}

const audio_codec_ctrl_if_t* make_observed_ctrl(const audio_codec_ctrl_if_t* inner,
                                                uint8_t addr, const char* codec_name) {
    auto* ctrl = static_cast<ObservedCtrl*>(std::calloc(1, sizeof(ObservedCtrl)));
    if (ctrl == nullptr) return nullptr;
    ctrl->inner = inner;
    ctrl->addr = addr;
    ctrl->codec_name = codec_name;
    ctrl->capture_active = true;
    ctrl->mutex = xSemaphoreCreateMutex();
    if (ctrl->mutex == nullptr) {
        std::free(ctrl);
        return nullptr;
    }
    ctrl->base.open = observed_open;
    ctrl->base.is_open = observed_is_open;
    ctrl->base.read_reg = observed_read;
    ctrl->base.write_reg = observed_write;
    ctrl->base.get_info = observed_get_info;
    ctrl->base.close = observed_close;
    return &ctrl->base;
}

void stop_observed_ctrl(const audio_codec_ctrl_if_t* base) {
    auto* ctrl = observed(base);
    xSemaphoreTake(ctrl->mutex, portMAX_DELAY);
    ctrl->capture_active = false;
    xSemaphoreGive(ctrl->mutex);
}

void begin_observed_ctrl(const audio_codec_ctrl_if_t* base) {
    auto* ctrl = observed(base);
    xSemaphoreTake(ctrl->mutex, portMAX_DELAY);
    ctrl->sequence = 0;
    ctrl->failures = 0;
    ctrl->recovered_ops = 0;
    ctrl->terminal_ops = 0;
    ctrl->read_fault_latched = false;
    ctrl->terminal_fault_latched = false;
    ctrl->capture_active = true;
    xSemaphoreGive(ctrl->mutex);
}

bool observed_ctrl_faulted(const audio_codec_ctrl_if_t* base) {
    auto* ctrl = observed(base);
    xSemaphoreTake(ctrl->mutex, portMAX_DELAY);
    const bool faulted = ctrl->terminal_fault_latched || ctrl->terminal_ops != 0;
    xSemaphoreGive(ctrl->mutex);
    return faulted;
}

void report_observed_ctrl(const audio_codec_ctrl_if_t* base) {
    auto* ctrl = observed(base);
    xSemaphoreTake(ctrl->mutex, portMAX_DELAY);
    ctrl->capture_active = false;
    ESP_LOGI(TAG, "%s_CTRL_SUMMARY addr=0x%02x ops=%lu failed_attempts=%lu recovered_ops=%lu terminal_ops=%lu read_fault_guard=%d terminal_fault_guard=%d max_attempts=%d retry_delay_us=%lu",
             ctrl->codec_name, ctrl->addr, static_cast<unsigned long>(ctrl->sequence),
             static_cast<unsigned long>(ctrl->failures),
             static_cast<unsigned long>(ctrl->recovered_ops),
             static_cast<unsigned long>(ctrl->terminal_ops), ctrl->read_fault_latched,
             ctrl->terminal_fault_latched,
             kEs7210MaxAttempts, static_cast<unsigned long>(kEs7210RetryDelayUs));
    const uint32_t count = std::min<uint32_t>(ctrl->failures, std::size(ctrl->events));
    for (uint32_t i = 0; i < count; ++i) {
        const auto& e = ctrl->events[i];
        ESP_LOGW(TAG, "%s_CTRL_ATTEMPT seq=%lu t_us=%lld addr=0x%02x op=%s reg=0x%02x reg_len=%d data_len=%d value=%02x%02x%02x%02x ret=%d attempt=%d final_ret=%d recovered=%d terminal=%d blocked=%d",
                 ctrl->codec_name,
                 static_cast<unsigned long>(e.sequence), static_cast<long long>(e.time_us),
                 ctrl->addr, e.write ? "write" : "read", e.reg, e.reg_len, e.data_len,
                 e.value[0], e.value[1], e.value[2], e.value[3], e.result,
                 e.attempt, e.final_result, e.final_result == ESP_CODEC_DEV_OK,
                 e.terminal, e.blocked);
    }
    if (ctrl->failures > count) {
        ESP_LOGW(TAG, "%s_CTRL_FAILURE_OVERFLOW dropped=%lu",
                 ctrl->codec_name, static_cast<unsigned long>(ctrl->failures - count));
    }
    xSemaphoreGive(ctrl->mutex);
}
}  // namespace

BoxAudioCodec::BoxAudioCodec(void* i2c_master_handle, int input_sample_rate, int output_sample_rate,
                             gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout,
                             gpio_num_t din, gpio_num_t pa_pin, uint8_t es8311_addr,
                             uint8_t es7210_addr, bool input_reference, float input_gain,
                             int reference_gain_channel, float reference_gain,
                             i2c_port_t i2c_port, uint32_t es7210_mic_mask,
                             bool speaker_pa_enabled, bool std_stereo_input,
                             bool graceful_init_failure) {
    graceful_init_failure_ = graceful_init_failure;
    auto fail = [this](const char* stage) {
        ESP_LOGE(TAG, "initialization failed at %s; graceful=%d", stage,
                 graceful_init_failure_);
        if (!graceful_init_failure_) {
            assert(false && "BoxAudioCodec initialization failed");
        }
    };
    duplex_ = true;                              // 是否双工
    input_reference_ = input_reference;          // 是否使用参考输入，实现回声消除
    input_channels_ = input_reference_ ? 2 : 1;  // 输入通道数
    input_sample_rate_ = input_sample_rate;
    output_sample_rate_ = output_sample_rate;
    input_gain_ = input_gain;
    reference_gain_channel_ = reference_gain_channel;
    reference_gain_ = reference_gain;
    speaker_pa_enabled_ = speaker_pa_enabled;
    std_stereo_input_ = std_stereo_input;

    if (!CreateDuplexChannels(mclk, bclk, ws, dout, din)) {
        fail("I2S duplex channels");
        return;
    }

    // Do initialize of related interface: data_if, ctrl_if and gpio_if
    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = I2S_NUM_0,
        .rx_handle = rx_handle_,
        .tx_handle = tx_handle_,
    };
    data_if_ = audio_codec_new_i2s_data(&i2s_cfg);
    if (data_if_ == nullptr) {
        fail("I2S data interface");
        return;
    }

    // Output
    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = i2c_port,
        .addr = es8311_addr,
        .bus_handle = i2c_master_handle,
    };
    const auto* es8311_inner_ctrl = audio_codec_new_i2c_ctrl(&i2c_cfg);
    if (es8311_inner_ctrl != nullptr && graceful_init_failure_) {
        out_ctrl_if_ = make_observed_ctrl(es8311_inner_ctrl, es8311_addr, "ES8311");
        if (out_ctrl_if_ == nullptr) audio_codec_delete_ctrl_if(es8311_inner_ctrl);
    } else {
        out_ctrl_if_ = es8311_inner_ctrl;
    }
    if (out_ctrl_if_ == nullptr) {
        fail("ES8311 I2C control interface");
        return;
    }

    gpio_if_ = audio_codec_new_gpio();
    if (gpio_if_ == nullptr) {
        fail("codec GPIO interface");
        return;
    }

    es8311_codec_cfg_t es8311_cfg = {};
    es8311_cfg.ctrl_if = out_ctrl_if_;
    es8311_cfg.gpio_if = gpio_if_;
    es8311_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC;
    es8311_cfg.pa_pin = speaker_pa_enabled_ ? pa_pin : GPIO_NUM_NC;
    es8311_cfg.use_mclk = true;
    es8311_cfg.hw_gain.pa_voltage = 5.0;
    es8311_cfg.hw_gain.codec_dac_voltage = 3.3;
    out_codec_if_ = es8311_codec_new(&es8311_cfg);
    if (graceful_init_failure_) stop_observed_ctrl(out_ctrl_if_);
    if (out_codec_if_ == nullptr) {
        if (graceful_init_failure_) report_observed_ctrl(out_ctrl_if_);
        fail("ES8311 codec open");
        return;
    }

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = out_codec_if_,
        .data_if = data_if_,
    };
    output_dev_ = esp_codec_dev_new(&dev_cfg);
    if (output_dev_ == nullptr) {
        fail("ES8311 device");
        return;
    }

    // Input
    i2c_cfg.addr = es7210_addr;
    const auto* es7210_inner_ctrl = audio_codec_new_i2c_ctrl(&i2c_cfg);
    if (es7210_inner_ctrl != nullptr && graceful_init_failure_) {
        in_ctrl_if_ = make_observed_ctrl(es7210_inner_ctrl, es7210_addr, "ES7210");
        if (in_ctrl_if_ == nullptr) audio_codec_delete_ctrl_if(es7210_inner_ctrl);
    } else {
        in_ctrl_if_ = es7210_inner_ctrl;
    }
    if (in_ctrl_if_ == nullptr) {
        fail("ES7210 I2C control interface");
        return;
    }

    es7210_codec_cfg_t es7210_cfg = {};
    es7210_cfg.ctrl_if = in_ctrl_if_;
    es7210_cfg.mic_selected = es7210_mic_mask;
    in_codec_if_ = es7210_codec_new(&es7210_cfg);
    if (graceful_init_failure_) {
        report_observed_ctrl(in_ctrl_if_);
        report_observed_ctrl(out_ctrl_if_);
    }
    if (in_codec_if_ == nullptr) {
        fail("ES7210 codec open");
        return;
    }

    dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_IN;
    dev_cfg.codec_if = in_codec_if_;
    input_dev_ = esp_codec_dev_new(&dev_cfg);
    if (input_dev_ == nullptr) {
        fail("ES7210 device");
        return;
    }

    ready_ = true;
    ESP_LOGI(TAG, "BoxAudioDevice initialized (speaker_pa=%s)",
             speaker_pa_enabled_ ? "enabled" : "disabled");
}

BoxAudioCodec::~BoxAudioCodec() {
    ready_ = false;
    if (input_dev_ != nullptr) {
        const int result = esp_codec_dev_close(input_dev_);
        if (result != ESP_CODEC_DEV_OK) {
            ESP_LOGW(TAG, "input device close failed: %d", result);
        }
        esp_codec_dev_delete(input_dev_);
    }
    if (output_dev_ != nullptr) {
        const int result = esp_codec_dev_close(output_dev_);
        if (result != ESP_CODEC_DEV_OK) {
            ESP_LOGW(TAG, "output device close failed: %d", result);
        }
        esp_codec_dev_delete(output_dev_);
    }
    if (in_codec_if_ != nullptr) audio_codec_delete_codec_if(in_codec_if_);
    if (in_ctrl_if_ != nullptr) audio_codec_delete_ctrl_if(in_ctrl_if_);
    if (out_codec_if_ != nullptr) audio_codec_delete_codec_if(out_codec_if_);
    if (out_ctrl_if_ != nullptr) audio_codec_delete_ctrl_if(out_ctrl_if_);
    if (gpio_if_ != nullptr) audio_codec_delete_gpio_if(gpio_if_);
    if (data_if_ != nullptr) audio_codec_delete_data_if(data_if_);

    if (rx_handle_ != nullptr) {
        if (rx_enabled_) {
            const esp_err_t result = i2s_channel_disable(rx_handle_);
            if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
                ESP_LOGW(TAG, "RX disable failed: %s", esp_err_to_name(result));
            }
        }
        const esp_err_t result = i2s_del_channel(rx_handle_);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "RX delete failed: %s", esp_err_to_name(result));
        }
    }
    if (tx_handle_ != nullptr) {
        if (tx_enabled_) {
            const esp_err_t result = i2s_channel_disable(tx_handle_);
            if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
                ESP_LOGW(TAG, "TX disable failed: %s", esp_err_to_name(result));
            }
        }
        const esp_err_t result = i2s_del_channel(tx_handle_);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "TX delete failed: %s", esp_err_to_name(result));
        }
    }
}

bool BoxAudioCodec::CreateDuplexChannels(gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din) {
    if (input_sample_rate_ != output_sample_rate_) {
        ESP_LOGE(TAG, "I2S input/output sample-rate mismatch");
        return false;
    }

    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM,
        .dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = false,
        .intr_priority = 0,
    };
    esp_err_t result = i2s_new_channel(&chan_cfg, &tx_handle_, &rx_handle_);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "I2S channel creation failed: %s", esp_err_to_name(result));
        return false;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = (uint32_t)output_sample_rate_,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .ext_clk_freq_hz = 0,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_STEREO,
            .slot_mask = I2S_STD_SLOT_BOTH,
            .ws_width = I2S_DATA_BIT_WIDTH_16BIT,
            .ws_pol = false,
            .bit_shift = true,
            .left_align = true,
            .big_endian = false,
            .bit_order_lsb = false
        },
        .gpio_cfg = {
            .mclk = mclk,
            .bclk = bclk,
            .ws = ws,
            .dout = dout,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            }
        }
    };

    i2s_tdm_config_t tdm_cfg = {
        .clk_cfg = {
            .sample_rate_hz = (uint32_t)input_sample_rate_,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .ext_clk_freq_hz = 0,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
            .bclk_div = 8,
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_STEREO,
            .slot_mask = i2s_tdm_slot_mask_t(I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3),
            .ws_width = I2S_TDM_AUTO_WS_WIDTH,
            .ws_pol = false,
            .bit_shift = true,
            .left_align = false,
            .big_endian = false,
            .bit_order_lsb = false,
            .skip_mask = false,
            .total_slot = I2S_TDM_AUTO_SLOT_NUM
        },
        .gpio_cfg = {
            .mclk = mclk,
            .bclk = bclk,
            .ws = ws,
            .dout = I2S_GPIO_UNUSED,
            .din = din,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            }
        }
    };

    result = i2s_channel_init_std_mode(tx_handle_, &std_cfg);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "I2S TX initialization failed: %s", esp_err_to_name(result));
        return false;
    }
    if (std_stereo_input_) {
        i2s_std_config_t rx_std_cfg = std_cfg;
        rx_std_cfg.gpio_cfg.dout = I2S_GPIO_UNUSED;
        rx_std_cfg.gpio_cfg.din = din;
        result = i2s_channel_init_std_mode(rx_handle_, &rx_std_cfg);
    } else {
        result = i2s_channel_init_tdm_mode(rx_handle_, &tdm_cfg);
    }
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "I2S RX initialization failed: %s", esp_err_to_name(result));
        return false;
    }
    result = i2s_channel_enable(tx_handle_);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "I2S TX enable failed: %s", esp_err_to_name(result));
        return false;
    }
    tx_enabled_ = true;
    result = i2s_channel_enable(rx_handle_);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "I2S RX enable failed: %s", esp_err_to_name(result));
        return false;
    }
    rx_enabled_ = true;
    ESP_LOGI(TAG, "Duplex channels created");
    return true;
}

void BoxAudioCodec::SetOutputVolume(int volume) {
    ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(output_dev_, volume));
    AudioCodec::SetOutputVolume(volume);
}

void BoxAudioCodec::EnableInput(bool enable) {
    std::lock_guard<std::mutex> lock(data_if_mutex_);
    if (!ready_ || input_dev_ == nullptr) {
        ESP_LOGE(TAG, "input enable skipped: codec unavailable");
        return;
    }
    if (enable == input_enabled_) {
        return;
    }
    if (enable) {
        esp_codec_dev_sample_info_t fs = {
            .bits_per_sample = 16,
            .channel = static_cast<uint8_t>(std_stereo_input_ ? 2 : 4),
            .channel_mask = ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0),
            .sample_rate = (uint32_t)output_sample_rate_,
            .mclk_multiple = 0,
        };
        if (input_reference_) {
            fs.channel_mask |= ESP_CODEC_DEV_MAKE_CHANNEL_MASK(1);
        }
        if (graceful_init_failure_) {
            begin_observed_ctrl(in_ctrl_if_);
            const char* failed_stage = nullptr;
            int result = esp_codec_dev_open(input_dev_, &fs);
            if (result != ESP_CODEC_DEV_OK || observed_ctrl_faulted(in_ctrl_if_)) {
                failed_stage = "open";
            }
            if (failed_stage == nullptr) {
                result = esp_codec_dev_set_in_channel_gain(
                    input_dev_, ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0), input_gain_);
                if (result != ESP_CODEC_DEV_OK || observed_ctrl_faulted(in_ctrl_if_)) {
                    failed_stage = "mic1_gain";
                }
            }
            if (failed_stage == nullptr && input_reference_ && reference_gain_channel_ >= 0) {
                result = esp_codec_dev_set_in_channel_gain(
                    input_dev_, ESP_CODEC_DEV_MAKE_CHANNEL_MASK(reference_gain_channel_),
                    reference_gain_);
                if (result != ESP_CODEC_DEV_OK || observed_ctrl_faulted(in_ctrl_if_)) {
                    failed_stage = "reference_gain";
                }
            }
            report_observed_ctrl(in_ctrl_if_);
            if (failed_stage != nullptr) {
                ESP_LOGE(TAG, "input enable failed stage=%s result=%d; closing partial device",
                         failed_stage, result);
                const int close_result = esp_codec_dev_close(input_dev_);
                ESP_LOGW(TAG, "partial input close result=%d; not proof of hardware reset",
                         close_result);
                return;
            }
        } else {
            ESP_ERROR_CHECK(esp_codec_dev_open(input_dev_, &fs));
            ESP_ERROR_CHECK(esp_codec_dev_set_in_channel_gain(
                input_dev_, ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0), input_gain_));
            if (input_reference_ && reference_gain_channel_ >= 0) {
                ESP_ERROR_CHECK(esp_codec_dev_set_in_channel_gain(
                    input_dev_, ESP_CODEC_DEV_MAKE_CHANNEL_MASK(reference_gain_channel_),
                    reference_gain_));
            }
        }
    } else {
        if (graceful_init_failure_) {
            const int result = esp_codec_dev_close(input_dev_);
            if (result != ESP_CODEC_DEV_OK) {
                ESP_LOGE(TAG, "input close failed result=%d", result);
            }
        } else {
            ESP_ERROR_CHECK(esp_codec_dev_close(input_dev_));
        }
    }
    AudioCodec::EnableInput(enable);
}

void BoxAudioCodec::EnableOutput(bool enable) {
    std::lock_guard<std::mutex> lock(data_if_mutex_);
    if (graceful_init_failure_ && !speaker_pa_enabled_) {
        if (enable) ESP_LOGI(TAG, "output disabled for microphone-only diagnostic");
        return;
    }
    if (enable == output_enabled_) {
        return;
    }
    if (enable) {
        // Play 16bit 1 channel
        esp_codec_dev_sample_info_t fs = {
            .bits_per_sample = 16,
            .channel = 1,
            .channel_mask = 0,
            .sample_rate = (uint32_t)output_sample_rate_,
            .mclk_multiple = 0,
        };
        ESP_ERROR_CHECK(esp_codec_dev_open(output_dev_, &fs));
        ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(output_dev_, output_volume_));
    } else {
        ESP_ERROR_CHECK(esp_codec_dev_close(output_dev_));
    }
    AudioCodec::EnableOutput(enable);
}

bool BoxAudioCodec::StartSilentOutputPrelude() {
    std::lock_guard<std::mutex> lock(data_if_mutex_);
    if (!graceful_init_failure_ || speaker_pa_enabled_ || !ready_ ||
        output_dev_ == nullptr || silent_prelude_open_) {
        ESP_LOGE(TAG, "silent prelude start rejected: ready=%d pa_enabled=%d open=%d",
                 ready_, speaker_pa_enabled_, silent_prelude_open_);
        return false;
    }
    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel = 1,
        .channel_mask = 0,
        .sample_rate = static_cast<uint32_t>(output_sample_rate_),
        .mclk_multiple = 0,
    };
    begin_observed_ctrl(out_ctrl_if_);
    const int open_result = esp_codec_dev_open(output_dev_, &fs);
    const bool open_fault = observed_ctrl_faulted(out_ctrl_if_);
    int volume_result = ESP_CODEC_DEV_WRONG_STATE;
    if (open_result == ESP_CODEC_DEV_OK && !open_fault) {
        volume_result = esp_codec_dev_set_out_vol(output_dev_, output_volume_);
    }
    const bool final_fault = observed_ctrl_faulted(out_ctrl_if_);
    report_observed_ctrl(out_ctrl_if_);
    ESP_LOGI(TAG, "D5_PRELUDE_OPEN ret=%d volume_ret=%d control_fault=%d",
             open_result, volume_result, final_fault);
    if (open_result != ESP_CODEC_DEV_OK || volume_result != ESP_CODEC_DEV_OK ||
        final_fault) {
        const int close_result = esp_codec_dev_close(output_dev_);
        ESP_LOGW(TAG, "D5_PRELUDE_PARTIAL_CLOSE ret=%d (not hardware reset proof)",
                 close_result);
        return false;
    }
    silent_prelude_open_ = true;
    return true;
}

bool BoxAudioCodec::WriteSilentOutputPrelude(const int16_t* samples, int count) {
    std::lock_guard<std::mutex> lock(data_if_mutex_);
    if (!silent_prelude_open_ || samples == nullptr || count != 960) return false;
    const int result = esp_codec_dev_write(output_dev_,
                                           const_cast<int16_t*>(samples),
                                           count * sizeof(int16_t));
    if (result != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "D5_PRELUDE_WRITE ret=%d samples=%d", result, count);
    }
    return result == ESP_CODEC_DEV_OK;
}

bool BoxAudioCodec::StopSilentOutputPrelude() {
    std::lock_guard<std::mutex> lock(data_if_mutex_);
    if (!silent_prelude_open_) return false;
    begin_observed_ctrl(out_ctrl_if_);
    const int close_result = esp_codec_dev_close(output_dev_);
    const bool control_fault = observed_ctrl_faulted(out_ctrl_if_);
    report_observed_ctrl(out_ctrl_if_);
    silent_prelude_open_ = false;
    ESP_LOGI(TAG, "D5_PRELUDE_CLOSE ret=%d control_fault=%d", close_result, control_fault);
    return close_result == ESP_CODEC_DEV_OK && !control_fault;
}

int BoxAudioCodec::Read(int16_t* dest, int samples) {
    if (!input_enabled_ || dest == nullptr || samples <= 0) {
        return 0;
    }
    const int result = esp_codec_dev_read(input_dev_, dest, samples * sizeof(int16_t));
    if (result != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "input read failed: codec result=%d samples=%d", result, samples);
        return 0;
    }
    return samples;
}

void BoxAudioCodec::LogInputDiagnostics() {
    if (!input_enabled_) {
        ESP_LOGW(TAG, "input diagnostic skipped: input disabled");
        return;
    }
    constexpr std::array<int, 23> kRegisters = {
        0x00, 0x01, 0x02, 0x06, 0x08, 0x0B, 0x11, 0x12,
        0x14, 0x15, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45,
        0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C,
    };
    for (const int reg : kRegisters) {
        int value = 0;
        const int result = esp_codec_dev_read_reg(input_dev_, reg, &value);
        ESP_LOGI(TAG, "ES7210 register 0x%02X: result=%d value=0x%02X", reg,
                 result, value & 0xFF);
        if (reg == 0x0B && result == 0) {
            ESP_LOGI(TAG, "ES7210 chip state: %d (0=power-down, 1=initial, 2=normal, 3=power-up)",
                     value & 0x03);
        }
    }

    std::array<int16_t, 512> raw{};
    size_t bytes_read = 0;
    const esp_err_t result = i2s_channel_read(rx_handle_, raw.data(),
                                               raw.size() * sizeof(raw[0]),
                                               &bytes_read, pdMS_TO_TICKS(250));
    int peak_even = 0;
    int peak_odd = 0;
    size_t nonzero = 0;
    for (size_t index = 0; index < bytes_read / sizeof(raw[0]); ++index) {
        const int value = raw[index] < 0 ? -static_cast<int>(raw[index])
                                         : static_cast<int>(raw[index]);
        (index & 1U ? peak_odd : peak_even) =
            std::max(index & 1U ? peak_odd : peak_even, value);
        nonzero += raw[index] != 0;
    }
    ESP_LOGI(TAG,
             "raw I2S RX: result=%s bytes=%u nonzero=%u even_peak=%d odd_peak=%d first=%d,%d,%d,%d",
             esp_err_to_name(result), static_cast<unsigned>(bytes_read),
             static_cast<unsigned>(nonzero), peak_even, peak_odd,
             raw[0], raw[1], raw[2], raw[3]);
}

int BoxAudioCodec::Write(const int16_t* data, int samples) {
    if (output_enabled_) {
        const esp_err_t result =
            esp_codec_dev_write(output_dev_, (void*)data, samples * sizeof(int16_t));
        ESP_ERROR_CHECK_WITHOUT_ABORT(result);
        if (result == ESP_OK && !digital_output_observed_ && samples > 0) {
            ESP_LOGI(TAG,
                     "digital audio write verified: samples=%d speaker_pa=%s",
                     samples, speaker_pa_enabled_ ? "enabled" : "disabled");
            digital_output_observed_ = true;
        }
    }
    return samples;
}
