#ifndef HI_YAKA_LCD_MENU_H_
#define HI_YAKA_LCD_MENU_H_

#include "hi_yaka_pca9557.h"
#include <esp_lcd_panel_io.h>

// Lab-only LVGL overlay. The product UI is unchanged when the menu Kconfig is off.
void hi_yaka_start_lcd_menu_diagnostic(esp_lcd_panel_io_handle_t io,
                                       HiYaKaPca9557* expander);

#endif
