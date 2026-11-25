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
    lv_obj_t* root;
    lv_obj_t* uptime;
    lv_obj_t* animation;
    lv_obj_t* timing;
    lv_obj_t* health;
    lv_obj_t* retries;
    uint32_t frame = 0;
    uint32_t last_second = UINT32_MAX;
    uint32_t last_logged_failure_sequence = 0;
    uint32_t completed_before_menu = 0;
    int64_t menu_created_us = 0;
    bool one_shot_done = false;
    bool one_shot_wait_logged = false;
};

const char* FailureStageName(HiYaKaLcdFailureStage stage) {
    switch (stage) {
        case HiYaKaLcdFailureStage::kCaset: return "CASET";
        case HiYaKaLcdFailureStage::kRaset: return "RASET";
        case HiYaKaLcdFailureStage::kRamwr: return "RAMWR";
        case HiYaKaLcdFailureStage::kCsRelease: return "CS_RELEASE";
        case HiYaKaLcdFailureStage::kOther: return "OTHER";
        default: return "NONE";
    }
}

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
        constexpr int64_t kOneShotDelayUs = 3000000;
        const int64_t age_us = esp_timer_get_time() - state->menu_created_us;
        if (!state->one_shot_done && !state->one_shot_wait_logged &&
            age_us >= kOneShotDelayUs &&
            stats.completed <= state->completed_before_menu) {
            state->one_shot_wait_logged = true;
            ESP_LOGW("HiYaKaMenu",
                     "ONE_SHOT_WAIT no successful menu flush after %lld us",
                     static_cast<long long>(age_us));
        }
        if (!state->one_shot_done && age_us >= kOneShotDelayUs &&
            stats.completed > state->completed_before_menu) {
            // Queue exactly one full redraw after the menu's first flush.
            // This does not retry on a failed LCD or PCA transaction.
            state->one_shot_done = true;
            const lv_area_t area{0, 0,
                                 lv_obj_get_width(state->root) - 1,
                                 lv_obj_get_height(state->root) - 1};
            const lv_result_t queued =
                lv_obj_invalidate_area(state->root, &area);
            ESP_LOGW("HiYaKaMenu",
                     "ONE_SHOT_FULL_REDRAW age_us=%lld queued=%d area=%ld..%ld,%ld..%ld before_completed=%lu before_failed=%lu last_failure_seq=%lu",
                     static_cast<long long>(age_us), queued == LV_RESULT_OK,
                     static_cast<long>(area.x1), static_cast<long>(area.x2),
                     static_cast<long>(area.y1), static_cast<long>(area.y2),
                     static_cast<unsigned long>(stats.completed),
                     static_cast<unsigned long>(stats.failed),
                     static_cast<unsigned long>(stats.last_failure.sequence));
        }
        std::snprintf(line, sizeof(line), "LCD last %lu  max %lu ms",
                      static_cast<unsigned long>(stats.last_us / 1000),
                      static_cast<unsigned long>(stats.max_us / 1000));
        lv_label_set_text(state->timing, line);
        std::snprintf(line, sizeof(line), "slow>50:%lu  draw fail:%lu",
                      static_cast<unsigned long>(stats.slow_over_50ms),
                      static_cast<unsigned long>(stats.failed));
        lv_label_set_text(state->health, line);
        std::snprintf(line, sizeof(line), "PCA first:%lu  exhausted:%lu",
                      static_cast<unsigned long>(state->expander->first_write_failures()),
                      static_cast<unsigned long>(state->expander->exhausted_writes()));
        lv_label_set_text(state->retries, line);
        ESP_LOGI("HiYaKaMenu", "uptime=%lu refresh_last_us=%lu refresh_max_us=%lu slow50=%lu refresh_failed=%lu pca_first=%lu pca_exhausted=%lu",
                 static_cast<unsigned long>(seconds),
                 static_cast<unsigned long>(stats.last_us),
                 static_cast<unsigned long>(stats.max_us),
                 static_cast<unsigned long>(stats.slow_over_50ms),
                 static_cast<unsigned long>(stats.failed),
                 static_cast<unsigned long>(state->expander->first_write_failures()),
                 static_cast<unsigned long>(state->expander->exhausted_writes()));
        const auto& failure = stats.last_failure;
        if (failure.sequence != state->last_logged_failure_sequence) {
            state->last_logged_failure_sequence = failure.sequence;
            ESP_LOGW("HiYaKaMenu",
                     "last LCD failure seq=%lu stage=%s err=%s x=%u..%u%s y=%u..%u%s",
                     static_cast<unsigned long>(failure.sequence),
                     FailureStageName(failure.stage), esp_err_to_name(failure.error),
                     failure.x_start, failure.x_end,
                     failure.has_columns ? "" : "?",
                     failure.y_start, failure.y_end,
                     failure.has_rows ? "" : "?");
        }
    }
}

}  // namespace

