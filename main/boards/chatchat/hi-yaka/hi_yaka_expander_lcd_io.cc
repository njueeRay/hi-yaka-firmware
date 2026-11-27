#include "hi_yaka_expander_lcd_io.h"

#include "config.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <new>

#include <driver/parlio_tx.h>
#include <driver/gpio.h>
#if CONFIG_HI_YAKA_LCD_SPI_DMA_DIAGNOSTIC
#include <driver/spi_master.h>
#endif
#if CONFIG_HI_YAKA_LCD_GPIO_DIAGNOSTIC
#if CONFIG_HI_YAKA_LCD_GPIO_REGISTER_FAST_DIAGNOSTIC
#include <soc/gpio_reg.h>
#include <soc/soc.h>
#endif
#endif
#if !CONFIG_HI_YAKA_LCD_SPI_DMA_DIAGNOSTIC
#include <esp_rom_sys.h>
#endif
#if !CONFIG_HI_YAKA_LCD_SPI_DMA_DIAGNOSTIC && !CONFIG_HI_YAKA_LCD_GPIO_DIAGNOSTIC
#include <esp_rom_gpio.h>
#include <soc/gpio_sig_map.h>
#endif
#include <esp_heap_caps.h>
#include <esp_lcd_panel_commands.h>
#include <esp_lcd_panel_io_interface.h>
#include <esp_timer.h>

#if CONFIG_HI_YAKA_LCD_SPI_DMA_DIAGNOSTIC && CONFIG_HI_YAKA_ENABLE_STORAGE
#error "HiYaKa SPI2 LCD diagnostic cannot coexist with TF/SDSPI on different pins"
#endif

