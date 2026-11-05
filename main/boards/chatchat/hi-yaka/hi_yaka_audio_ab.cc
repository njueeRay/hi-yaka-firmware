#include "hi_yaka_audio_ab.h"

#include "config.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <driver/gpio.h>
#include <driver/i2s_std.h>
#include <esp_codec_dev.h>
#include <esp_codec_dev_defaults.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>

#if CONFIG_HI_YAKA_AUDIO_AB_DIAGNOSTICS

namespace {

constexpr char kTag[] = "HiYaKaAudioAB";
constexpr int kSampleRate = 16000;
constexpr std::size_t kSamplesPerBlock = 512;

i2s_std_config_t MakeStandardConfig(gpio_num_t din) {
    i2s_std_config_t config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(kSampleRate),
        .slot_cfg =
            I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg =
            {
                .mclk = YAKA_AUDIO_MCLK_GPIO,
                .bclk = YAKA_AUDIO_BCLK_GPIO,
                .ws = YAKA_AUDIO_WS_GPIO,
                .dout = din == YAKA_AUDIO_DOUT_GPIO ? YAKA_AUDIO_DOUT_GPIO : I2S_GPIO_UNUSED,
                .din = din,
                .invert_flags = {},
            },
    };
    return config;
}

void StopAndDelete(i2s_chan_handle_t channel) {
    if (channel == nullptr) {
        return;
    }
    // The codec data interface can already have stopped the channel.
    const esp_err_t stopped = i2s_channel_disable(channel);
    if (stopped != ESP_OK && stopped != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(kTag, "I2S stop: %s", esp_err_to_name(stopped));
    }
    const esp_err_t deleted = i2s_del_channel(channel);
    if (deleted != ESP_OK) {
        ESP_LOGE(kTag, "I2S delete: %s", esp_err_to_name(deleted));
    }
}

void RunInternalLoopback() {
    i2s_chan_handle_t tx = nullptr;
    i2s_chan_handle_t rx = nullptr;
    i2s_chan_config_t chan_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_config.dma_desc_num = 4;
    chan_config.dma_frame_num = 256;
    esp_err_t result = i2s_new_channel(&chan_config, &tx, &rx);
    if (result != ESP_OK) {
        ESP_LOGE(kTag, "A internal loopback allocation failed: %s", esp_err_to_name(result));
        return;
    }

    // GPIO1 terminates at the ES8311 digital input. No PA is enabled. Never
    // drive GPIO13, which is connected to the ES7210 digital output.
    const i2s_std_config_t config = MakeStandardConfig(YAKA_AUDIO_DOUT_GPIO);
    result = i2s_channel_init_std_mode(tx, &config);
    if (result == ESP_OK)
        result = i2s_channel_init_std_mode(rx, &config);
    if (result == ESP_OK)
        result = i2s_channel_enable(tx);
    if (result == ESP_OK)
        result = i2s_channel_enable(rx);
    if (result != ESP_OK) {
        ESP_LOGE(kTag, "A internal loopback setup failed: %s", esp_err_to_name(result));
        StopAndDelete(rx);
        StopAndDelete(tx);
        return;
    }

    std::array<int16_t, kSamplesPerBlock> sent{};
    std::array<int16_t, kSamplesPerBlock> received{};
    for (std::size_t i = 0; i < sent.size(); ++i) {
        sent[i] = i & 1U ? static_cast<int16_t>(0x2468) : static_cast<int16_t>(0x1357);
    }

    std::size_t total_received = 0;
    std::size_t matching_words = 0;
    esp_err_t io_result = ESP_OK;
    for (int pass = 0; pass < 8; ++pass) {
        std::size_t written = 0;
        std::size_t read = 0;
        io_result = i2s_channel_write(tx, sent.data(), sizeof(sent), &written, pdMS_TO_TICKS(300));
        if (io_result != ESP_OK || written != sizeof(sent))
            break;
        io_result =
            i2s_channel_read(rx, received.data(), sizeof(received), &read, pdMS_TO_TICKS(300));
        if (io_result != ESP_OK)
            break;
        total_received += read / sizeof(int16_t);
        for (std::size_t i = 0; i < read / sizeof(int16_t); ++i) {
            matching_words += received[i] == static_cast<int16_t>(0x1357) ||
                              received[i] == static_cast<int16_t>(0x2468);
        }
    }
    ESP_LOGI(kTag, "A internal loopback: io=%s received=%u matching=%u result=%s",
             esp_err_to_name(io_result), static_cast<unsigned>(total_received),
             static_cast<unsigned>(matching_words), matching_words >= 64 ? "PASS" : "INCONCLUSIVE");
    StopAndDelete(rx);
    StopAndDelete(tx);
}

