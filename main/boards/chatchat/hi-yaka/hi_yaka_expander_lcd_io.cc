#include "hi_yaka_expander_lcd_io.h"

#include "config.h"

#include <mutex>
#include <new>

#include <driver/parlio_tx.h>
#include <esp_lcd_panel_io_interface.h>

namespace {

struct ExpanderLcdIo {
    esp_lcd_panel_io_t base{};
    parlio_tx_unit_handle_t tx_unit = nullptr;
    HiYaKaPca9557* expander = nullptr;
    esp_lcd_panel_io_callbacks_t callbacks{};
    void* callback_context = nullptr;
    std::mutex mutex;
};

ExpanderLcdIo* ToContext(esp_lcd_panel_io_t* io) {
    return reinterpret_cast<ExpanderLcdIo*>(io);
}

esp_err_t Transmit(parlio_tx_unit_handle_t tx_unit, const void* data, size_t size) {
    if (size == 0) {
        return ESP_OK;
    }
    if (tx_unit == nullptr || data == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    parlio_transmit_config_t transmit_config = {};
    transmit_config.idle_value = 0;
    esp_err_t result =
        parlio_tx_unit_transmit(tx_unit, data, size * 8, &transmit_config);
    if (result == ESP_OK) {
        result = parlio_tx_unit_wait_all_done(tx_unit, 250);
    }
    return result;
}

esp_err_t FinishTransaction(ExpanderLcdIo* context, esp_err_t result) {
    const esp_err_t deselect = context->expander->SetLcdControl(false, true);
    return result == ESP_OK ? deselect : result;
}

esp_err_t TxParam(esp_lcd_panel_io_t* io, int lcd_cmd, const void* param,
                  size_t param_size) {
    if (io == nullptr || (param_size > 0 && param == nullptr)) {
        return ESP_ERR_INVALID_ARG;
    }
    auto* context = ToContext(io);
    std::lock_guard<std::mutex> lock(context->mutex);

    esp_err_t result = context->expander->SetLcdControl(true, false);
    if (result == ESP_OK && lcd_cmd >= 0) {
        const uint8_t command = static_cast<uint8_t>(lcd_cmd);
        result = Transmit(context->tx_unit, &command, sizeof(command));
    }
    if (result == ESP_OK && param_size > 0) {
        result = context->expander->SetLcdControl(true, true);
        if (result == ESP_OK) {
            result = Transmit(context->tx_unit, param, param_size);
        }
    }
    return FinishTransaction(context, result);
}

esp_err_t TxColor(esp_lcd_panel_io_t* io, int lcd_cmd, const void* color,
                  size_t color_size) {
    if (io == nullptr || (color_size > 0 && color == nullptr)) {
        return ESP_ERR_INVALID_ARG;
    }
    auto* context = ToContext(io);
    {
        std::lock_guard<std::mutex> lock(context->mutex);
        esp_err_t result = context->expander->SetLcdControl(true, false);
        if (result == ESP_OK && lcd_cmd >= 0) {
            const uint8_t command = static_cast<uint8_t>(lcd_cmd);
            result = Transmit(context->tx_unit, &command, sizeof(command));
        }
        if (result == ESP_OK && color_size > 0) {
            result = context->expander->SetLcdControl(true, true);
            if (result == ESP_OK) {
                result = Transmit(context->tx_unit, color, color_size);
            }
        }
        result = FinishTransaction(context, result);
        if (result != ESP_OK) {
            return result;
        }
    }

    // This implementation is deliberately synchronous because CS and DC are
    // I2C-expander outputs. PARLIO provides the serial data clock without
    // consuming ESP32-C6's only user SPI controller, leaving SPI2 for TF.
    if (context->callbacks.on_color_trans_done != nullptr) {
        esp_lcd_panel_io_event_data_t event = {};
        context->callbacks.on_color_trans_done(io, &event, context->callback_context);
    }
    return ESP_OK;
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
    delete context;
    return result;
}

}  // namespace

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
    context->base.rx_param = RxParam;
    context->base.tx_param = TxParam;
    context->base.tx_color = TxColor;
    context->base.del = DeleteIo;
    context->base.register_event_callbacks = RegisterCallbacks;

    parlio_tx_unit_config_t tx_config = {};
    tx_config.clk_src = PARLIO_CLK_SRC_DEFAULT;
    tx_config.clk_in_gpio_num = GPIO_NUM_NC;
    tx_config.output_clk_freq_hz = clock_hz;
    tx_config.data_width = 1;
    for (auto& gpio : tx_config.data_gpio_nums) {
        gpio = GPIO_NUM_NC;
    }
    tx_config.data_gpio_nums[0] = YAKA_LCD_MOSI_GPIO;
    tx_config.clk_out_gpio_num = YAKA_LCD_SCLK_GPIO;
    tx_config.valid_gpio_num = GPIO_NUM_NC;
    tx_config.trans_queue_depth = 1;
    tx_config.max_transfer_size = YAKA_LCD_WIDTH * 20 * sizeof(uint16_t);
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
        delete context;
        return result;
    }

    *out_io = &context->base;
    return ESP_OK;
}
