#include "hi_yaka_lcd_menu.h"

#include "config.h"
#include "hi_yaka_demo_frames.h"
#include "hi_yaka_expander_lcd_io.h"

#include <cstdio>
#include <esp_lvgl_port.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <lvgl.h>

#if CONFIG_HI_YAKA_LCD_MENU_DIAGNOSTIC
namespace {

constexpr uint32_t kClockUpdatePeriodMs = 10;
constexpr uint32_t kMenuUpdatePeriodMs = 150;

struct MenuState {
    esp_lcd_panel_io_handle_t io;
    HiYaKaPca9557* expander;
    lv_obj_t* root;
    lv_obj_t* uptime;
    lv_obj_t* animation;
    lv_obj_t* mode;
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

// The reference GIF is decoded ahead of time into eight RGB565 frames. Each
// descriptor points at immutable flash data; no runtime GIF decoder is used.
lv_image_dsc_t frame_images[GIF_FRAME_COUNT] = {};

const char* TransportName() {
#if CONFIG_HI_YAKA_LCD_SPI_DMA_DIAGNOSTIC
    return "SPI2 DMA 12MHZ";
#elif CONFIG_HI_YAKA_LCD_PARLIO_MSB_DIAGNOSTIC
    return "PARLIO 1BIT BIDIR CS";
#elif CONFIG_HI_YAKA_LCD_PARLIO_ENCODED_DIAGNOSTIC
#if CONFIG_HI_YAKA_LCD_PARLIO_CS_GUARD_DIAGNOSTIC
    return "PARLIO 4BIT CS GUARD";
#else
    return "PARLIO 4BIT 10MHZ";
#endif
#elif CONFIG_HI_YAKA_LCD_GPIO_REGISTER_FAST_DIAGNOSTIC
    return "GPIO REG FAST";
#elif CONFIG_HI_YAKA_LCD_GPIO_FAST_DIAGNOSTIC
    return "GPIO FAST";
#elif CONFIG_HI_YAKA_LCD_GPIO_DIAGNOSTIC
    return "GPIO SLOW";
#else
    return "PARLIO";
#endif
}

void PrepareFrames() {
    for (size_t i = 0; i < GIF_FRAME_COUNT; ++i) {
        frame_images[i].header.magic = LV_IMAGE_HEADER_MAGIC;
        frame_images[i].header.cf = LV_COLOR_FORMAT_RGB565;
        frame_images[i].header.w = GIF_WIDTH;
        frame_images[i].header.h = GIF_HEIGHT;
        frame_images[i].header.stride = GIF_WIDTH * sizeof(uint16_t);
        frame_images[i].data_size = sizeof(gif_frames[i]);
        frame_images[i].data = reinterpret_cast<const uint8_t*>(gif_frames[i]);
    }
}

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

void UpdateClock(lv_timer_t* timer) {
    auto* state = static_cast<MenuState*>(lv_timer_get_user_data(timer));
    if (state == nullptr) return;

    const uint64_t uptime_ms =
        static_cast<uint64_t>(esp_timer_get_time() / 1000);
    const uint32_t seconds = static_cast<uint32_t>(uptime_ms / 1000);

    char line[64];
    std::snprintf(line, sizeof(line), "UPTIME %02lu:%02lu:%02lu.%03lu",
                  static_cast<unsigned long>(seconds / 3600 % 100),
                  static_cast<unsigned long>(seconds / 60 % 60),
                  static_cast<unsigned long>(seconds % 60),
                  static_cast<unsigned long>(uptime_ms % 1000));
    lv_label_set_text(state->uptime, line);
}

void UpdateMenu(lv_timer_t* timer) {
    auto* state = static_cast<MenuState*>(lv_timer_get_user_data(timer));
    if (state == nullptr) return;

    // Keep the 48x48 changing region identical in both transport profiles.
    // The animation remains at 150 ms; only the uptime text runs at 10 ms.
    // This avoids multiplying animation redraw and PCA9557 control traffic by
    // fifteen merely to expose the milliseconds field.
    lv_image_set_src(state->animation,
                     &frame_images[(state->frame++) % GIF_FRAME_COUNT]);

    const uint32_t seconds = static_cast<uint32_t>(
        esp_timer_get_time() / 1000000);
    char line[64];

    if (seconds == state->last_second) return;
    state->last_second = seconds;

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
        std::snprintf(line, sizeof(line), "LCD %lu / %lu ms",
                      static_cast<unsigned long>(stats.last_us / 1000),
                      static_cast<unsigned long>(stats.max_us / 1000));
        lv_label_set_text(state->timing, line);
        std::snprintf(line, sizeof(line), "SLOW %lu  DRAW FAIL %lu",
                      static_cast<unsigned long>(stats.slow_over_50ms),
                      static_cast<unsigned long>(stats.failed));
        lv_label_set_text(state->health, line);
        std::snprintf(line, sizeof(line), "PCA FIRST %lu  EXH %lu",
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
                           nullptr, nullptr, nullptr};
    PrepareFrames();
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
    lv_obj_set_style_bg_color(root, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_radius(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(root);
    lv_label_set_text(title, "HiYaKa LCD DEMO");
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 8, 5);

    state.uptime = lv_label_create(root);
    lv_label_set_text(state.uptime, "UPTIME --:--:--.---");
    lv_obj_align(state.uptime, LV_ALIGN_TOP_LEFT, 8, 27);
    state.animation = lv_image_create(root);
    lv_image_set_src(state.animation, &frame_images[0]);
    lv_obj_align(state.animation, LV_ALIGN_TOP_LEFT, 8, 52);
    lv_obj_t* animation_caption = lv_label_create(root);
    lv_label_set_text(animation_caption, "ANIMATION 48x48");
    lv_obj_align(animation_caption, LV_ALIGN_TOP_LEFT, 68, 65);
    state.timing = lv_label_create(root);
    lv_label_set_text(state.timing, "LCD -- / -- ms");
    lv_obj_align(state.timing, LV_ALIGN_TOP_LEFT, 8, 109);
    state.health = lv_label_create(root);
    lv_label_set_text(state.health, "SLOW --  DRAW FAIL --");
    lv_obj_align(state.health, LV_ALIGN_TOP_LEFT, 8, 131);
    state.retries = lv_label_create(root);
    lv_label_set_text(state.retries, "PCA FIRST --  EXH --");
    lv_obj_align(state.retries, LV_ALIGN_TOP_LEFT, 8, 153);
    state.mode = lv_label_create(root);
    char mode_line[32];
    std::snprintf(mode_line, sizeof(mode_line), "MODE: %s", TransportName());
    lv_label_set_text(state.mode, mode_line);
    lv_obj_align(state.mode, LV_ALIGN_TOP_LEFT, 8, 176);
    lv_obj_t* labels[] = {title, state.uptime, animation_caption,
                          state.timing, state.health, state.retries, state.mode};
    for (lv_obj_t* label : labels) {
        lv_obj_set_size(label, width - 16, 19);
        lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(label, lv_color_black(), 0);
    }
    lv_timer_create(UpdateClock, kClockUpdatePeriodMs, &state);
    lv_timer_create(UpdateMenu, kMenuUpdatePeriodMs, &state);
    lvgl_port_unlock();
}
#else
void hi_yaka_start_lcd_menu_diagnostic(esp_lcd_panel_io_handle_t,
                                       HiYaKaPca9557*) {}
#endif