void RunRxOnly(i2c_master_bus_handle_t i2c_bus) {
    i2s_chan_handle_t rx = nullptr;
    i2s_chan_config_t chan_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_config.dma_desc_num = 4;
    chan_config.dma_frame_num = 256;
    esp_err_t result = i2s_new_channel(&chan_config, nullptr, &rx);
    if (result == ESP_OK) {
        const i2s_std_config_t config = MakeStandardConfig(YAKA_AUDIO_DIN_GPIO);
        result = i2s_channel_init_std_mode(rx, &config);
    }
    if (result != ESP_OK) {
        ESP_LOGE(kTag, "B RX-only I2S setup failed: %s", esp_err_to_name(result));
        StopAndDelete(rx);
        return;
    }

    audio_codec_i2s_cfg_t i2s_config = {
        .port = I2S_NUM_0,
        .rx_handle = rx,
        .tx_handle = nullptr,
    };
    const audio_codec_data_if_t* data_if = audio_codec_new_i2s_data(&i2s_config);
    audio_codec_i2c_cfg_t i2c_config = {
        .port = YAKA_I2C_PORT,
        .addr = YAKA_ES7210_CODEC_ADDRESS,
        .bus_handle = i2c_bus,
    };
    const audio_codec_ctrl_if_t* ctrl_if = audio_codec_new_i2c_ctrl(&i2c_config);
    es7210_codec_cfg_t codec_config = {};
    codec_config.ctrl_if = ctrl_if;
    codec_config.mic_selected = ES7210_SEL_MIC1 | ES7210_SEL_MIC2;
    const audio_codec_if_t* codec_if =
        ctrl_if == nullptr ? nullptr : es7210_codec_new(&codec_config);
    esp_codec_dev_cfg_t dev_config = {
        .dev_type = ESP_CODEC_DEV_TYPE_IN,
        .codec_if = codec_if,
        .data_if = data_if,
    };
    esp_codec_dev_handle_t dev =
        data_if == nullptr || codec_if == nullptr ? nullptr : esp_codec_dev_new(&dev_config);
    int open_result = -1;
    if (dev != nullptr) {
        esp_codec_dev_sample_info_t sample_info = {
            .bits_per_sample = 16,
            .channel = 2,
            .channel_mask = ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0) | ESP_CODEC_DEV_MAKE_CHANNEL_MASK(1),
            .sample_rate = kSampleRate,
            .mclk_multiple = 0,
        };
        open_result = esp_codec_dev_open(dev, &sample_info);
    }

    esp_err_t read_result = ESP_ERR_INVALID_STATE;
    std::size_t bytes_read = 0;
    if (open_result == ESP_CODEC_DEV_OK) {
        for (int pass = 0; pass < 5; ++pass) {
            int gain0 = ESP_CODEC_DEV_OK;
            int gain1 = ESP_CODEC_DEV_OK;
            if (pass == 1) {
                gain0 = esp_codec_dev_set_in_channel_gain(dev, ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0),
                                                          24.0f);
                gain1 = esp_codec_dev_set_in_channel_gain(dev, ESP_CODEC_DEV_MAKE_CHANNEL_MASK(1),
                                                          24.0f);
            }
            if (pass == 3) {
                ESP_LOGI(
                    kTag, "B GPIO13 weak pulldown set=%s",
                    esp_err_to_name(gpio_set_pull_mode(YAKA_AUDIO_DIN_GPIO, GPIO_PULLDOWN_ONLY)));
            } else if (pass == 4) {
                ESP_LOGI(
                    kTag, "B GPIO13 weak pullup set=%s",
                    esp_err_to_name(gpio_set_pull_mode(YAKA_AUDIO_DIN_GPIO, GPIO_PULLUP_ONLY)));
            }
            vTaskDelay(pdMS_TO_TICKS(pass == 2 ? 10000 : 150));
            std::array<int16_t, kSamplesPerBlock> samples{};
            bytes_read = 0;
            read_result = i2s_channel_read(rx, samples.data(), sizeof(samples), &bytes_read,
                                           pdMS_TO_TICKS(300));
            std::size_t nonzero = 0;
            std::size_t plus_one = 0;
            std::size_t minus_one = 0;
            int peak = 0;
            if (read_result == ESP_OK) {
                for (std::size_t i = 0; i < bytes_read / sizeof(int16_t); ++i) {
                    nonzero += samples[i] != 0;
                    plus_one += samples[i] == 1;
                    minus_one += samples[i] == -1;
                    peak = std::max(peak, std::abs(static_cast<int>(samples[i])));
                }
            }
            int chip_state = -1;
            const int state_result = esp_codec_dev_read_reg(dev, 0x0B, &chip_state);
            ESP_LOGI(kTag,
                     "B ES7210 RX-only phase=%d set=%d,%d state=%d:0x%02X gpio13=%d read=%s "
                     "bytes=%u nonzero=%u +1=%u -1=%u peak=%d first=%d,%d,%d,%d",
                     pass, gain0, gain1, state_result, chip_state & 0xFF,
                     gpio_get_level(YAKA_AUDIO_DIN_GPIO), esp_err_to_name(read_result),
                     static_cast<unsigned>(bytes_read), static_cast<unsigned>(nonzero),
                     static_cast<unsigned>(plus_one), static_cast<unsigned>(minus_one), peak,
                     samples[0], samples[1], samples[2], samples[3]);
        }
        ESP_LOGI(kTag, "B GPIO13 restore floating=%s",
                 esp_err_to_name(gpio_set_pull_mode(YAKA_AUDIO_DIN_GPIO, GPIO_FLOATING)));
    }
    ESP_LOGI(kTag, "B ES7210 RX-only open=%d", open_result);

    if (dev != nullptr) {
        if (open_result == ESP_CODEC_DEV_OK)
            esp_codec_dev_close(dev);
        esp_codec_dev_delete(dev);
    }
    if (codec_if != nullptr)
        audio_codec_delete_codec_if(codec_if);
    if (ctrl_if != nullptr)
        audio_codec_delete_ctrl_if(ctrl_if);
    if (data_if != nullptr)
        audio_codec_delete_data_if(data_if);
    StopAndDelete(rx);
}

}  // namespace

void HiYaKaRunAudioAbDiagnostics(i2c_master_bus_handle_t i2c_bus) {
    ESP_LOGW(kTag, "audio A/B starts: PA forced off; no speaker test");
    RunInternalLoopback();
    RunRxOnly(i2c_bus);
    ESP_LOGW(kTag, "audio A/B finished; normal lab codec initialization follows");
}

#else

void HiYaKaRunAudioAbDiagnostics(i2c_master_bus_handle_t) {}

#endif