namespace {

constexpr size_t kTransportChunkBytes =
    YAKA_LCD_WIDTH * 20 * sizeof(uint16_t);
static_assert(kTransportChunkBytes % sizeof(uint32_t) == 0);
#if CONFIG_HI_YAKA_LCD_PARLIO_ENCODED_DIAGNOSTIC
constexpr size_t kTransportBufferBytes = kTransportChunkBytes * 4;
#else
constexpr size_t kTransportBufferBytes = kTransportChunkBytes;
#endif
static_assert(kTransportBufferBytes % sizeof(uint32_t) == 0);

struct ExpanderLcdIo {
    esp_lcd_panel_io_t base{};
    parlio_tx_unit_handle_t tx_unit = nullptr;
#if CONFIG_HI_YAKA_LCD_SPI_DMA_DIAGNOSTIC
    spi_device_handle_t spi_device = nullptr;
    bool spi_bus_initialized = false;
#endif
    HiYaKaPca9557* expander = nullptr;
    int clock_hz = 0;
    uint8_t* transport_buffer = nullptr;
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

#if !CONFIG_HI_YAKA_LCD_SPI_DMA_DIAGNOSTIC
esp_err_t PrepareGpioOutput(bool configure_pins) {
    esp_err_t result = ESP_OK;
    if (configure_pins) {
        gpio_config_t gpio_setup = {};
        gpio_setup.pin_bit_mask =
            (1ULL << YAKA_LCD_MOSI_GPIO) | (1ULL << YAKA_LCD_SCLK_GPIO);
        gpio_setup.mode = GPIO_MODE_OUTPUT;
        gpio_setup.pull_up_en = GPIO_PULLUP_DISABLE;
        gpio_setup.pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_setup.intr_type = GPIO_INTR_DISABLE;
        result = gpio_config(&gpio_setup);
    }
#if !CONFIG_HI_YAKA_LCD_GPIO_DIAGNOSTIC
    if (result == ESP_OK) {
        esp_rom_gpio_connect_out_signal(
            YAKA_LCD_MOSI_GPIO, SIG_GPIO_OUT_IDX, false, false);
        esp_rom_gpio_connect_out_signal(
            YAKA_LCD_SCLK_GPIO, SIG_GPIO_OUT_IDX, false, false);
    }
#endif
    if (result == ESP_OK) result = gpio_set_level(YAKA_LCD_SCLK_GPIO, 0);
    if (result == ESP_OK) result = gpio_set_level(YAKA_LCD_MOSI_GPIO, 0);
    return result;
}

esp_err_t TransmitGpioBytes(const uint8_t* source, size_t size,
                            bool slow_edges) {
    if (source == nullptr && size > 0) return ESP_ERR_INVALID_ARG;
    for (size_t index = 0; index < size; ++index) {
        for (int bit = 7; bit >= 0; --bit) {
            gpio_set_level(YAKA_LCD_MOSI_GPIO, (source[index] >> bit) & 1U);
            if (slow_edges) esp_rom_delay_us(1);
            gpio_set_level(YAKA_LCD_SCLK_GPIO, 1);
            if (slow_edges) esp_rom_delay_us(1);
            gpio_set_level(YAKA_LCD_SCLK_GPIO, 0);
        }
    }
    gpio_set_level(YAKA_LCD_MOSI_GPIO, 0);
    return ESP_OK;
}
#endif

#if CONFIG_HI_YAKA_LCD_PARLIO_ENCODED_DIAGNOSTIC
void EncodeSerialBytes(const uint8_t* source, size_t size,
                       uint8_t* destination) {
    // Four PARLIO data bits form one bus cycle, but only data line 0 is wired
    // to LCD SDA. Packing each source bit into bit 0 of consecutive nibbles
    // yields exactly eight LCD clocks per source byte and avoids relying on
    // PARLIO valid for CS, which is unavailable behind PCA9557 on HiYaKa.
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

#if !CONFIG_HI_YAKA_LCD_SPI_DMA_DIAGNOSTIC && !CONFIG_HI_YAKA_LCD_GPIO_DIAGNOSTIC
esp_err_t ReleaseParlioToGpio(ExpanderLcdIo* context) {
    if (context->tx_unit != nullptr) {
        esp_err_t result = parlio_tx_unit_wait_all_done(context->tx_unit, 250);
        if (result == ESP_OK) result = parlio_tx_unit_disable(context->tx_unit);
        if (result == ESP_OK) result = parlio_del_tx_unit(context->tx_unit);
        if (result != ESP_OK) return result;
        context->tx_unit = nullptr;
    }
    // The pins were reserved once when the LCD IO was created. Re-running
    // gpio_config() after every PARLIO transaction reports a false conflict
    // against that same reservation. Only restore the GPIO matrix routing.
    return PrepareGpioOutput(false);
}

esp_err_t AcquireParlioForPixels(ExpanderLcdIo* context) {
    if (context->tx_unit != nullptr) return ESP_OK;
    parlio_tx_unit_config_t tx_config = {};
    tx_config.clk_src = PARLIO_CLK_SRC_DEFAULT;
    tx_config.clk_in_gpio_num = GPIO_NUM_NC;
    tx_config.output_clk_freq_hz = context->clock_hz;
    tx_config.data_width =
#if CONFIG_HI_YAKA_LCD_PARLIO_ENCODED_DIAGNOSTIC
        4;
#else
        1;
#endif
    for (auto& gpio : tx_config.data_gpio_nums) gpio = GPIO_NUM_NC;
    tx_config.data_gpio_nums[0] = YAKA_LCD_MOSI_GPIO;
    tx_config.clk_out_gpio_num = YAKA_LCD_SCLK_GPIO;
    tx_config.valid_gpio_num = GPIO_NUM_NC;
    tx_config.trans_queue_depth = 1;
    tx_config.max_transfer_size = kTransportBufferBytes;
    tx_config.dma_burst_size = 64;
    tx_config.shift_edge = PARLIO_SHIFT_EDGE_NEG;
    tx_config.bit_pack_order =
#if CONFIG_HI_YAKA_LCD_PARLIO_ENCODED_DIAGNOSTIC || CONFIG_HI_YAKA_LCD_PARLIO_MSB_DIAGNOSTIC
        PARLIO_BIT_PACK_ORDER_MSB;
#else
        PARLIO_BIT_PACK_ORDER_LSB;
#endif

    esp_err_t result = parlio_new_tx_unit(&tx_config, &context->tx_unit);
    if (result == ESP_OK) result = parlio_tx_unit_enable(context->tx_unit);
    if (result != ESP_OK && context->tx_unit != nullptr) {
        parlio_del_tx_unit(context->tx_unit);
        context->tx_unit = nullptr;
    }
    return result;
}
#endif

ExpanderLcdIo* ToContext(esp_lcd_panel_io_t* io) {
    return reinterpret_cast<ExpanderLcdIo*>(io);
}

esp_err_t Transmit(ExpanderLcdIo* context, const void* data, size_t size) {
    if (size == 0) {
        return ESP_OK;
    }
    if (context == nullptr || data == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    const auto* source = static_cast<const uint8_t*>(data);
#if CONFIG_HI_YAKA_LCD_SPI_DMA_DIAGNOSTIC
    if (context->spi_device == nullptr || context->transport_buffer == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    while (size > 0) {
        const size_t chunk_size = std::min(size, kTransportChunkBytes);
        std::memcpy(context->transport_buffer, source, chunk_size);
        spi_transaction_t transaction = {};
        transaction.length = chunk_size * 8;
        transaction.tx_buffer = context->transport_buffer;
        const esp_err_t result =
            spi_device_polling_transmit(context->spi_device, &transaction);
        if (result != ESP_OK) {
            return result;
        }
        source += chunk_size;
        size -= chunk_size;
    }
    return ESP_OK;
#elif CONFIG_HI_YAKA_LCD_GPIO_DIAGNOSTIC
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
    if (context->tx_unit == nullptr || context->transport_buffer == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    parlio_transmit_config_t transmit_config = {};
    transmit_config.idle_value = 0;

    while (size > 0) {
        const size_t chunk_size = std::min(size, kTransportChunkBytes);
#if CONFIG_HI_YAKA_LCD_PARLIO_ENCODED_DIAGNOSTIC
        EncodeSerialBytes(source, chunk_size, context->transport_buffer);
        const size_t transfer_bits = chunk_size * 32;
#else
        std::memcpy(context->transport_buffer, source, chunk_size);
        const size_t transfer_bits = chunk_size * 8;
#endif
        esp_err_t result = parlio_tx_unit_transmit(
            context->tx_unit, context->transport_buffer, transfer_bits,
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

esp_err_t TransmitCommandPath(ExpanderLcdIo* context, const void* data,
                              size_t size) {
#if !CONFIG_HI_YAKA_LCD_SPI_DMA_DIAGNOSTIC && !CONFIG_HI_YAKA_LCD_GPIO_DIAGNOSTIC
    // Keep ST7789 commands and parameters on the already proven GPIO path.
    // PARLIO diagnostics vary only the pixel payload, otherwise a rejected
    // command stream is indistinguishable from a bad pixel packing scheme.
    esp_err_t result = ReleaseParlioToGpio(context);
    if (result != ESP_OK) return result;
    return TransmitGpioBytes(static_cast<const uint8_t*>(data), size, false);
#else
    return Transmit(context, data, size);
#endif
}

esp_err_t TransmitColorPath(ExpanderLcdIo* context, const void* data,
                            size_t size) {
#if !CONFIG_HI_YAKA_LCD_SPI_DMA_DIAGNOSTIC && !CONFIG_HI_YAKA_LCD_GPIO_DIAGNOSTIC
    esp_err_t result = AcquireParlioForPixels(context);
    if (result != ESP_OK) return result;
    return Transmit(context, data, size);
#else
    return Transmit(context, data, size);
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
            result = TransmitCommandPath(context, &command, sizeof(command));
        }
        if (result == ESP_OK && param_size > 0) {
            result = context->expander->SetLcdControl(true, true);
            if (result == ESP_OK) {
                result = TransmitCommandPath(context, param, param_size);
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
            result = TransmitCommandPath(context, &command, sizeof(command));
        }
        if (result == ESP_OK && color_size > 0) {
#if CONFIG_HI_YAKA_LCD_PARLIO_CS_GUARD_DIAGNOSTIC
            // The reference board proved that PARLIO must not claim MOSI/SCLK
            // while the panel is selected. HiYaKa cannot route PARLIO valid
            // directly to CS because CS is behind PCA9557, so reproduce the
            // same effective window explicitly: end the GPIO RAMWR selection,
            // claim the data pins, and only then select the panel for pixels.
            result = context->expander->SetLcdControl(false, true);
            if (result == ESP_OK) result = AcquireParlioForPixels(context);
            if (result == ESP_OK) {
                result = context->expander->SetLcdControl(true, true);
            }
            if (result == ESP_OK) result = Transmit(context, color, color_size);
#else
            result = context->expander->SetLcdControl(true, true);
            if (result == ESP_OK) {
                result = TransmitColorPath(context, color, color_size);
            }
#endif
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
#if CONFIG_HI_YAKA_LCD_SPI_DMA_DIAGNOSTIC
    if (context->spi_device != nullptr) {
        result = spi_bus_remove_device(context->spi_device);
    }
    if (context->spi_bus_initialized) {
        const esp_err_t free_result = spi_bus_free(SPI2_HOST);
        if (result == ESP_OK) result = free_result;
    }
#else
    if (context->tx_unit != nullptr) {
        result = parlio_tx_unit_disable(context->tx_unit);
        if (result == ESP_OK) {
            result = parlio_del_tx_unit(context->tx_unit);
        }
    }
#endif
    free(context->transport_buffer);
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
    context->clock_hz = clock_hz;
#if !CONFIG_HI_YAKA_LCD_SPI_DMA_DIAGNOSTIC
    esp_err_t result = PrepareGpioOutput(true);
    if (result != ESP_OK) {
        delete context;
        return result;
    }
#endif
#if !CONFIG_HI_YAKA_LCD_GPIO_DIAGNOSTIC
    context->transport_buffer = static_cast<uint8_t*>(heap_caps_aligned_alloc(
        sizeof(uint32_t), kTransportBufferBytes,
        MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
    if (context->transport_buffer == nullptr) {
        delete context;
        return ESP_ERR_NO_MEM;
    }
#endif
    context->base.rx_param = RxParam;
    context->base.tx_param = TxParam;
    context->base.tx_color = TxColor;
    context->base.del = DeleteIo;
    context->base.register_event_callbacks = RegisterCallbacks;

#if CONFIG_HI_YAKA_LCD_SPI_DMA_DIAGNOSTIC
    spi_bus_config_t bus_config = {};
    bus_config.mosi_io_num = YAKA_LCD_MOSI_GPIO;
    bus_config.miso_io_num = GPIO_NUM_NC;
    bus_config.sclk_io_num = YAKA_LCD_SCLK_GPIO;
    bus_config.quadwp_io_num = GPIO_NUM_NC;
    bus_config.quadhd_io_num = GPIO_NUM_NC;
    bus_config.max_transfer_sz = kTransportChunkBytes;
    esp_err_t result =
        spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (result == ESP_OK) {
        context->spi_bus_initialized = true;
        spi_device_interface_config_t device_config = {};
        device_config.clock_speed_hz = clock_hz;
        device_config.mode = 0;
        device_config.spics_io_num = GPIO_NUM_NC;
        device_config.queue_size = 1;
        result = spi_bus_add_device(
            SPI2_HOST, &device_config, &context->spi_device);
    }
    if (result != ESP_OK) {
        if (context->spi_device != nullptr) {
            spi_bus_remove_device(context->spi_device);
        }
        if (context->spi_bus_initialized) {
            spi_bus_free(SPI2_HOST);
        }
        free(context->transport_buffer);
        delete context;
        return result;
    }
#endif

    *out_io = &context->base;
    return ESP_OK;
}
