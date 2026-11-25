#ifndef HI_YAKA_EXPANDER_LCD_IO_H_
#define HI_YAKA_EXPANDER_LCD_IO_H_

#include "hi_yaka_pca9557.h"

#include <esp_lcd_panel_io.h>

enum class HiYaKaLcdFailureStage : uint8_t {
    kNone,
    kCaset,
    kRaset,
    kRamwr,
    kCsRelease,
    kOther,
};

struct HiYaKaLcdFailureSnapshot {
    uint32_t sequence = 0;
    HiYaKaLcdFailureStage stage = HiYaKaLcdFailureStage::kNone;
    esp_err_t error = ESP_OK;
    uint16_t x_start = 0;
    uint16_t x_end = 0;
    uint16_t y_start = 0;
    uint16_t y_end = 0;
    bool has_columns = false;
    bool has_rows = false;
};

struct HiYaKaLcdRefreshStats {
    uint32_t completed = 0;
    uint32_t failed = 0;
    uint32_t slow_over_50ms = 0;
    uint32_t last_us = 0;
    uint32_t max_us = 0;
    HiYaKaLcdFailureSnapshot last_failure;
};

esp_err_t hi_yaka_lcd_refresh_stats(esp_lcd_panel_io_handle_t io,
                                    HiYaKaLcdRefreshStats* out);

esp_err_t hi_yaka_new_expander_lcd_io(HiYaKaPca9557* expander,
                                      int clock_hz,
                                      esp_lcd_panel_io_handle_t* out_io);

#endif  // HI_YAKA_EXPANDER_LCD_IO_H_
