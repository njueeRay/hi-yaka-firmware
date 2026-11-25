#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "driver/parlio_tx.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/gpio_reg.h"
#include "soc/soc.h"

// Waveshare ESP32-C6-LCD-1.47, 172x320 ST7789, not HiYaKa pin assignments.
enum { PIN_TF_CS = 4, PIN_MOSI = 6, PIN_SCLK = 7, PIN_CS = 14, PIN_DC = 15, PIN_RST = 21,
       PIN_BL = 22, WIDTH = 172, HEIGHT = 320, X_OFFSET = 34, Y_OFFSET = 0 };
static const char *TAG = "WS147_LCD_BENCH";
static spi_device_handle_t spi;
static parlio_tx_unit_handle_t parlio;
static uint8_t line[WIDTH * 2];
enum { BATCH_ROWS = 160, BATCH_BYTES = WIDTH * 2 * BATCH_ROWS };
static uint8_t batch[BATCH_BYTES] __attribute__((aligned(64)));
typedef enum { MODE_GPIO_SLOW, MODE_GPIO_FAST, MODE_SPI2, MODE_PARLIO } mode_t;
static mode_t mode;
static const char *screen_label = "BOOT";

// Small built-in 5x7 font for the two transport labels; no font asset needed.
static const uint8_t *glyph(char ch) {
    static const uint8_t A[] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11};
    static const uint8_t B[] = {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E};
    static const uint8_t C[] = {0x0F,0x10,0x10,0x10,0x10,0x10,0x0F};
    static const uint8_t D[] = {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E};
    static const uint8_t F[] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10};
    static const uint8_t G[] = {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F};
    static const uint8_t I[] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x1F};
    static const uint8_t L[] = {0x10,0x10,0x10,0x10,0x10,0x10,0x1F};
    static const uint8_t M[] = {0x11,0x1B,0x15,0x15,0x11,0x11,0x11};
    static const uint8_t O[] = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E};
    static const uint8_t P[] = {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10};
    static const uint8_t R[] = {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11};
    static const uint8_t S[] = {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E};
    static const uint8_t T[] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x04};
    static const uint8_t U[] = {0x11,0x11,0x11,0x11,0x11,0x11,0x0E};
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
    if (ch >= '0' && ch <= '9') return digits[ch - '0'];
    switch (ch) {
        case 'A': return A; case 'B': return B; case 'C': return C;
        case 'D': return D; case 'F': return F; case 'M': return M;
        case 'G': return G; case 'I': return I;
        case 'L': return L; case 'O': return O; case 'P': return P;
        case 'R': return R; case 'S': return S; case 'T': return T;
        case 'U': return U;
        default: return NULL;
    }
}

static void make_labeled_row(int y, uint16_t background) {
    const char *label = screen_label;
    const int scale = 4;
    const int top = 36;
    const int left = (WIDTH - (int)strlen(label) * 6 * scale) / 2;
    const uint16_t foreground = background == 0xFFFF || background == 0x07E0
                                    ? 0x0000 : 0xFFFF;
    for (int x = 0; x < WIDTH; ++x) {
        uint16_t pixel = background;
        if (y >= top && y < top + 7 * scale && x >= left &&
            x < left + (int)strlen(label) * 6 * scale) {
            int cell = (x - left) / (6 * scale);
            int gx = ((x - left) % (6 * scale)) / scale;
            const uint8_t *rows = label[cell] ? glyph(label[cell]) : NULL;
            if (rows != NULL && gx < 5 &&
                (rows[(y - top) / scale] & (1U << (4 - gx)))) {
                pixel = foreground;
            }
        }
        line[2 * x] = pixel >> 8;
        line[2 * x + 1] = pixel & 0xff;
    }
}

static inline void fast_set(int pin) { REG_WRITE(GPIO_OUT_W1TS_REG, 1U << pin); }
static inline void fast_clear(int pin) { REG_WRITE(GPIO_OUT_W1TC_REG, 1U << pin); }

static void send_byte_gpio(uint8_t b) {
    if (mode == MODE_GPIO_SLOW) {
        for (int bit = 7; bit >= 0; --bit) {
            gpio_set_level(PIN_SCLK, 0);
            gpio_set_level(PIN_MOSI, (b >> bit) & 1);
            esp_rom_delay_us(1);
            gpio_set_level(PIN_SCLK, 1);
            esp_rom_delay_us(1);
        }
    } else {
        for (int bit = 7; bit >= 0; --bit) {
            fast_clear(PIN_SCLK);
            if ((b >> bit) & 1) fast_set(PIN_MOSI); else fast_clear(PIN_MOSI);
            fast_set(PIN_SCLK);
        }
    }
}

