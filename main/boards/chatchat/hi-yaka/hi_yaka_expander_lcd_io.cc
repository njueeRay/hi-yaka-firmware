#include "hi_yaka_expander_lcd_io.h"

#include "config.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <mutex>
#include <new>

#include <driver/parlio_tx.h>
#if CONFIG_HI_YAKA_LCD_GPIO_DIAGNOSTIC
#include <driver/gpio.h>
#include <esp_rom_sys.h>
#if CONFIG_HI_YAKA_LCD_GPIO_REGISTER_FAST_DIAGNOSTIC
#include <soc/gpio_reg.h>
#include <soc/soc.h>
#endif
#endif
#include <esp_heap_caps.h>
#include <esp_lcd_panel_commands.h>
#include <esp_lcd_panel_io_interface.h>
#include <esp_timer.h>

namespace {

constexpr size_t kSerialChunkBytes =
    YAKA_LCD_WIDTH * 20 * sizeof(uint16_t);
constexpr size_t kEncodedBytesPerSerialByte = 4;
constexpr size_t kEncodedChunkBytes =
    kSerialChunkBytes * kEncodedBytesPerSerialByte;

static_assert(kEncodedChunkBytes % sizeof(uint32_t) == 0);

struct ExpanderLcdIo {
    esp_lcd_panel_io_t base{};
    parlio_tx_unit_handle_t tx_unit = nullptr;
    HiYaKaPca9557* expander = nullptr;
    uint8_t* encoded_buffer = nullptr;
    esp_lcd_panel_io_callbacks_t callbacks{};
    void* callback_context = nullptr;
    bool flush_pending = false;
    int64_t refresh_started_us = 0;
    std::atomic<uint32_t> refresh_completed{0};
    std::atomic<uint32_t> refresh_failed{0};
    std::atomic<uint32_t> refresh_slow{0};
    std::atomic<uint32_t> refresh_last_us{0};
    std::atomic<uint32_t> refresh_max_us{0};
    HiYaKaLcdFailureSnapshot last_failure;
    uint16_t window_x_start = 0;
    uint16_t window_x_end = 0;
    uint16_t window_y_start = 0;
    uint16_t window_y_end = 0;
    bool window_has_columns = false;
    bool window_has_rows = false;
    std::mutex mutex;
};

ExpanderLcdIo* ToContext(esp_lcd_panel_io_t* io) {
    return reinterpret_cast<ExpanderLcdIo*>(io);
}

#if !CONFIG_HI_YAKA_LCD_GPIO_DIAGNOSTIC
void EncodeSerialBytes(const uint8_t* source, size_t size, uint8_t* destination) {
    // ESP32-C6 PARLIO cannot reliably terminate sub-32-bit DMA transfers. Use
    // a four-bit parallel bus with only data line 0 connected. Each source
    // bit becomes bit 0 of one four-bit bus cycle, so one serial byte becomes
    // exactly 32 DMA bits while still producing exactly eight LCD clocks.
    for (size_t index = 0; index < size; ++index) {
        const uint8_t value = source[index];
        destination[index * 4] =
            ((value & 0x80U) != 0 ? 0x10U : 0x00U) |
            ((value & 0x40U) != 0 ? 0x01U : 0x00U);
        destination[index * 4 + 1] =
            ((value & 0x20U) != 0 ? 0x10U : 0x00U) |
            ((value & 0x10U) != 0 ? 0x01U : 0x00U);
        destination[index * 4 + 2] =
            ((value & 0x08U) != 0 ? 0x10U : 0x00U) |
            ((value & 0x04U) != 0 ? 0x01U : 0x00U);
        destination[index * 4 + 3] =
            ((value & 0x02U) != 0 ? 0x10U : 0x00U) |
            ((value & 0x01U) != 0 ? 0x01U : 0x00U);
    }
}
#endif

esp_err_t Transmit(ExpanderLcdIo* context, const void* data, size_t size) {
    if (size == 0) {
        return ESP_OK;
    }
    if (context == nullptr || data == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    const auto* source = static_cast<const uint8_t*>(data);
#if CONFIG_HI_YAKA_LCD_GPIO_DIAGNOSTIC
#if CONFIG_HI_YAKA_LCD_GPIO_REGISTER_FAST_DIAGNOSTIC
    static_assert(YAKA_LCD_MOSI_GPIO == GPIO_NUM_6 &&
                  YAKA_LCD_SCLK_GPIO == GPIO_NUM_7,
                  "Register-fast LCD output is only audited for C6 GPIO6/7");
    static_assert(YAKA_LCD_MOSI_GPIO < 32 && YAKA_LCD_SCLK_GPIO < 32,
                  "W1TS/W1TC low-bank registers only address GPIO0-31");
    constexpr uint32_t kDataMask = 1U << YAKA_LCD_MOSI_GPIO;
    constexpr uint32_t kClockMask = 1U << YAKA_LCD_SCLK_GPIO;
    // Match the proven Waveshare GPIO-fast edge order: clock low, data
    // stable, clock high. W1TS/W1TC affect only the selected pins, never
    // read-modify-write the other GPIO outputs.
    for (size_t index = 0; index < size; ++index) {
        for (int bit = 7; bit >= 0; --bit) {
            REG_WRITE(GPIO_OUT_W1TC_REG, kClockMask);
            REG_WRITE((source[index] >> bit) & 1U
                          ? GPIO_OUT_W1TS_REG : GPIO_OUT_W1TC_REG,
                      kDataMask);
            REG_WRITE(GPIO_OUT_W1TS_REG, kClockMask);
        }
    }
    REG_WRITE(GPIO_OUT_W1TC_REG, kClockMask | kDataMask);
#else
    // ST7789V latches SDA on SCL's rising edge. Three separate gpio_set_level
    // calls per bit preserve data-before-clock ordering. Function-call and
    // loop overhead is expected to exceed the panel's 10 ns data setup,
    // 15 ns SCL high/low and 66 ns cycle minima; pad timing is not measured.
    for (size_t index = 0; index < size; ++index) {
        for (int bit = 7; bit >= 0; --bit) {
            gpio_set_level(YAKA_LCD_MOSI_GPIO, (source[index] >> bit) & 1U);
#if !CONFIG_HI_YAKA_LCD_GPIO_FAST_DIAGNOSTIC
            esp_rom_delay_us(1);
#endif
            gpio_set_level(YAKA_LCD_SCLK_GPIO, 1);
#if !CONFIG_HI_YAKA_LCD_GPIO_FAST_DIAGNOSTIC
            esp_rom_delay_us(1);
#endif
            gpio_set_level(YAKA_LCD_SCLK_GPIO, 0);
        }
    }
    gpio_set_level(YAKA_LCD_MOSI_GPIO, 0);
#endif
    return ESP_OK;
#else
    if (context->tx_unit == nullptr || context->encoded_buffer == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    parlio_transmit_config_t transmit_config = {};
    transmit_config.idle_value = 0;

    while (size > 0) {
        const size_t chunk_size = std::min(size, kSerialChunkBytes);
        EncodeSerialBytes(source, chunk_size, context->encoded_buffer);
        esp_err_t result = parlio_tx_unit_transmit(
            context->tx_unit, context->encoded_buffer, chunk_size * 32,
            &transmit_config);
        if (result == ESP_OK) {
            result = parlio_tx_unit_wait_all_done(context->tx_unit, 250);
        }
        if (result != ESP_OK) {
            return result;
        }
        source += chunk_size;
        size -= chunk_size;
    }
    return ESP_OK;
#endif
}

esp_err_t FinishTransaction(ExpanderLcdIo* context, esp_err_t result) {
    const esp_err_t deselect = context->expander->SetLcdControl(false, true);
    return result == ESP_OK ? deselect : result;
}

void CaptureWindow(ExpanderLcdIo* context, int lcd_cmd, const void* param,
                   size_t param_size) {
    if (lcd_cmd == LCD_CMD_CASET) {
        context->window_has_columns = false;
        context->window_has_rows = false;
    }
    if (param == nullptr || param_size != 4 ||
        (lcd_cmd != LCD_CMD_CASET && lcd_cmd != LCD_CMD_RASET)) {
        return;
    }
    const auto* bytes = static_cast<const uint8_t*>(param);
    const uint16_t start = static_cast<uint16_t>((bytes[0] << 8) | bytes[1]);
    const uint16_t end = static_cast<uint16_t>((bytes[2] << 8) | bytes[3]);
    if (lcd_cmd == LCD_CMD_CASET) {
        context->window_x_start = start;
        context->window_x_end = end;
        context->window_has_columns = true;
    } else {
        context->window_y_start = start;
        context->window_y_end = end;
        context->window_has_rows = true;
    }
}

void RecordFailure(ExpanderLcdIo* context, HiYaKaLcdFailureStage stage,
                   esp_err_t error) {
    auto& snapshot = context->last_failure;
    ++snapshot.sequence;
    snapshot.stage = stage;
    snapshot.error = error;
    snapshot.x_start = context->window_x_start;
    snapshot.x_end = context->window_x_end;
    snapshot.y_start = context->window_y_start;
    snapshot.y_end = context->window_y_end;
    snapshot.has_columns = context->window_has_columns;
    snapshot.has_rows = context->window_has_rows;
}

void NotifyColorDone(ExpanderLcdIo* context) {
    if (context->callbacks.on_color_trans_done != nullptr) {
        esp_lcd_panel_io_event_data_t event = {};
        context->callbacks.on_color_trans_done(
            &context->base, &event, context->callback_context);
    }
}

esp_err_t TxParam(esp_lcd_panel_io_t* io, int lcd_cmd, const void* param,
                  size_t param_size) {
    if (io == nullptr || (param_size > 0 && param == nullptr)) {
        return ESP_ERR_INVALID_ARG;
    }
    auto* context = ToContext(io);
    esp_err_t result = ESP_OK;
    bool release_failed_flush = false;
    {
        std::lock_guard<std::mutex> lock(context->mutex);
        if (lcd_cmd == LCD_CMD_CASET) {
            // ST7789 draw_bitmap starts every LVGL flush with CASET, followed
            // by RASET and a RAMWR color transaction. Remember that sequence
            // so a failed parameter transfer can release LVGL even though the
            // driver aborts before reaching TxColor.
            context->flush_pending = true;
            context->refresh_started_us = esp_timer_get_time();
        }
        if (context->flush_pending) {
            CaptureWindow(context, lcd_cmd, param, param_size);
        }

        result = context->expander->SetLcdControl(true, false);
        if (result == ESP_OK && lcd_cmd >= 0) {
            const uint8_t command = static_cast<uint8_t>(lcd_cmd);
            result = Transmit(context, &command, sizeof(command));
        }
        if (result == ESP_OK && param_size > 0) {
            result = context->expander->SetLcdControl(true, true);
            if (result == ESP_OK) {
                result = Transmit(context, param, param_size);
            }
        }
        const esp_err_t transfer_result = result;
        result = FinishTransaction(context, result);
        if (result != ESP_OK && context->flush_pending) {
            context->refresh_failed.fetch_add(1);
            RecordFailure(context,
                          transfer_result == ESP_OK
                              ? HiYaKaLcdFailureStage::kCsRelease
                              : lcd_cmd == LCD_CMD_CASET
                                    ? HiYaKaLcdFailureStage::kCaset
                                    : lcd_cmd == LCD_CMD_RASET
                                          ? HiYaKaLcdFailureStage::kRaset
                                          : HiYaKaLcdFailureStage::kOther,
                          result);
            context->flush_pending = false;
            context->refresh_started_us = 0;
            release_failed_flush = true;
        }
    }
    if (release_failed_flush) {
        NotifyColorDone(context);
    }
    return result;
}

esp_err_t TxColor(esp_lcd_panel_io_t* io, int lcd_cmd, const void* color,
                  size_t color_size) {
    if (io == nullptr || (color_size > 0 && color == nullptr)) {
        return ESP_ERR_INVALID_ARG;
    }
    auto* context = ToContext(io);
    esp_err_t result = ESP_OK;
    {
        std::lock_guard<std::mutex> lock(context->mutex);
        result = context->expander->SetLcdControl(true, false);
        if (result == ESP_OK && lcd_cmd >= 0) {
            const uint8_t command = static_cast<uint8_t>(lcd_cmd);
            result = Transmit(context, &command, sizeof(command));
        }
        if (result == ESP_OK && color_size > 0) {
            result = context->expander->SetLcdControl(true, true);
            if (result == ESP_OK) {
                result = Transmit(context, color, color_size);
            }
        }
        const esp_err_t transfer_result = result;
        result = FinishTransaction(context, result);
        if (context->refresh_started_us > 0) {
            const uint32_t elapsed_us = static_cast<uint32_t>(
                esp_timer_get_time() - context->refresh_started_us);
            context->refresh_last_us.store(elapsed_us);
            uint32_t previous = context->refresh_max_us.load();
            while (elapsed_us > previous &&
                   !context->refresh_max_us.compare_exchange_weak(previous, elapsed_us)) {}
            if (elapsed_us > 50000) context->refresh_slow.fetch_add(1);
            if (result == ESP_OK) context->refresh_completed.fetch_add(1);
            else {
                context->refresh_failed.fetch_add(1);
                RecordFailure(context,
                              transfer_result == ESP_OK
                                  ? HiYaKaLcdFailureStage::kCsRelease
                                  : HiYaKaLcdFailureStage::kRamwr,
                              result);
            }
            context->refresh_started_us = 0;
        }
        context->flush_pending = false;
    }

    // This implementation is deliberately synchronous because CS and DC are
    // I2C-expander outputs. PARLIO provides the serial data clock without
    // consuming ESP32-C6's only user SPI controller, leaving SPI2 for TF.
    // LVGL must receive the flush-complete callback even when the underlying
    // transfer fails. Otherwise lv_refr waits forever and taskLVGL starves the
    // idle task. The return value still preserves the transport failure.
    NotifyColorDone(context);
    return result;
}

esp_err_t RxParam(esp_lcd_panel_io_t*, int, void*, size_t) {
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t RegisterCallbacks(esp_lcd_panel_io_t* io,
                            const esp_lcd_panel_io_callbacks_t* callbacks,
                            void* user_context) {
    if (io == nullptr || callbacks == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    auto* context = ToContext(io);
    std::lock_guard<std::mutex> lock(context->mutex);
    context->callbacks = *callbacks;
    context->callback_context = user_context;
    return ESP_OK;
}

esp_err_t DeleteIo(esp_lcd_panel_io_t* io) {
    if (io == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    auto* context = ToContext(io);
    esp_err_t result = ESP_OK;
    if (context->tx_unit != nullptr) {
        result = parlio_tx_unit_disable(context->tx_unit);
        if (result == ESP_OK) {
            result = parlio_del_tx_unit(context->tx_unit);
        }
    }
    free(context->encoded_buffer);
    delete context;
    return result;
}

}  // namespace

esp_err_t hi_yaka_lcd_refresh_stats(esp_lcd_panel_io_handle_t io,
                                    HiYaKaLcdRefreshStats* out) {
    if (io == nullptr || out == nullptr) return ESP_ERR_INVALID_ARG;
    auto* context = ToContext(reinterpret_cast<esp_lcd_panel_io_t*>(io));
    out->completed = context->refresh_completed.load();
    out->failed = context->refresh_failed.load();
    out->slow_over_50ms = context->refresh_slow.load();
    out->last_us = context->refresh_last_us.load();
    out->max_us = context->refresh_max_us.load();
    {
        std::lock_guard<std::mutex> lock(context->mutex);
        out->last_failure = context->last_failure;
    }
    return ESP_OK;
}

esp_err_t hi_yaka_new_expander_lcd_io(HiYaKaPca9557* expander,
                                      int clock_hz,
                                      esp_lcd_panel_io_handle_t* out_io) {
    if (expander == nullptr || !expander->ready() || clock_hz <= 0 || out_io == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    auto* context = new (std::nothrow) ExpanderLcdIo();
    if (context == nullptr) {
        return ESP_ERR_NO_MEM;
    }
    context->expander = expander;
#if CONFIG_HI_YAKA_LCD_GPIO_DIAGNOSTIC
    gpio_config_t gpio_setup = {};
    gpio_setup.pin_bit_mask = (1ULL << YAKA_LCD_MOSI_GPIO) | (1ULL << YAKA_LCD_SCLK_GPIO);
    gpio_setup.mode = GPIO_MODE_OUTPUT;
    gpio_setup.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_setup.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_setup.intr_type = GPIO_INTR_DISABLE;
    esp_err_t result = gpio_config(&gpio_setup);
    if (result == ESP_OK) {
        result = gpio_set_level(YAKA_LCD_SCLK_GPIO, 0);
    }
    if (result == ESP_OK) {
        result = gpio_set_level(YAKA_LCD_MOSI_GPIO, 0);
    }
    if (result != ESP_OK) {
        delete context;
        return result;
    }
#else
    context->encoded_buffer = static_cast<uint8_t*>(heap_caps_aligned_alloc(
        sizeof(uint32_t), kEncodedChunkBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
    if (context->encoded_buffer == nullptr) {
        delete context;
        return ESP_ERR_NO_MEM;
    }
#endif
    context->base.rx_param = RxParam;
    context->base.tx_param = TxParam;
    context->base.tx_color = TxColor;
    context->base.del = DeleteIo;
    context->base.register_event_callbacks = RegisterCallbacks;

#if !CONFIG_HI_YAKA_LCD_GPIO_DIAGNOSTIC
    parlio_tx_unit_config_t tx_config = {};
    tx_config.clk_src = PARLIO_CLK_SRC_DEFAULT;
    tx_config.clk_in_gpio_num = GPIO_NUM_NC;
    tx_config.output_clk_freq_hz = clock_hz;
    tx_config.data_width = 4;
    for (auto& gpio : tx_config.data_gpio_nums) {
        gpio = GPIO_NUM_NC;
    }
    tx_config.data_gpio_nums[0] = YAKA_LCD_MOSI_GPIO;
    tx_config.clk_out_gpio_num = YAKA_LCD_SCLK_GPIO;
    tx_config.valid_gpio_num = GPIO_NUM_NC;
    tx_config.trans_queue_depth = 1;
    tx_config.max_transfer_size = kEncodedChunkBytes;
    tx_config.dma_burst_size = 64;
    tx_config.shift_edge = PARLIO_SHIFT_EDGE_NEG;
    tx_config.bit_pack_order = PARLIO_BIT_PACK_ORDER_MSB;

    esp_err_t result = parlio_new_tx_unit(&tx_config, &context->tx_unit);
    if (result == ESP_OK) {
        result = parlio_tx_unit_enable(context->tx_unit);
    }
    if (result != ESP_OK) {
        if (context->tx_unit != nullptr) {
            parlio_del_tx_unit(context->tx_unit);
        }
        free(context->encoded_buffer);
        delete context;
        return result;
    }
#endif

    *out_io = &context->base;
    return ESP_OK;
}
