#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

static const char *TAG = "HiYaKaPcaCross";
static bool soft_fault;
enum { SDA = 10, SCL = 11, PA = 23, PCA = 0x19, QMI = 0x6B,
       EMPTY = 0x55, SAFE_OUT = 0x40, SAFE_CFG = 0x38 };
enum { TRIALS = 8, IDLE_US = 20000, HALF_US = 5 };

typedef struct {
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t pca;
    i2c_master_dev_handle_t qmi;
} hw_t;

static void release_lines(void)
{
    gpio_set_level(SDA, 1);
    gpio_set_level(SCL, 1);
}

static bool lines_high(void)
{
    release_lines();
    esp_rom_delay_us(20);
    return gpio_get_level(SDA) && gpio_get_level(SCL);
}

static bool pa_off(void)
{
    return gpio_set_level(PA, 0) == ESP_OK && gpio_get_level(PA) == 0;
}

static void half(void) { esp_rom_delay_us(HALF_US); }
static void sda(bool high) { gpio_set_level(SDA, high); }
static void scl(bool high) { gpio_set_level(SCL, high); }

static bool scl_high(void)
{
    scl(true);
    for (unsigned n = 0; n < 1000; ++n) {
        if (gpio_get_level(SCL)) return true;
        esp_rom_delay_us(1);
    }
    soft_fault = true;
    return false;
}

static bool start(void)
{
    sda(true); half();
    if (!scl_high()) return false;
    half();
    if (!gpio_get_level(SDA)) return false;
    sda(false); half(); scl(false); half();
    return true;
}

static void stop(void)
{
    sda(false); half();
    (void)scl_high(); half();
    sda(true); half();
}

// Return 0=ACK, 1=NACK, -1=SCL timeout.
static int write_byte(uint8_t value)
{
    for (unsigned bit = 0; bit < 8; ++bit) {
        sda((value & 0x80U) != 0); half();
        if (!scl_high()) return -1;
        half(); scl(false); half(); value <<= 1;
    }
    sda(true); half();
    if (!scl_high()) return -1;
    half();
    int nack = gpio_get_level(SDA);
    scl(false); half();
    return nack;
}

static int soft_write(uint8_t addr, uint8_t reg, uint8_t value,
                      int *addr_ack, int *reg_ack, int *data_ack)
{
    *addr_ack = *reg_ack = *data_ack = -2;
    if (!start()) { stop(); return -1; }
    *addr_ack = write_byte((uint8_t)(addr << 1));
    if (*addr_ack != 0) { stop(); return 1; }
    *reg_ack = write_byte(reg);
    if (*reg_ack != 0) { stop(); return 2; }
    *data_ack = write_byte(value);
    stop();
    return *data_ack == 0 ? 0 : 3;
}

static int soft_read_byte(uint8_t *value)
{
    sda(true);
    uint8_t out = 0;
    for (unsigned bit = 0; bit < 8; ++bit) {
        half();
        if (!scl_high()) return -1;
        half(); out = (uint8_t)((out << 1) | (gpio_get_level(SDA) ? 1 : 0));
        scl(false); half();
    }
    sda(true); half();
    if (!scl_high()) return -1;
    half(); scl(false); half();
    *value = out;
    return 0;
}

static bool soft_read(uint8_t addr, uint8_t reg, uint8_t *value)
{
    if (!start()) { stop(); return false; }
    if (write_byte((uint8_t)(addr << 1)) != 0 || write_byte(reg) != 0) {
        stop(); return false;
    }
    if (!start() || write_byte((uint8_t)((addr << 1) | 1)) != 0) {
        stop(); return false;
    }
    bool ok = soft_read_byte(value) == 0;
    stop();
    return ok;
}

static bool soft_safe(void)
{
    if (soft_fault) return false;
    int a, r, d;
    for (unsigned n = 0; n < 5; ++n) {
        if (soft_write(PCA, 0x01, SAFE_OUT, &a, &r, &d) == 0 &&
            soft_write(PCA, 0x03, SAFE_CFG, &a, &r, &d) == 0) {
            bool verified = true;
            for (unsigned pass = 0; pass < 2; ++pass) {
                uint8_t out = 0, cfg = 0;
                verified &= soft_read(PCA, 0x01, &out) &&
                            soft_read(PCA, 0x03, &cfg) &&
                            out == SAFE_OUT && cfg == SAFE_CFG && !soft_fault;
                if (!verified) break;
            }
            if (verified) return true;
        }
    }
    return false;
}

