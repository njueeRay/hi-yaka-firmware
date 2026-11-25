#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/parlio_tx.h"
#include "esp_rom_sys.h"
#include "esp_rom_gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gif_frames.h"
#include "soc/gpio_sig_map.h"

#ifndef WS_PARLIO_HZ
#define WS_PARLIO_HZ 1000000
#endif
#if WS_PARLIO_HZ == 10000000
#define TRANSPORT_LABEL "PARLIO 10M"
#elif WS_PARLIO_HZ == 1000000
#define TRANSPORT_LABEL "PARLIO 1M"
#else
#define TRANSPORT_LABEL "PARLIO"
#endif
#define TRANSPORT_LOG "PARLIO_VALID_LSB"

// Waveshare ESP32-C6-LCD-1.47, not HiYaKa pin assignments.
enum { PIN_TF_CS=4, PIN_MOSI=6, PIN_SCLK=7, PIN_CS=14, PIN_DC=15,
       PIN_RST=21, PIN_BL=22, WIDTH=172, HEIGHT=320, X_OFFSET=34, Y_OFFSET=0 };
static const char *TAG = "WS147_PARLIO_MENU";
enum { FULL_CHUNK_ROWS = 160, FULL_CHUNK_BYTES = WIDTH * FULL_CHUNK_ROWS * 2 };
static parlio_tx_unit_handle_t parlio;
static uint8_t full_chunk[FULL_CHUNK_BYTES] __attribute__((aligned(64)));
static uint8_t menu_region[WIDTH*48*2] __attribute__((aligned(64)));

