#include "hi_yaka_lcd_menu.h"

#include "hi_yaka_expander_lcd_io.h"

#include <cstdio>
#include <esp_lvgl_port.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <lvgl.h>

#if CONFIG_HI_YAKA_LCD_MENU_DIAGNOSTIC
namespace {

struct MenuState {
    esp_lcd_panel_io_handle_t io;
    HiYaKaPca9557* expander;
    lv_obj_t* uptime;
    lv_obj_t* animation;
    lv_obj_t* timing;
    lv_obj_t* retries;
    uint32_t frame = 0;
    uint32_t last_second = UINT32_MAX;
};

void UpdateMenu(lv_timer_t* timer) {
    auto* state = static_cast<MenuState*>(lv_timer_get_user_data(timer));
    if (state == nullptr) return;

    // A bounded 48x48 animation exercises the same partial-refresh path as
    // the external menu demo without bringing its screen pins or SPI2 driver.
    constexpr uint32_t kColors[] = {0x00AEEA, 0x22C55E, 0xF59E0B, 0xEF4444};
    lv_obj_set_style_bg_color(state->animation,
                              lv_color_hex(kColors[(state->frame++) % 4]), 0);

    const uint32_t seconds = static_cast<uint32_t>(esp_timer_get_time() / 1000000);
    if (seconds == state->last_second) return;
    state->last_second = seconds;

    char line[64];
    std::snprintf(line, sizeof(line), "UPTIME %02lu:%02lu:%02lu",
                  static_cast<unsigned long>(seconds / 3600 % 100),
                  static_cast<unsigned long>(seconds / 60 % 60),
                  static_cast<unsigned long>(seconds % 60));
    lv_label_set_text(state->uptime, line);

    HiYaKaLcdRefreshStats stats{};
    if (hi_yaka_lcd_refresh_stats(state->io, &stats) == ESP_OK) {
        std::snprintf(line, sizeof(line), "LCD %lu/%lu ms  slow:%lu",
                      static_cast<unsigned long>(stats.last_us / 1000),
                      static_cast<unsigned long>(stats.max_us / 1000),
                      static_cast<unsigned long>(stats.slow_over_50ms));
        lv_label_set_text(state->timing, line);
        std::snprintf(line, sizeof(line), "PCA 1st/exh %lu/%lu  draw:%lu",
                      static_cast<unsigned long>(state->expander->first_write_failures()),
                      static_cast<unsigned long>(state->expander->exhausted_writes()),
                      static_cast<unsigned long>(stats.failed));
        lv_label_set_text(state->retries, line);
        ESP_LOGI("HiYaKaMenu", "uptime=%lu refresh_last_us=%lu refresh_max_us=%lu slow50=%lu refresh_failed=%lu pca_first=%lu pca_exhausted=%lu",
                 static_cast<unsigned long>(seconds),
                 static_cast<unsigned long>(stats.last_us),
                 static_cast<unsigned long>(stats.max_us),
                 static_cast<unsigned long>(stats.slow_over_50ms),
                 static_cast<unsigned long>(stats.failed),
                 static_cast<unsigned long>(state->expander->first_write_failures()),
                 static_cast<unsigned long>(state->expander->exhausted_writes()));
    }
}

}  // namespace

void hi_yaka_start_lcd_menu_diagnostic(esp_lcd_panel_io_handle_t io,
                                       HiYaKaPca9557* expander) {
    if (io == nullptr || expander == nullptr || !lvgl_port_lock(1000)) return;
    static MenuState state{io, expander, nullptr, nullptr, nullptr, nullptr};
    lv_obj_t* root = lv_obj_create(lv_layer_top());
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_align(root, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x0B1220), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_radius(root, 0, 0);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* title = lv_label_create(root);
    lv_label_set_text(title, "HiYaKa  MENU  GPIO FAST");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 4, 4);

    state.uptime = lv_label_create(root);
    lv_obj_set_style_text_color(state.uptime, lv_color_white(), 0);
    lv_obj_align(state.uptime, LV_ALIGN_TOP_LEFT, 4, 34);
    state.animation = lv_obj_create(root);
    lv_obj_set_size(state.animation, 48, 48);
    lv_obj_set_style_border_width(state.animation, 0, 0);
    lv_obj_set_style_radius(state.animation, 6, 0);
    lv_obj_align(state.animation, LV_ALIGN_TOP_LEFT, 4, 70);
    state.timing = lv_label_create(root);
    lv_obj_set_style_text_color(state.timing, lv_color_white(), 0);
    lv_obj_align(state.timing, LV_ALIGN_TOP_LEFT, 4, 133);
    state.retries = lv_label_create(root);
    lv_obj_set_style_text_color(state.retries, lv_color_white(), 0);
    lv_obj_align(state.retries, LV_ALIGN_TOP_LEFT, 4, 157);
    lv_timer_create(UpdateMenu, 150, &state);
    lvgl_port_unlock();
}
#else
void hi_yaka_start_lcd_menu_diagnostic(esp_lcd_panel_io_handle_t,
                                       HiYaKaPca9557*) {}
#endif