static esp_err_t add_dev(i2c_master_bus_handle_t bus, uint8_t addr,
                         i2c_master_dev_handle_t *dev)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr, .scl_speed_hz = 100000,
    };
    return i2c_master_bus_add_device(bus, &cfg, dev);
}

static void close_hw(hw_t *hw)
{
    if (hw->qmi) i2c_master_bus_rm_device(hw->qmi);
    if (hw->pca) i2c_master_bus_rm_device(hw->pca);
    if (hw->bus) i2c_del_master_bus(hw->bus);
    *hw = (hw_t){0};
}

static bool open_hw(hw_t *hw)
{
    const i2c_master_bus_config_t cfg = {
        .i2c_port = I2C_NUM_0, .sda_io_num = SDA, .scl_io_num = SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7,
    };
    if (i2c_new_master_bus(&cfg, &hw->bus) != ESP_OK ||
        add_dev(hw->bus, PCA, &hw->pca) != ESP_OK ||
        add_dev(hw->bus, QMI, &hw->qmi) != ESP_OK) {
        close_hw(hw);
        return false;
    }
    if (!gpio_get_level(SDA) || !gpio_get_level(SCL)) {
        close_hw(hw);
        return false;
    }
    return true;
}

static bool hw_read(hw_t *hw, uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(hw->pca, &reg, 1, value, 1, 100) == ESP_OK;
}

static bool hw_safe(hw_t *hw)
{
    const uint8_t out[] = {0x01, SAFE_OUT}, cfg[] = {0x03, SAFE_CFG};
    for (unsigned n = 0; n < 5; ++n) {
        if (i2c_master_transmit(hw->pca, out, 2, 100) == ESP_OK &&
            i2c_master_transmit(hw->pca, cfg, 2, 100) == ESP_OK) {
            bool verified = true;
            for (unsigned pass = 0; pass < 2; ++pass) {
                uint8_t a = 0, b = 0;
                verified &= hw_read(hw, 0x01, &a) &&
                            hw_read(hw, 0x03, &b) &&
                            a == SAFE_OUT && b == SAFE_CFG;
                if (!verified) break;
            }
            if (verified) return true;
        }
    }
    return false;
}

static bool open_soft(void)
{
    soft_fault = false;
    release_lines();
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << SDA) | (1ULL << SCL),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    if (gpio_config(&cfg) == ESP_OK && lines_high()) return true;
    release_lines();
    gpio_reset_pin(SDA);
    gpio_reset_pin(SCL);
    return false;
}

static void close_soft(void)
{
    release_lines();
    gpio_reset_pin(SDA);
    gpio_reset_pin(SCL);
}