// Small built-in 5x7 font for the menu and timing labels.
static const uint8_t *glyph(char ch) {
    static const uint8_t A[] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11};
    static const uint8_t B[] = {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E};
    static const uint8_t C[] = {0x0F,0x10,0x10,0x10,0x10,0x10,0x0F};
    static const uint8_t D[] = {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E};
    static const uint8_t E[] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F};
    static const uint8_t F[] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10};
    static const uint8_t G[] = {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F};
    static const uint8_t H[] = {0x11,0x11,0x11,0x1F,0x11,0x11,0x11};
    static const uint8_t I[] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x1F};
    static const uint8_t L[] = {0x10,0x10,0x10,0x10,0x10,0x10,0x1F};
    static const uint8_t M[] = {0x11,0x1B,0x15,0x15,0x11,0x11,0x11};
    static const uint8_t N[] = {0x11,0x19,0x15,0x13,0x11,0x11,0x11};
    static const uint8_t O[] = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E};
    static const uint8_t P[] = {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10};
    static const uint8_t R[] = {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11};
    static const uint8_t S[] = {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E};
    static const uint8_t T[] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x04};
    static const uint8_t U[] = {0x11,0x11,0x11,0x11,0x11,0x11,0x0E};
    static const uint8_t W[] = {0x11,0x11,0x11,0x15,0x15,0x15,0x0A};
    static const uint8_t Z[] = {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F};
    static const uint8_t colon[] = {0,4,4,0,4,4,0};
    static const uint8_t dot[] = {0,0,0,0,0,0x0C,0x0C};
    static const uint8_t digits[][7] = {
        {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
        {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
        {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
        {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E},
        {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
        {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E},
        {0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E},
        {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
        {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
        {0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E},
    };
    if (ch >= 'a' && ch <= 'z') ch -= 32;
    if (ch >= '0' && ch <= '9') return digits[ch - '0'];
    switch (ch) {
        case 'A': return A; case 'B': return B; case 'C': return C;
        case 'D': return D; case 'E': return E; case 'F': return F; case 'M': return M;
        case 'G': return G; case 'H': return H; case 'I': return I; case 'N': return N;
        case 'L': return L; case 'O': return O; case 'P': return P;
        case 'R': return R; case 'S': return S; case 'T': return T;
        case 'U': return U; case 'W': return W; case 'Z': return Z;
        case ':': return colon; case '.': return dot;
        default: return NULL;
    }
}

typedef struct {
    int64_t release_us;
    int64_t command_us;
    int64_t acquire_us;
    int64_t pixel_us;
    int64_t total_us;
} refresh_t;

static esp_err_t gpio_tx(const uint8_t *data, size_t size) {
    for (size_t i = 0; i < size; ++i) for (int bit = 7; bit >= 0; --bit) {
        gpio_set_level(PIN_SCLK, 0);
        gpio_set_level(PIN_MOSI, (data[i] >> bit) & 1);
        esp_rom_delay_us(1);
        gpio_set_level(PIN_SCLK, 1);
        esp_rom_delay_us(1);
    }
    return ESP_OK;
}

static esp_err_t command(uint8_t cmd, const uint8_t *args, size_t size) {
    gpio_set_level(PIN_CS, 0);
    gpio_set_level(PIN_DC, 0);
    esp_err_t err = gpio_tx(&cmd, 1);
    if (err == ESP_OK && size) {
        gpio_set_level(PIN_DC, 1);
        err = gpio_tx(args, size);
    }
    gpio_set_level(PIN_CS, 1);
    return err;
}

static esp_err_t panel_init(void) {
    gpio_set_level(PIN_CS, 1);
    gpio_set_level(PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
    ESP_RETURN_ON_ERROR(command(0x01, NULL, 0), TAG, "SWRESET");
    vTaskDelay(pdMS_TO_TICKS(150));
    ESP_RETURN_ON_ERROR(command(0x11, NULL, 0), TAG, "SLPOUT");
    vTaskDelay(pdMS_TO_TICKS(150));
    // Waveshare's Vernon_ST7789T vendor sequence, then its portrait MX+BGR mirror.
    static const struct { uint8_t cmd, size; uint8_t data[14]; } init[] = {
        {0x36, 1, {0x00}}, {0x3A, 1, {0x55}},
        {0xB0, 2, {0x00, 0xE8}}, {0xB2, 5, {0x0C, 0x0C, 0x00, 0x33, 0x33}},
        {0xB7, 1, {0x75}}, {0xBB, 1, {0x1A}}, {0xC0, 1, {0x80}},
        {0xC2, 2, {0x01, 0xFF}}, {0xC3, 1, {0x13}}, {0xC4, 1, {0x20}},
        {0xC6, 1, {0x0F}}, {0xD0, 2, {0xA4, 0xA1}},
        {0xE0, 14, {0xD0, 0x0D, 0x14, 0x0D, 0x0D, 0x09, 0x38, 0x44,
                    0x4E, 0x3A, 0x17, 0x18, 0x2F, 0x30}},
        {0xE1, 14, {0xD0, 0x09, 0x0F, 0x08, 0x07, 0x14, 0x37, 0x44,
                    0x4D, 0x38, 0x15, 0x16, 0x2C, 0x2E}},
    };
    for (size_t i = 0; i < sizeof(init) / sizeof(init[0]); ++i)
        ESP_RETURN_ON_ERROR(command(init[i].cmd, init[i].data, init[i].size), TAG, "vendor init");
    ESP_RETURN_ON_ERROR(command(0x21, NULL, 0), TAG, "INVON");
    ESP_RETURN_ON_ERROR(command(0x29, NULL, 0), TAG, "DISPON");
    const uint8_t madctl = 0x48; // Waveshare mirror(true,false) plus BGR.
    ESP_RETURN_ON_ERROR(command(0x36, &madctl, 1), TAG, "mirror + BGR");
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}

static esp_err_t setup_gpio(void) {
    gpio_config_t cfg = { .pin_bit_mask = (1ULL << PIN_TF_CS) | (1ULL << PIN_MOSI) | (1ULL << PIN_SCLK) |
                                            (1ULL << PIN_CS) | (1ULL << PIN_DC) |
                                            (1ULL << PIN_RST) | (1ULL << PIN_BL),
                          .mode = GPIO_MODE_OUTPUT, .pull_up_en = 0,
                          .pull_down_en = 0, .intr_type = GPIO_INTR_DISABLE };
    ESP_RETURN_ON_ERROR(gpio_config(&cfg), TAG, "GPIO config");
    gpio_set_level(PIN_MOSI, 0);
    gpio_set_level(PIN_SCLK, 0);
    gpio_set_level(PIN_TF_CS, 1);
    gpio_set_level(PIN_CS, 1);
    gpio_set_level(PIN_DC, 0);
    gpio_set_level(PIN_RST, 1);
    gpio_set_level(PIN_BL, 0);
    return ESP_OK;
}

static esp_err_t setup_backlight(void) {
    // setup_gpio has already reserved GPIO22. IDF 6.1 may warn about this
    // reservation, but ledc_channel_config still remaps the pin and returns OK.
    ledc_timer_config_t timer = { .speed_mode = LEDC_LOW_SPEED_MODE,
                                  .duty_resolution = LEDC_TIMER_10_BIT,
                                  .timer_num = LEDC_TIMER_0, .freq_hz = 5000,
                                  .clk_cfg = LEDC_AUTO_CLK };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer), TAG, "BL timer");
    ledc_channel_config_t ch = { .gpio_num = PIN_BL,
                                 .speed_mode = LEDC_LOW_SPEED_MODE,
                                 .channel = LEDC_CHANNEL_0,
                                 .intr_type = LEDC_INTR_DISABLE,
                                 .timer_sel = LEDC_TIMER_0,
                                 .duty = 384, .hpoint = 0 };
    return ledc_channel_config(&ch); // 37.5% duty, below Waveshare 50% limit
}

static esp_err_t release_parlio_to_gpio(refresh_t *timing) {
    int64_t start = esp_timer_get_time();
    if (parlio != NULL) {
        ESP_RETURN_ON_ERROR(parlio_tx_unit_wait_all_done(parlio, 2000), TAG, "PARLIO drain");
        ESP_RETURN_ON_ERROR(parlio_tx_unit_disable(parlio), TAG, "PARLIO disable");
        ESP_RETURN_ON_ERROR(parlio_del_tx_unit(parlio), TAG, "PARLIO delete");
        parlio = NULL;
    }
    // The PARLIO driver disables these outputs when deleted. Reclaim all
    // three pins before sending any short LCD command.
    gpio_config_t pins = { .pin_bit_mask = (1ULL << PIN_MOSI) |
                                            (1ULL << PIN_SCLK) |
                                            (1ULL << PIN_CS),
                           .mode = GPIO_MODE_OUTPUT, .pull_up_en = 0,
                           .pull_down_en = 0, .intr_type = GPIO_INTR_DISABLE };
    ESP_RETURN_ON_ERROR(gpio_config(&pins), TAG, "GPIO reclaim");
    // gpio_config alone may retain PARLIO's output matrix after unit deletion.
    esp_rom_gpio_connect_out_signal(PIN_MOSI, SIG_GPIO_OUT_IDX, false, false);
    esp_rom_gpio_connect_out_signal(PIN_SCLK, SIG_GPIO_OUT_IDX, false, false);
    esp_rom_gpio_connect_out_signal(PIN_CS, SIG_GPIO_OUT_IDX, false, false);
    gpio_set_level(PIN_CS, 1);
    gpio_set_level(PIN_SCLK, 0);
    gpio_set_level(PIN_MOSI, 0);
    timing->release_us = esp_timer_get_time() - start;
    return ESP_OK;
}

static esp_err_t setup_parlio_valid_cs(void) {
    parlio_tx_unit_config_t cfg = {0};
    cfg.clk_src = PARLIO_CLK_SRC_DEFAULT;
    cfg.clk_in_gpio_num = GPIO_NUM_NC;
    cfg.output_clk_freq_hz = WS_PARLIO_HZ;
    cfg.data_width = 1;
    for (size_t i = 0; i < sizeof(cfg.data_gpio_nums) / sizeof(cfg.data_gpio_nums[0]); ++i)
        cfg.data_gpio_nums[i] = GPIO_NUM_NC;
    cfg.data_gpio_nums[0] = PIN_MOSI;
    cfg.clk_out_gpio_num = PIN_SCLK;
    cfg.valid_gpio_num = PIN_CS;
    cfg.trans_queue_depth = 1;
    cfg.max_transfer_size = FULL_CHUNK_BYTES;
    cfg.dma_burst_size = 64;
    cfg.shift_edge = PARLIO_SHIFT_EDGE_NEG;
    cfg.bit_pack_order = PARLIO_BIT_PACK_ORDER_LSB;
    cfg.flags.invert_valid_out = true;
    ESP_RETURN_ON_ERROR(parlio_new_tx_unit(&cfg, &parlio), TAG, "PARLIO new unit");
    return parlio_tx_unit_enable(parlio);
}

static esp_err_t window_begin(int x, int y, int w, int h, refresh_t *timing) {
    ESP_RETURN_ON_FALSE(x >= 0 && y >= 0 && w > 0 && h > 0 &&
                        x + w <= WIDTH && y + h <= HEIGHT,
                        ESP_ERR_INVALID_ARG, TAG, "window bounds");
    ESP_RETURN_ON_ERROR(release_parlio_to_gpio(timing), TAG, "release PARLIO");
    int64_t start = esp_timer_get_time();
    int x0 = X_OFFSET + x, x1 = x0 + w - 1;
    int y0 = Y_OFFSET + y, y1 = y0 + h - 1;
    uint8_t col[] = {x0 >> 8, x0, x1 >> 8, x1};
    uint8_t row[] = {y0 >> 8, y0, y1 >> 8, y1};
    ESP_RETURN_ON_ERROR(command(0x2A, col, sizeof(col)), TAG, "CASET");
    ESP_RETURN_ON_ERROR(command(0x2B, row, sizeof(row)), TAG, "RASET");
    gpio_set_level(PIN_CS, 0);
    gpio_set_level(PIN_DC, 0);
    const uint8_t ramwr = 0x2C;
    esp_err_t err = gpio_tx(&ramwr, 1);
    gpio_set_level(PIN_DC, 1);
    gpio_set_level(PIN_CS, 1);
    gpio_set_level(PIN_SCLK, 0);
    ESP_RETURN_ON_ERROR(err, TAG, "RAMWR");
    timing->command_us = esp_timer_get_time() - start;
    start = esp_timer_get_time();
    ESP_RETURN_ON_ERROR(setup_parlio_valid_cs(), TAG, "claim PARLIO");
    timing->acquire_us = esp_timer_get_time() - start;
    return ESP_OK;
}

static esp_err_t parlio_pixels(const uint8_t *data, size_t bytes, int64_t *duration_us) {
    ESP_RETURN_ON_FALSE(parlio && data && bytes && bytes <= FULL_CHUNK_BYTES &&
                        (bytes % 4) == 0, ESP_ERR_INVALID_ARG, TAG, "pixel buffer");
    const parlio_transmit_config_t cfg = { .idle_value = 0 };
    int64_t start = esp_timer_get_time();
    esp_err_t err = parlio_tx_unit_transmit(parlio, data, bytes * 8, &cfg);
    if (err == ESP_OK) err = parlio_tx_unit_wait_all_done(parlio, 2000);
    if (duration_us) *duration_us = esp_timer_get_time() - start;
    return err;
}

static esp_err_t menu_clear_screen(refresh_t *timing) {
    int64_t start = esp_timer_get_time();
    memset(timing, 0, sizeof(*timing));
    memset(full_chunk, 0, sizeof(full_chunk));
    ESP_RETURN_ON_ERROR(window_begin(0, 0, WIDTH, HEIGHT, timing), TAG, "full window");
    for (int half = 0; half < 2; ++half) {
        int64_t pixel_us = 0;
        ESP_RETURN_ON_ERROR(parlio_pixels(full_chunk, sizeof(full_chunk), &pixel_us),
                            TAG, "full pixels");
        timing->pixel_us += pixel_us;
    }
    timing->total_us = esp_timer_get_time() - start;
    return ESP_OK;
}

static esp_err_t menu_full_pattern(refresh_t *timing) {
    int64_t start = esp_timer_get_time();
    memset(timing, 0, sizeof(*timing));
    ESP_RETURN_ON_ERROR(window_begin(0, 0, WIDTH, HEIGHT, timing), TAG, "pattern window");
    for (int half = 0; half < 2; ++half) {
        for (int row = 0; row < FULL_CHUNK_ROWS; ++row) {
            for (int x = 0; x < WIDTH; ++x) {
                uint16_t color = half == 0
                    ? (x < WIDTH / 2 ? 0xF800 : 0x07E0)
                    : (x < WIDTH / 2 ? 0x001F : 0xFFFF);
                size_t p = ((size_t)row * WIDTH + x) * 2;
                full_chunk[p] = color >> 8;
                full_chunk[p + 1] = color;
            }
        }
        int64_t pixel_us = 0;
        ESP_RETURN_ON_ERROR(parlio_pixels(full_chunk, sizeof(full_chunk), &pixel_us),
                            TAG, "pattern pixels");
        timing->pixel_us += pixel_us;
    }
    timing->total_us = esp_timer_get_time() - start;
    return ESP_OK;
}

static void menu_pixel(int stride, int x, int y, uint16_t color) {
    const size_t p = ((size_t)y * stride + x) * 2;
    menu_region[p] = color >> 8;
    menu_region[p + 1] = color;
}

static void menu_clear(int w, int h) {
    memset(menu_region, 0, (size_t)w * h * 2);
}

static esp_err_t menu_blit(int x, int y, int w, int h, refresh_t *timing);

static esp_err_t menu_rect(int x, int y, int w, int h,
                           uint16_t color, refresh_t *timing) {
    ESP_RETURN_ON_FALSE(w > 0 && h > 0 && w <= WIDTH && h <= 48,
                        ESP_ERR_INVALID_ARG, TAG, "rectangle size");
    for (int yy = 0; yy < h; ++yy) for (int xx = 0; xx < w; ++xx)
        menu_pixel(w, xx, yy, color);
    return menu_blit(x, y, w, h, timing);
}

static void menu_text(int w, int h, const char *text, int scale, uint16_t color) {
    for (int i = 0; text[i]; ++i) {
        const uint8_t *g = glyph(text[i]);
        if (g == NULL) continue;
        for (int gy = 0; gy < 7; ++gy) for (int gx = 0; gx < 5; ++gx) {
            if (!(g[gy] & (1U << (4 - gx)))) continue;
            for (int sy = 0; sy < scale; ++sy) for (int sx = 0; sx < scale; ++sx) {
                int x = i * 6 * scale + gx * scale + sx;
                int y = gy * scale + sy + 1;
                if (x < w && y < h) menu_pixel(w, x, y, color);
            }
        }
    }
}

// `total_us` includes release/reclaim, CASET/RASET/RAMWR, PARLIO allocation,
// and DMA completion. `pixel_us` covers only transmit plus wait_all_done.
static esp_err_t menu_blit(int x, int y, int w, int h, refresh_t *timing) {
    if (w <= 0 || h <= 0 || w > WIDTH || h > 48 || x < 0 || y < 0 ||
        x + w > WIDTH || y + h > HEIGHT) return ESP_ERR_INVALID_ARG;
    refresh_t local = {0};
    if (timing == NULL) timing = &local;
    else memset(timing, 0, sizeof(*timing));
    int64_t start = esp_timer_get_time();
    ESP_RETURN_ON_ERROR(window_begin(x, y, w, h, timing), TAG, "partial window");
    ESP_RETURN_ON_ERROR(parlio_pixels(menu_region, (size_t)w * h * 2,
                                      &timing->pixel_us), TAG, "partial pixels");
    timing->total_us = esp_timer_get_time() - start;
    return ESP_OK;
}

static esp_err_t menu_text_region(int x, int y, int w, int h,
                                  const char *text, int scale,
                                  uint16_t color, refresh_t *timing) {
    menu_clear(w, h);
    menu_text(w, h, text, scale, color);
    return menu_blit(x, y, w, h, timing);
}

static esp_err_t menu_anim(unsigned frame, refresh_t *timing) {
    for (int y = 0; y < GIF_HEIGHT; ++y) for (int x = 0; x < GIF_WIDTH; ++x)
        menu_pixel(GIF_WIDTH, x, y,
                   gif_frames[frame % GIF_FRAME_COUNT][y * GIF_WIDTH + x]);
    return menu_blit((WIDTH-GIF_WIDTH)/2, 128, GIF_WIDTH, GIF_HEIGHT, timing);
}

static void stop_on_error(esp_err_t err, const char *stage) {
    ESP_LOGE(TAG, "STOP stage=%s err=%s parlio_owner=%d;"
             " do not force GPIO14 while a DMA transfer may be active;"
             " no automatic reboot", stage, esp_err_to_name(err), parlio != NULL);
    while (true) vTaskDelay(pdMS_TO_TICKS(1000));
}

#define REQUIRE_OK(expr, stage) do { \
    esp_err_t _err = (expr); \
    if (_err != ESP_OK) stop_on_error(_err, (stage)); \
} while (0)

void app_main(void) {
    REQUIRE_OK(setup_gpio(), "GPIO setup");
    REQUIRE_OK(panel_init(), "panel init");
    REQUIRE_OK(setup_backlight(), "backlight request");
    refresh_t full = {0};
    REQUIRE_OK(menu_full_pattern(&full), "four quadrant full refresh");
    ESP_LOGI(TAG, "FULL_PATTERN bytes=%u pixel_us=%" PRId64
             " release_us=%" PRId64 " command_us=%" PRId64
             " acquire_us=%" PRId64 " total_us=%" PRId64
             " clock_request_hz=%d corners=red,green,blue,white",
             FULL_CHUNK_BYTES * 2, full.pixel_us, full.release_us,
             full.command_us, full.acquire_us, full.total_us, WS_PARLIO_HZ);
    vTaskDelay(pdMS_TO_TICKS(1500));
    refresh_t clear = {0};
    REQUIRE_OK(menu_clear_screen(&clear), "clear before corner test");
    const int corner_x[] = {0, WIDTH - 24, 0, WIDTH - 24};
    const int corner_y[] = {0, 0, HEIGHT - 24, HEIGHT - 24};
    const uint16_t corner_color[] = {0xF800, 0x07E0, 0x001F, 0xFFFF};
    for (int i = 0; i < 4; ++i) {
        refresh_t corner = {0};
        REQUIRE_OK(menu_rect(corner_x[i], corner_y[i], 24, 24,
                             corner_color[i], &corner), "corner partial refresh");
        ESP_LOGI(TAG, "CORNER index=%d x=%d y=%d bytes=%d"
                 " release_us=%" PRId64 " command_us=%" PRId64
                 " acquire_us=%" PRId64 " pixel_us=%" PRId64
                 " total_us=%" PRId64,
                 i, corner_x[i], corner_y[i], 24 * 24 * 2,
                 corner.release_us, corner.command_us, corner.acquire_us,
                 corner.pixel_us, corner.total_us);
    }
    vTaskDelay(pdMS_TO_TICKS(1500));
    REQUIRE_OK(menu_clear_screen(&clear), "clear before menu");

    REQUIRE_OK(menu_text_region(8, 6, 100, 20, "PARLIO", 2, 0x07FF, NULL), "title");
    REQUIRE_OK(menu_text_region(118, 6, 52, 20, "menu", 2, 0x07FF, NULL), "menu");
    REQUIRE_OK(menu_text_region(10, 36, 100, 20, "UPTIME", 2, 0xFFFF, NULL), "uptime label");
    REQUIRE_OK(menu_text_region(10, 101, 100, 20, "ANIM", 2, 0xFFE0, NULL), "anim label");
    REQUIRE_OK(menu_text_region(10, 180, 156, 20, "FULL US", 2, 0xFFFF, NULL), "full label");
    char value[24];
    snprintf(value, sizeof(value), "FULL %06" PRId64, full.total_us);
    REQUIRE_OK(menu_text_region(10, 202, 156, 20, value, 2, 0x07E0, NULL), "full value");
    REQUIRE_OK(menu_text_region(10, 226, 156, 20, "PARTIAL US", 2, 0xFFFF, NULL), "partial label");
    REQUIRE_OK(menu_text_region(10, 296, 160, 20, TRANSPORT_LABEL, 2, 0x07E0, NULL), "mode label");
    ESP_LOGI(TAG, "MENU_READY mode=%s panel=172x320 x_offset=34"
             " anim=8_predecoded_RGB565_frames_48x48 BL_request=37.5%%"
             " clock_request_hz=%d",
             TRANSPORT_LOG, WS_PARLIO_HZ);

    unsigned frame = 0, frame_count = 0, previous_frame_count = 0;
    refresh_t last_anim = {0}, last_clock = {0};
    int64_t next_anim = 0, next_clock = 0;
    int64_t last_stats_at = esp_timer_get_time();
    int64_t next_stats = last_stats_at + 1000000;
    while (true) {
        int64_t now = esp_timer_get_time();
        if (now >= next_clock) {
            uint64_t ms = (uint64_t)now / 1000, sec = ms / 1000;
            snprintf(value, sizeof(value), "%02llu:%02llu:%02llu.%03llu",
                     (unsigned long long)((sec / 3600) % 100),
                     (unsigned long long)((sec / 60) % 60),
                     (unsigned long long)(sec % 60),
                     (unsigned long long)(ms % 1000));
            REQUIRE_OK(menu_text_region(8, 63, 156, 22, value, 2,
                                        0xFFFF, &last_clock), "clock refresh");
            next_clock = now + 100000; // Target cadence, not measured frame rate.
        }
        if (now >= next_anim) {
            REQUIRE_OK(menu_anim(frame++, &last_anim), "animation refresh");
            next_anim = now + 150000;
            ++frame_count; // Only count DMA-completed frames.
        }
        int64_t stats_now = esp_timer_get_time();
        if (stats_now >= next_stats) {
            int64_t window_us = stats_now - last_stats_at;
            unsigned frames_window = frame_count - previous_frame_count;
            uint64_t fps_milli = window_us > 0
                ? (uint64_t)frames_window * 1000000000ULL / (uint64_t)window_us : 0;
            snprintf(value, sizeof(value), "ANM %06" PRId64, last_anim.total_us);
            REQUIRE_OK(menu_text_region(10, 248, 155, 20, value, 2,
                                        0xFFE0, NULL), "animation timing");
            snprintf(value, sizeof(value), "CLK %06" PRId64, last_clock.total_us);
            REQUIRE_OK(menu_text_region(10, 270, 155, 20, value, 2,
                                        0xFFFF, NULL), "clock timing");
            ESP_LOGI(TAG, "PARTIAL mode=%s anim_pixels=%u anim_bytes=%u"
                     " anim_release_us=%" PRId64 " anim_command_us=%" PRId64
                     " anim_acquire_us=%" PRId64 " anim_pixel_us=%" PRId64
                     " anim_total_us=%" PRId64
                     " clock_pixels=%u clock_bytes=%u"
                     " clock_release_us=%" PRId64 " clock_command_us=%" PRId64
                     " clock_acquire_us=%" PRId64 " clock_pixel_us=%" PRId64
                     " clock_total_us=%" PRId64
                     " frames=%u frames_window=%u window_us=%" PRId64
                     " actual_fps_milli=%" PRIu64 " status=ESP_OK",
                     TRANSPORT_LOG, GIF_WIDTH * GIF_HEIGHT, GIF_WIDTH * GIF_HEIGHT * 2,
                     last_anim.release_us, last_anim.command_us,
                     last_anim.acquire_us, last_anim.pixel_us, last_anim.total_us,
                     156 * 22, 156 * 22 * 2,
                     last_clock.release_us, last_clock.command_us,
                     last_clock.acquire_us, last_clock.pixel_us, last_clock.total_us,
                     frame_count, frames_window, window_us, fps_milli);
            last_stats_at = stats_now;
            previous_frame_count = frame_count;
            next_stats = stats_now + 1000000;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
