#ifndef HI_YAKA_EXPANDER_LCD_IO_H_
#define HI_YAKA_EXPANDER_LCD_IO_H_

#include "hi_yaka_pca9557.h"

#include <esp_lcd_panel_io.h>

esp_err_t hi_yaka_new_expander_lcd_io(HiYaKaPca9557* expander,
                                      int clock_hz,
                                      esp_lcd_panel_io_handle_t* out_io);

#endif  // HI_YAKA_EXPANDER_LCD_IO_H_