void hi_yaka_start_lcd_menu_diagnostic(esp_lcd_panel_io_handle_t io,
                                       HiYaKaPca9557* expander) {
    if (io == nullptr || expander == nullptr || !lvgl_port_lock(1000)) return;
    static MenuState state{io, expander, nullptr, nullptr, nullptr, nullptr,
                           nullptr, nullptr};
    HiYaKaLcdRefreshStats initial_stats{};
    if (hi_yaka_lcd_refresh_stats(io, &initial_stats) == ESP_OK) {
        state.completed_before_menu = initial_stats.completed;
    }
    lv_display_t* display = lv_display_get_default();
    const int width = lv_display_get_horizontal_resolution(display);
    const int height = lv_display_get_vertical_resolution(display);
    ESP_LOGI("HiYaKaMenu", "LVGL geometry=%dx%d; expected c0 landscape=320x200", width, height);
    lv_obj_t* root = lv_obj_create(lv_layer_top());
    state.root = root;
    state.menu_created_us = esp_timer_get_time();
    lv_obj_set_size(root, width, height);
    lv_obj_set_pos(root, 0, 0);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x0B1220), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_radius(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(root);
    lv_label_set_text(title, "HiYaKa  MENU  GPIO FAST");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 8, 7);

    state.uptime = lv_label_create(root);
    lv_obj_set_style_text_color(state.uptime, lv_color_white(), 0);
    lv_obj_align(state.uptime, LV_ALIGN_TOP_LEFT, 8, 36);
    state.animation = lv_obj_create(root);
    lv_obj_set_size(state.animation, 48, 48);
    lv_obj_set_style_border_width(state.animation, 0, 0);
    lv_obj_set_style_radius(state.animation, 6, 0);
    lv_obj_align(state.animation, LV_ALIGN_TOP_LEFT, 8, 72);
    state.timing = lv_label_create(root);
    lv_obj_set_style_text_color(state.timing, lv_color_white(), 0);
    lv_obj_align(state.timing, LV_ALIGN_TOP_LEFT, 8, 124);
    state.health = lv_label_create(root);
    lv_obj_set_style_text_color(state.health, lv_color_white(), 0);
    lv_obj_align(state.health, LV_ALIGN_TOP_LEFT, 8, 145);
    state.retries = lv_label_create(root);
    lv_obj_set_style_text_color(state.retries, lv_color_white(), 0);
    lv_obj_align(state.retries, LV_ALIGN_TOP_LEFT, 8, 166);
    lv_obj_t* labels[] = {title, state.uptime, state.timing, state.health,
                          state.retries};
    for (lv_obj_t* label : labels) {
        lv_obj_set_size(label, width - 16, 19);
        lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    }
    lv_timer_create(UpdateMenu, 150, &state);
    lvgl_port_unlock();
}
#else
void hi_yaka_start_lcd_menu_diagnostic(esp_lcd_panel_io_handle_t,
                                       HiYaKaPca9557*) {}
#endif