static esp_err_t tx(const uint8_t *data, size_t n) {
    if (mode == MODE_GPIO_SLOW || mode == MODE_GPIO_FAST) {
        for (size_t i = 0; i < n; ++i) send_byte_gpio(data[i]);
        return ESP_OK;
    }
    if (mode == MODE_PARLIO) {
        const parlio_transmit_config_t cfg = { .idle_value = 0 };
        while (n) {
            size_t take = n > sizeof(batch) ? sizeof(batch) : n;
            esp_err_t err = parlio_tx_unit_transmit(parlio, data, take * 8, &cfg);
            if (err == ESP_OK) err = parlio_tx_unit_wait_all_done(parlio, 500);
            if (err != ESP_OK) return err;
            data += take;
            n -= take;
        }
        return ESP_OK;
    }
    while (n) {
        size_t take = n > sizeof(line) ? sizeof(line) : n;
        spi_transaction_t t = { .length = take * 8, .tx_buffer = data };
        esp_err_t err = spi_device_polling_transmit(spi, &t);
        if (err != ESP_OK) return err;
        data += take;
        n -= take;
    }
    return ESP_OK;
}

static esp_err_t command(uint8_t cmd, const uint8_t *args, size_t size) {
    gpio_set_level(PIN_CS, 0);
    gpio_set_level(PIN_DC, 0);
    esp_err_t err = tx(&cmd, 1);
    if (err == ESP_OK && size) {
        gpio_set_level(PIN_DC, 1);
        err = tx(args, size);
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

static esp_err_t full_color(uint16_t rgb565) {
    uint8_t col[] = { 0, X_OFFSET, 0, X_OFFSET + WIDTH - 1 };
    uint8_t row[] = { 0, Y_OFFSET, (HEIGHT - 1) >> 8, (HEIGHT - 1) & 0xff };
    ESP_RETURN_ON_ERROR(command(0x2A, col, sizeof(col)), TAG, "CASET");
    ESP_RETURN_ON_ERROR(command(0x2B, row, sizeof(row)), TAG, "RASET");
    gpio_set_level(PIN_CS, 0);
    gpio_set_level(PIN_DC, 0);
    uint8_t ramwr = 0x2C;
    esp_err_t err = tx(&ramwr, 1);
    if (err == ESP_OK) {
        gpio_set_level(PIN_DC, 1);
        for (int y = 0; y < HEIGHT && err == ESP_OK;) {
            if (mode == MODE_PARLIO) {
                int count = HEIGHT - y < BATCH_ROWS ? HEIGHT - y : BATCH_ROWS;
                for (int j = 0; j < count; ++j) {
                    make_labeled_row(y + j, rgb565);
                    memcpy(batch + j * sizeof(line), line, sizeof(line));
                }
                err = tx(batch, count * sizeof(line));
                y += count;
            } else {
                make_labeled_row(y, rgb565);
                err = tx(line, sizeof(line));
                ++y;
            }
        }
    }
    gpio_set_level(PIN_CS, 1);
    return err;
}

static esp_err_t partial_letter(char letter) {
    enum { W = 32, H = 32, X = (WIDTH - W) / 2, Y = (HEIGHT - H) / 2 };
    uint8_t col[] = {0, X_OFFSET + X, 0, X_OFFSET + X + W - 1};
    uint8_t row[] = {Y >> 8, Y & 0xff, (Y + H - 1) >> 8, (Y + H - 1) & 0xff};
    ESP_RETURN_ON_ERROR(command(0x2A, col, sizeof(col)), TAG, "partial CASET");
    ESP_RETURN_ON_ERROR(command(0x2B, row, sizeof(row)), TAG, "partial RASET");
    uint8_t ramwr = 0x2C;
    gpio_set_level(PIN_CS, 0);
    gpio_set_level(PIN_DC, 0);
    esp_err_t err = tx(&ramwr, 1);
    if (err == ESP_OK) {
        gpio_set_level(PIN_DC, 1);
        const uint8_t *rows = glyph(letter);
        for (int y = 0; y < H && err == ESP_OK; ++y) {
            for (int x = 0; x < W; ++x) {
                int gx = (x - 6) / 4;
                int gy = (y - 2) / 4;
                bool on = x >= 6 && x < 26 && y >= 2 && y < 30 &&
                          rows != NULL && (rows[gy] & (1U << (4 - gx)));
                uint16_t pixel = on ? 0xffff : 0x0000;
                line[2 * x] = pixel >> 8;
                line[2 * x + 1] = pixel & 0xff;
            }
            err = tx(line, W * 2);
        }
    }
    gpio_set_level(PIN_CS, 1);
    return err;
}

static esp_err_t speed_banner(char kind, int64_t us) {
    enum { W = 164, H = 32, X = 4, Y = HEIGHT - H - 8 };
    char message[24];
    int64_t amount = kind == 'F' ? (us + 500) / 1000 : us;
    snprintf(message, sizeof(message), "%c %" PRId64 "%s", kind, amount,
             kind == 'F' ? "MS" : "US");
    const int scale = 3;
    const int text_x = (W - (int)strlen(message) * 6 * scale) / 2;
    uint8_t col[] = {0, X_OFFSET + X, 0, X_OFFSET + X + W - 1};
    uint8_t row[] = {Y >> 8, Y & 0xff, (Y + H - 1) >> 8, (Y + H - 1) & 0xff};
    ESP_RETURN_ON_ERROR(command(0x2A, col, sizeof(col)), TAG, "speed CASET");
    ESP_RETURN_ON_ERROR(command(0x2B, row, sizeof(row)), TAG, "speed RASET");
    uint8_t ramwr = 0x2C;
    gpio_set_level(PIN_CS, 0);
    gpio_set_level(PIN_DC, 0);
    esp_err_t err = tx(&ramwr, 1);
    if (err == ESP_OK) {
        gpio_set_level(PIN_DC, 1);
        for (int y = 0; y < H && err == ESP_OK; ++y) {
            for (int x = 0; x < W; ++x) {
                int index = (x - text_x) / (6 * scale);
                int gx = ((x - text_x) % (6 * scale)) / scale;
                const uint8_t *rows = x >= text_x &&
                    x < text_x + (int)strlen(message) * 6 * scale
                        ? glyph(message[index]) : NULL;
                bool on = rows != NULL && y >= 5 && y < 5 + 7 * scale &&
                          gx < 5 && (rows[(y - 5) / scale] & (1U << (4 - gx)));
                uint16_t pixel = on ? 0xffff : 0x0000;
                line[2 * x] = pixel >> 8;
                line[2 * x + 1] = pixel & 0xff;
            }
            err = tx(line, W * 2);
        }
    }
    gpio_set_level(PIN_CS, 1);
    return err;
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

static esp_err_t setup_spi(void) {
    spi_bus_config_t bus = { .mosi_io_num = PIN_MOSI, .miso_io_num = -1,
                             .sclk_io_num = PIN_SCLK, .quadwp_io_num = -1,
                             .quadhd_io_num = -1,
                             .max_transfer_sz = sizeof(line) };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO), TAG, "SPI bus");
    spi_device_interface_config_t dev = { .clock_speed_hz = 12 * 1000 * 1000,
                                          .mode = 0, .spics_io_num = -1,
                                          .queue_size = 1 };
    return spi_bus_add_device(SPI2_HOST, &dev, &spi);
}

static esp_err_t setup_parlio(void) {
    if (spi != NULL) {
        ESP_RETURN_ON_ERROR(spi_bus_remove_device(spi), TAG, "release SPI device");
        spi = NULL;
        ESP_RETURN_ON_ERROR(spi_bus_free(SPI2_HOST), TAG, "release SPI bus");
    }
    parlio_tx_unit_config_t cfg = {0};
    cfg.clk_src = PARLIO_CLK_SRC_DEFAULT;
    cfg.clk_in_gpio_num = GPIO_NUM_NC;
    cfg.output_clk_freq_hz = 1000000; // Same nominal clock as prior HiYaKa PARLIO trial.
    cfg.data_width = 1; // Native serial pixels; each transfer is far larger than 32 bits.
    for (size_t i = 0; i < sizeof(cfg.data_gpio_nums) / sizeof(cfg.data_gpio_nums[0]); ++i)
        cfg.data_gpio_nums[i] = GPIO_NUM_NC;
    cfg.data_gpio_nums[0] = PIN_MOSI;
    cfg.clk_out_gpio_num = PIN_SCLK;
    cfg.valid_gpio_num = PIN_CS;
    cfg.trans_queue_depth = 1;
    cfg.max_transfer_size = sizeof(batch);
    cfg.dma_burst_size = 64;
    cfg.shift_edge = PARLIO_SHIFT_EDGE_NEG;
    cfg.bit_pack_order = PARLIO_BIT_PACK_ORDER_LSB;
    cfg.flags.invert_valid_out = true; // PARLIO owns active-low LCD CS for each DMA transaction.
    ESP_RETURN_ON_ERROR(parlio_new_tx_unit(&cfg, &parlio), TAG, "PARLIO new unit");
    return parlio_tx_unit_enable(parlio);
}

void app_main(void) {
    ESP_ERROR_CHECK(setup_gpio());
    ESP_ERROR_CHECK(setup_backlight());
    const uint16_t colors[] = {0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0000};
    mode = MODE_GPIO_SLOW;
    ESP_ERROR_CHECK(panel_init());
    screen_label = "BOOT";
    for (int i = 0; i < 5; ++i) {
        ESP_ERROR_CHECK(full_color(colors[i]));
        ESP_LOGI(TAG, "BOOT_SLOW color=%d", i);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    mode = MODE_GPIO_FAST;
    screen_label = "GPIO";
    ESP_LOGI(TAG, "STAGE_GPIO_BEGIN");
    for (int i = 0; i < 3; ++i) {
        int64_t start = esp_timer_get_time();
        ESP_ERROR_CHECK(full_color(colors[i]));
        int64_t elapsed = esp_timer_get_time() - start;
        ESP_LOGI(TAG, "FULL mode=GPIO color=%d us=%" PRId64, i,
                 elapsed);
        ESP_ERROR_CHECK(speed_banner('F', elapsed));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    for (int i = 0; i < 4; ++i) {
        int64_t start = esp_timer_get_time();
        ESP_ERROR_CHECK(partial_letter((i & 1) ? 'B' : 'A'));
        int64_t elapsed = esp_timer_get_time() - start;
        ESP_LOGI(TAG, "PARTIAL mode=GPIO index=%d bytes=2048 us=%" PRId64,
                 i, elapsed);
        ESP_ERROR_CHECK(speed_banner('P', elapsed));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "STAGE_GPIO_END");

    ESP_ERROR_CHECK(setup_spi());
    mode = MODE_SPI2;
    screen_label = "SPIDMA";
    ESP_LOGI(TAG, "STAGE_SPI_DMA_BEGIN");
    for (int i = 0; i < 3; ++i) {
        int64_t start = esp_timer_get_time();
        ESP_ERROR_CHECK(full_color(colors[i]));
        int64_t elapsed = esp_timer_get_time() - start;
        ESP_LOGI(TAG, "FULL mode=SPI_DMA color=%d us=%" PRId64, i,
                 elapsed);
        ESP_ERROR_CHECK(speed_banner('F', elapsed));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    for (int i = 0; i < 5; ++i) {
        int64_t start = esp_timer_get_time();
        ESP_ERROR_CHECK(partial_letter((i & 1) ? 'B' : 'A'));
        int64_t elapsed = esp_timer_get_time() - start;
        ESP_LOGI(TAG, "PARTIAL mode=SPI_DMA index=%d bytes=2048 us=%" PRId64,
                 i, elapsed);
        ESP_ERROR_CHECK(speed_banner('P', elapsed));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "STAGE_SPI_DMA_END");

    ESP_ERROR_CHECK(spi_bus_remove_device(spi));
    spi = NULL;
    ESP_ERROR_CHECK(spi_bus_free(SPI2_HOST));
    gpio_config_t lcd_pins = {
        .pin_bit_mask = (1ULL << PIN_MOSI) | (1ULL << PIN_SCLK),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&lcd_pins));
    gpio_set_level(PIN_SCLK, 0);
    gpio_set_level(PIN_MOSI, 0);
    mode = MODE_GPIO_SLOW;
    uint8_t col[] = {0, X_OFFSET, 0, X_OFFSET + WIDTH - 1};
    uint8_t row[] = {0, 0, (HEIGHT - 1) >> 8, (HEIGHT - 1) & 0xff};
    ESP_ERROR_CHECK(command(0x2A, col, sizeof(col)));
    ESP_ERROR_CHECK(command(0x2B, row, sizeof(row)));
    gpio_set_level(PIN_CS, 0);
    gpio_set_level(PIN_DC, 0);
    uint8_t ramwr = 0x2C;
    ESP_ERROR_CHECK(tx(&ramwr, 1));
    gpio_set_level(PIN_DC, 1);
    gpio_set_level(PIN_CS, 1); // GPIO releases CS before PARLIO claims it as VALID.

    ESP_ERROR_CHECK(setup_parlio());
    mode = MODE_PARLIO;
    screen_label = "PARLIO";
    ESP_LOGI(TAG, "PARLIO_VALID_CS pin=%d active_low=1 rows_per_tx=%d", PIN_CS, BATCH_ROWS);
    ESP_LOGI(TAG, "STAGE_PARLIO_PIXELS_ONLY_BEGIN");
    int64_t start = esp_timer_get_time();
    for (int y = 0; y < HEIGHT; y += BATCH_ROWS) {
        int count = HEIGHT - y < BATCH_ROWS ? HEIGHT - y : BATCH_ROWS;
        for (int j = 0; j < count; ++j) {
            make_labeled_row(y + j, 0x07E0);
            memcpy(batch + j * sizeof(line), line, sizeof(line));
        }
        ESP_ERROR_CHECK(tx(batch, count * sizeof(line)));
    }
    ESP_LOGI(TAG, "STAGE_PARLIO_PIXELS_ONLY_DONE us=%" PRId64,
             esp_timer_get_time() - start);
    while (true) vTaskDelay(pdMS_TO_TICKS(1000));
}