static bool phase(bool soft, unsigned phase_no)
{
    hw_t hw = {0};
    if (!pa_off()) return false;
    if (soft ? !open_soft() : !open_hw(&hw)) return false;
    bool valid = soft ? soft_safe() : hw_safe(&hw);
    ESP_LOGI(TAG, "PHASE %u engine=%s safe=%u pa=%u",
             phase_no, soft ? "soft" : "hw", valid, pa_off());
    if (!valid) goto done;
    for (unsigned n = 0; n < TRIALS; ++n) {
        if (!pa_off()) { valid = false; break; }
        int64_t idle_begin = esp_timer_get_time();
        esp_rom_delay_us(IDLE_US);
        int64_t begin = esp_timer_get_time();
        if (soft) {
            if (!lines_high()) { valid = false; break; }
            int a1, r1, d1, a2, r2, d2;
            int64_t t1 = esp_timer_get_time();
            int first = soft_write(PCA, 0x01, SAFE_OUT, &a1, &r1, &d1);
            int64_t t2 = esp_timer_get_time();
            int second = soft_write(PCA, 0x01, SAFE_OUT, &a2, &r2, &d2);
            int64_t t3 = esp_timer_get_time();
            ESP_LOGI(TAG, "TRIAL phase=%u idx=%u engine=soft idle_us=%lld first=%d ack=%d/%d/%d dur_us=%lld second=%d ack=%d/%d/%d dur_us=%lld",
                     phase_no, n, (long long)(begin - idle_begin),
                     first, a1, r1, d1, (long long)(t2 - t1),
                     second, a2, r2, d2, (long long)(t3 - t2));
        } else {
            const uint8_t bytes[] = {0x01, SAFE_OUT};
            int64_t t1 = esp_timer_get_time();
            esp_err_t first = i2c_master_transmit(hw.pca, bytes, 2, 100);
            int64_t t2 = esp_timer_get_time();
            esp_err_t second = i2c_master_transmit(hw.pca, bytes, 2, 100);
            int64_t t3 = esp_timer_get_time();
            ESP_LOGI(TAG, "TRIAL phase=%u idx=%u engine=hw idle_us=%lld first=%s dur_us=%lld second=%s dur_us=%lld",
                     phase_no, n, (long long)(begin - idle_begin),
                     esp_err_to_name(first), (long long)(t2 - t1),
                     esp_err_to_name(second), (long long)(t3 - t2));
        }
        uint8_t out = 0, cfg = 0;
        bool read_ok = soft ? (soft_read(PCA, 0x01, &out) &&
                               soft_read(PCA, 0x03, &cfg))
                            : (hw_read(&hw, 0x01, &out) &&
                               hw_read(&hw, 0x03, &cfg));
        valid = read_ok && out == SAFE_OUT && cfg == SAFE_CFG && pa_off() &&
                (!soft || !soft_fault);
        ESP_LOGI(TAG, "VERIFY phase=%u idx=%u ok=%u out=%02x cfg=%02x",
                 phase_no, n, valid, out, cfg);
        if (!valid) break;
        // Positive QMI ID register and empty-address negative control after
        // target verification, never before the measured idle-first access.
        uint8_t who = 0;
        bool qmi_ok = false, empty_nack = false;
        if (soft) {
            qmi_ok = soft_read(QMI, 0x00, &who) && who == 0x05;
            int a, r, d;
            (void)soft_write(EMPTY, 0, 0, &a, &r, &d);
            empty_nack = a == 1;
        } else {
            qmi_ok = i2c_master_transmit_receive(hw.qmi, (uint8_t[]){0}, 1,
                                                  &who, 1, 100) == ESP_OK && who == 0x05;
            empty_nack = i2c_master_probe(hw.bus, EMPTY, 10) == ESP_ERR_NOT_FOUND;
        }
        ESP_LOGI(TAG, "CONTROL phase=%u idx=%u qmi=%u who=%02x empty_nack=%u",
                 phase_no, n, qmi_ok, who, empty_nack);
        if (!qmi_ok || !empty_nack) { valid = false; break; }
    }
done:
    if (soft) {
        bool safe = soft_safe();
        ESP_LOGI(TAG, "FINAL phase=%u safe=%u pa=%u", phase_no, safe, pa_off());
        valid &= safe && !soft_fault;
        close_soft();
    } else {
        bool safe = hw_safe(&hw);
        ESP_LOGI(TAG, "FINAL phase=%u safe=%u pa=%u", phase_no, safe, pa_off());
        valid &= safe;
        close_hw(&hw);
    }
    return valid;
}

void app_main(void)
{
#if !CONFIG_HI_YAKA_PCA_STRAPS_FIXED
    ESP_LOGW(TAG, "HOLD: PN532 mode straps require physical confirmation; no I2C trial");
    return;
#endif
    uint8_t mac[6] = {0};
    if (esp_efuse_mac_get_default(mac) == ESP_OK) {
        ESP_LOGI(TAG, "BOARD_MAC %02x:%02x:%02x:%02x:%02x:%02x PN532 straps=externally confirmed",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    gpio_set_level(PA, 0);
    const gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << PA, .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
    };
    if (gpio_config(&cfg) != ESP_OK || !pa_off()) {
        ESP_LOGE(TAG, "PA gate failed"); return;
    }
    const bool order[] = {false, true, true, false};
    for (unsigned n = 0; n < sizeof(order) / sizeof(order[0]); ++n) {
        if (!phase(order[n], n)) {
            ESP_LOGE(TAG, "ABBA INVALID phase=%u; no causal conclusion", n);
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_LOGI(TAG, "ABBA COMPLETE pa=%u; real-board evidence pending", pa_off());
}
