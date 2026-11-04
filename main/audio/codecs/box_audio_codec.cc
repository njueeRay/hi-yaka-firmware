#include "box_audio_codec.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <driver/i2s_tdm.h>
#include <algorithm>
#include <array>

#define TAG "BoxAudioCodec"

BoxAudioCodec::BoxAudioCodec(void* i2c_master_handle, int input_sample_rate, int output_sample_rate,
                             gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout,
                             gpio_num_t din, gpio_num_t pa_pin, uint8_t es8311_addr,
                             uint8_t es7210_addr, bool input_reference, float input_gain,
                             int reference_gain_channel, float reference_gain,
                             i2c_port_t i2c_port, uint32_t es7210_mic_mask,
                             bool speaker_pa_enabled, bool std_stereo_input) {
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

    CreateDuplexChannels(mclk, bclk, ws, dout, din);

    // Do initialize of related interface: data_if, ctrl_if and gpio_if
    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = I2S_NUM_0,
        .rx_handle = rx_handle_,
        .tx_handle = tx_handle_,
    };
    data_if_ = audio_codec_new_i2s_data(&i2s_cfg);
    assert(data_if_ != NULL);

    // Output
    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = i2c_port,
        .addr = es8311_addr,
        .bus_handle = i2c_master_handle,
    };
    out_ctrl_if_ = audio_codec_new_i2c_ctrl(&i2c_cfg);
    assert(out_ctrl_if_ != NULL);

    gpio_if_ = audio_codec_new_gpio();
    assert(gpio_if_ != NULL);

    es8311_codec_cfg_t es8311_cfg = {};
    es8311_cfg.ctrl_if = out_ctrl_if_;
    es8311_cfg.gpio_if = gpio_if_;
    es8311_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC;
    es8311_cfg.pa_pin = speaker_pa_enabled_ ? pa_pin : GPIO_NUM_NC;
    es8311_cfg.use_mclk = true;
    es8311_cfg.hw_gain.pa_voltage = 5.0;
    es8311_cfg.hw_gain.codec_dac_voltage = 3.3;
    out_codec_if_ = es8311_codec_new(&es8311_cfg);
    assert(out_codec_if_ != NULL);

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = out_codec_if_,
        .data_if = data_if_,
    };
    output_dev_ = esp_codec_dev_new(&dev_cfg);
    assert(output_dev_ != NULL);

    // Input
    i2c_cfg.addr = es7210_addr;
    in_ctrl_if_ = audio_codec_new_i2c_ctrl(&i2c_cfg);
    assert(in_ctrl_if_ != NULL);

    es7210_codec_cfg_t es7210_cfg = {};
    es7210_cfg.ctrl_if = in_ctrl_if_;
    es7210_cfg.mic_selected = es7210_mic_mask;
    in_codec_if_ = es7210_codec_new(&es7210_cfg);
    assert(in_codec_if_ != NULL);

    dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_IN;
    dev_cfg.codec_if = in_codec_if_;
    input_dev_ = esp_codec_dev_new(&dev_cfg);
    assert(input_dev_ != NULL);

    ESP_LOGI(TAG, "BoxAudioDevice initialized (speaker_pa=%s)",
             speaker_pa_enabled_ ? "enabled" : "disabled");
}

BoxAudioCodec::~BoxAudioCodec() {
    ESP_ERROR_CHECK(esp_codec_dev_close(output_dev_));
    esp_codec_dev_delete(output_dev_);
    ESP_ERROR_CHECK(esp_codec_dev_close(input_dev_));
    esp_codec_dev_delete(input_dev_);

    audio_codec_delete_codec_if(in_codec_if_);
    audio_codec_delete_ctrl_if(in_ctrl_if_);
    audio_codec_delete_codec_if(out_codec_if_);
    audio_codec_delete_ctrl_if(out_ctrl_if_);
    audio_codec_delete_gpio_if(gpio_if_);
    audio_codec_delete_data_if(data_if_);
}

void BoxAudioCodec::CreateDuplexChannels(gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din) {
    assert(input_sample_rate_ == output_sample_rate_);

    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM,
        .dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = false,
        .intr_priority = 0,
    };
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle_, &rx_handle_));

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

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle_, &std_cfg));
    if (std_stereo_input_) {
        i2s_std_config_t rx_std_cfg = std_cfg;
        rx_std_cfg.gpio_cfg.dout = I2S_GPIO_UNUSED;
        rx_std_cfg.gpio_cfg.din = din;
        ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &rx_std_cfg));
    } else {
        ESP_ERROR_CHECK(i2s_channel_init_tdm_mode(rx_handle_, &tdm_cfg));
    }
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle_));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle_));
    ESP_LOGI(TAG, "Duplex channels created");
}

void BoxAudioCodec::SetOutputVolume(int volume) {
    ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(output_dev_, volume));
    AudioCodec::SetOutputVolume(volume);
}

void BoxAudioCodec::EnableInput(bool enable) {
    std::lock_guard<std::mutex> lock(data_if_mutex_);
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
        ESP_ERROR_CHECK(esp_codec_dev_open(input_dev_, &fs));
        ESP_ERROR_CHECK(esp_codec_dev_set_in_channel_gain(
            input_dev_, ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0), input_gain_));
        if (input_reference_ && reference_gain_channel_ >= 0) {
            // ES7210 gain masks use physical MIC numbering, which differs
            // from the TDM slot order (MIC1, MIC3, MIC2, MIC4).
            ESP_ERROR_CHECK(esp_codec_dev_set_in_channel_gain(
                input_dev_, ESP_CODEC_DEV_MAKE_CHANNEL_MASK(reference_gain_channel_),
                reference_gain_));
        }
    } else {
        ESP_ERROR_CHECK(esp_codec_dev_close(input_dev_));
    }
    AudioCodec::EnableInput(enable);
}

void BoxAudioCodec::EnableOutput(bool enable) {
    std::lock_guard<std::mutex> lock(data_if_mutex_);
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
