#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "HiYaKaI2cIsolation";

typedef struct {
    unsigned ok;
    unsigned nack;
    unsigned not_found;
    unsigned timeout;
    unsigned other;
} result_counts_t;

#define RETRY_LIMIT 5

typedef struct {
    result_counts_t attempt[RETRY_LIMIT];
    unsigned success_on[RETRY_LIMIT];
    unsigned exhausted;
} retry5_counts_t;

static void count_result(result_counts_t *counts, esp_err_t result)
{
    if (result == ESP_OK) {
        ++counts->ok;
    } else if (result == ESP_ERR_INVALID_RESPONSE) {
        ++counts->nack;
    } else if (result == ESP_ERR_NOT_FOUND) {
        ++counts->not_found;
    } else if (result == ESP_ERR_TIMEOUT) {
        ++counts->timeout;
    } else {
        ++counts->other;
    }
}

static esp_err_t add_device(i2c_master_bus_handle_t bus, uint8_t address,
                            uint32_t speed, i2c_master_dev_handle_t *device)
{
    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = speed,
    };
    return i2c_master_bus_add_device(bus, &config, device);
}

static esp_err_t read_register(i2c_master_dev_handle_t device, uint8_t reg,
                               uint8_t *value)
{
    return i2c_master_transmit_receive(device, &reg, 1, value, 1, 100);
}

static esp_err_t read_register_retry5(i2c_master_dev_handle_t device,
                                      uint8_t reg, uint8_t *value,
                                      retry5_counts_t *counts)
{
    // Product firmware retries immediately and silently, up to five attempts.
    // Keep all attempts in one idle window so this measures that exact path.
    esp_err_t result = ESP_FAIL;
    for (unsigned i = 0; i < RETRY_LIMIT; ++i) {
        result = read_register(device, reg, value);
        count_result(&counts->attempt[i], result);
        if (result == ESP_OK) {
            ++counts->success_on[i];
            return result;
        }
    }
    ++counts->exhausted;
    return result;
}

static esp_err_t probe_retry5(i2c_master_bus_handle_t bus, uint8_t address,
                              retry5_counts_t *counts)
{
    esp_err_t result = ESP_FAIL;
    for (unsigned i = 0; i < RETRY_LIMIT; ++i) {
        result = i2c_master_probe(bus, address, 100);
        count_result(&counts->attempt[i], result);
        if (result == ESP_OK) {
            ++counts->success_on[i];
            return result;
        }
    }
    ++counts->exhausted;
    return result;
}

static esp_err_t transmit_retry5(i2c_master_dev_handle_t device,
                                 const uint8_t *data, size_t size,
                                 retry5_counts_t *counts)
{
    esp_err_t result = ESP_FAIL;
    for (unsigned i = 0; i < RETRY_LIMIT; ++i) {
        result = i2c_master_transmit(device, data, size, 100);
        count_result(&counts->attempt[i], result);
        if (result == ESP_OK) {
            ++counts->success_on[i];
            return result;
        }
    }
    ++counts->exhausted;
    return result;
}

static void print_counts(const char *name, const result_counts_t *counts)
{
    ESP_LOGI(TAG,
             "%s ok=%u nack=%u not_found=%u timeout=%u other=%u", name,
             counts->ok, counts->nack, counts->not_found, counts->timeout,
             counts->other);
}

static void print_retry5_counts(const char *name,
                                const retry5_counts_t *counts)
{
    ESP_LOGI(TAG, "%s success_on=1:%u,2:%u,3:%u,4:%u,5:%u exhausted=%u",
             name, counts->success_on[0], counts->success_on[1],
             counts->success_on[2], counts->success_on[3],
             counts->success_on[4], counts->exhausted);
    for (unsigned i = 0; i < RETRY_LIMIT; ++i) {
        ESP_LOGI(TAG,
                 "%s attempt=%u ok=%u nack=%u not_found=%u timeout=%u "
                 "other=%u",
                 name, i + 1, counts->attempt[i].ok,
                 counts->attempt[i].nack, counts->attempt[i].not_found,
                 counts->attempt[i].timeout, counts->attempt[i].other);
    }
}

static void run_idle_discriminator(i2c_master_bus_handle_t bus)
{
    i2c_master_dev_handle_t pca = NULL;
    i2c_master_dev_handle_t qmi = NULL;
    ESP_ERROR_CHECK(add_device(bus, 0x19, 100000, &pca));
    ESP_ERROR_CHECK(add_device(bus, 0x6B, 100000, &qmi));

    enum { SAMPLE_COUNT = 64 };
    uint8_t value = 0;
    result_counts_t reset_immediate = {0};
    result_counts_t pca_after_reset_immediate = {0};
    result_counts_t reset_delayed = {0};
    result_counts_t pca_after_reset_delayed = {0};

    // A reset can both reset the controller FSM and generate bus-clear
    // activity.  Comparing immediate access with access after another 10 ms
    // idle separates a persistent FSM correction from a short-lived primer.
    for (unsigned i = 0; i < SAMPLE_COUNT; ++i) {
        esp_rom_delay_us(10000);
        count_result(&reset_immediate, i2c_master_bus_reset(bus));
        count_result(&pca_after_reset_immediate,
                     read_register(pca, 0x01, &value));
    }
    print_counts("bus reset before immediate PCA", &reset_immediate);
    print_counts("PCA immediately after bus reset",
                 &pca_after_reset_immediate);

    for (unsigned i = 0; i < SAMPLE_COUNT; ++i) {
        esp_rom_delay_us(10000);
        count_result(&reset_delayed, i2c_master_bus_reset(bus));
        esp_rom_delay_us(10000);
        count_result(&pca_after_reset_delayed,
                     read_register(pca, 0x01, &value));
    }
    print_counts("bus reset before delayed PCA", &reset_delayed);
    print_counts("PCA 10000us after bus reset", &pca_after_reset_delayed);

    // Quantify how long the successful-primer effect survives.  Use both a
    // real target ACK (QMI) and an expected empty-address NACK so the result
    // does not depend on a PCA-specific command or QMI register semantics.
    const uint32_t gaps_us[] = {0, 100, 250, 500, 1000,
                                2000, 5000, 10000, 20000};
    for (unsigned gap = 0; gap < sizeof(gaps_us) / sizeof(gaps_us[0]); ++gap) {
        result_counts_t qmi_prime = {0};
        result_counts_t pca_after_qmi = {0};
        result_counts_t empty_prime = {0};
        result_counts_t pca_after_empty = {0};
        for (unsigned i = 0; i < SAMPLE_COUNT; ++i) {
            esp_rom_delay_us(10000);
            count_result(&qmi_prime, read_register(qmi, 0x00, &value));
            esp_rom_delay_us(gaps_us[gap]);
            count_result(&pca_after_qmi, read_register(pca, 0x01, &value));
        }
        for (unsigned i = 0; i < SAMPLE_COUNT; ++i) {
            esp_rom_delay_us(10000);
            count_result(&empty_prime, i2c_master_probe(bus, 0x55, 100));
            esp_rom_delay_us(gaps_us[gap]);
            count_result(&pca_after_empty,
                         read_register(pca, 0x01, &value));
        }

        char label[80];
        snprintf(label, sizeof(label), "gap=%luus QMI prime",
                 (unsigned long)gaps_us[gap]);
        print_counts(label, &qmi_prime);
        snprintf(label, sizeof(label), "gap=%luus PCA after QMI",
                 (unsigned long)gaps_us[gap]);
        print_counts(label, &pca_after_qmi);
        snprintf(label, sizeof(label), "gap=%luus empty prime",
                 (unsigned long)gaps_us[gap]);
        print_counts(label, &empty_prime);
        snprintf(label, sizeof(label), "gap=%luus PCA after empty",
                 (unsigned long)gaps_us[gap]);
        print_counts(label, &pca_after_empty);
    }

    ESP_ERROR_CHECK(i2c_master_bus_rm_device(qmi));
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(pca));
}

static void run_phase(i2c_master_bus_handle_t bus, uint32_t speed, int phase)
{
    i2c_master_dev_handle_t pca = NULL;
    i2c_master_dev_handle_t es7210 = NULL;
    i2c_master_dev_handle_t qmi = NULL;
    const esp_err_t pca_added = add_device(bus, 0x19, speed, &pca);
    const esp_err_t es7210_added = add_device(bus, 0x41, speed, &es7210);
    const esp_err_t qmi_added = add_device(bus, 0x6B, speed, &qmi);
    if (pca_added != ESP_OK || es7210_added != ESP_OK || qmi_added != ESP_OK) {
        ESP_LOGE(TAG, "phase=%d add device failed: PCA=%s ES7210=%s QMI=%s",
                 phase, esp_err_to_name(pca_added),
                 esp_err_to_name(es7210_added), esp_err_to_name(qmi_added));
        goto cleanup;
    }

    ESP_LOGI(TAG, "phase=%d speed=%lu idle SDA=%d SCL=%d", phase,
             (unsigned long)speed, gpio_get_level(GPIO_NUM_10),
             gpio_get_level(GPIO_NUM_11));
    result_counts_t pca_only = {0};
    result_counts_t es7210_only = {0};
    retry5_counts_t pca_retry5 = {0};
    retry5_counts_t es7210_retry5 = {0};
    retry5_counts_t qmi_retry5 = {0};
    retry5_counts_t pca_probe_retry5 = {0};
    retry5_counts_t es7210_probe_retry5 = {0};
    retry5_counts_t qmi_probe_retry5 = {0};
    retry5_counts_t pca_write_retry5 = {0};
    result_counts_t qmi_prime = {0};
    result_counts_t pca_after_qmi = {0};
    result_counts_t empty_prime = {0};
    result_counts_t pca_after_empty = {0};
    result_counts_t pca_probe = {0};
    result_counts_t es7210_probe = {0};
    result_counts_t pca_split_pointer = {0};
    result_counts_t pca_split_data = {0};
    result_counts_t es7210_split_pointer = {0};
    result_counts_t es7210_split_data = {0};
    result_counts_t pca_read = {0};
    result_counts_t pca_write = {0};
    result_counts_t es7210_read = {0};
    result_counts_t qmi_read = {0};
    result_counts_t es8311_probe = {0};
    uint8_t value = 0;
    for (unsigned i = 0; i < 128; ++i) {
        count_result(&pca_only, read_register(pca, 0x01, &value));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    print_counts("PCA-only read 0x01", &pca_only);

    for (unsigned i = 0; i < 128; ++i) {
        count_result(&es7210_only, read_register(es7210, 0x0B, &value));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    print_counts("ES7210-only read 0x0B", &es7210_only);

    for (unsigned i = 0; i < 128; ++i) {
        read_register_retry5(pca, 0x01, &value, &pca_retry5);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    print_retry5_counts("retry5 PCA read 0x01", &pca_retry5);

    for (unsigned i = 0; i < 128; ++i) {
        read_register_retry5(es7210, 0x0B, &value, &es7210_retry5);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    print_retry5_counts("retry5 ES7210 read 0x0B", &es7210_retry5);

    for (unsigned i = 0; i < 128; ++i) {
        read_register_retry5(qmi, 0x00, &value, &qmi_retry5);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    print_retry5_counts("retry5 QMI8658A read 0x00", &qmi_retry5);

    // Address-only probes are deliberately first after each 10 ms idle
    // interval.  They distinguish an address ACK failure from a register-read
    // or repeated-start failure.  Run each target in its own block so one
    // target cannot prime the next target within the same sample.
    for (unsigned i = 0; i < 128; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
        probe_retry5(bus, 0x19, &pca_probe_retry5);
    }
    print_retry5_counts("idle-first retry5 PCA probe 0x19",
                        &pca_probe_retry5);

    for (unsigned i = 0; i < 128; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
        probe_retry5(bus, 0x41, &es7210_probe_retry5);
    }
    print_retry5_counts("idle-first retry5 ES7210 probe 0x41",
                        &es7210_probe_retry5);

    for (unsigned i = 0; i < 128; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
        probe_retry5(bus, 0x6B, &qmi_probe_retry5);
    }
    print_retry5_counts("idle-first retry5 QMI8658A probe 0x6B",
                        &qmi_probe_retry5);

    // Match the product's PCA9557 write path without changing any unsafe
    // output: 0x01=0x40 keeps LCD deselected and motor/PA controls low.
    const uint8_t safe_output[] = {0x01, 0x40};
    for (unsigned i = 0; i < 128; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
        transmit_retry5(pca, safe_output, sizeof(safe_output),
                        &pca_write_retry5);
    }
    print_retry5_counts("idle-first retry5 PCA safe write 0x40",
                        &pca_write_retry5);

    // A successful QMI transaction and a deliberately NACKed empty-address
    // probe are two different primers.  If either makes the immediately
    // following PCA read succeed, the effect is tied to preceding bus
    // activity rather than a PCA register wake-up command.
    for (unsigned i = 0; i < 128; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
        count_result(&qmi_prime, read_register(qmi, 0x00, &value));
        count_result(&pca_after_qmi, read_register(pca, 0x01, &value));
    }
    print_counts("idle-first QMI prime read 0x00", &qmi_prime);
    print_counts("PCA read immediately after QMI", &pca_after_qmi);

    for (unsigned i = 0; i < 128; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
        count_result(&empty_prime, i2c_master_probe(bus, 0x55, 100));
        count_result(&pca_after_empty, read_register(pca, 0x01, &value));
    }
    print_counts("idle-first empty probe 0x55", &empty_prime);
    print_counts("PCA read immediately after empty probe", &pca_after_empty);

    const uint8_t pca_reg = 0x01;
    const uint8_t es7210_reg = 0x0B;
    for (unsigned i = 0; i < 16; ++i) {
        const esp_err_t pca_pointer = i2c_master_transmit(pca, &pca_reg, 1, 100);
        count_result(&pca_split_pointer, pca_pointer);
        if (pca_pointer == ESP_OK) {
            count_result(&pca_split_data, i2c_master_receive(pca, &value, 1, 100));
        }
        const esp_err_t es7210_pointer =
            i2c_master_transmit(es7210, &es7210_reg, 1, 100);
        count_result(&es7210_split_pointer, es7210_pointer);
        if (es7210_pointer == ESP_OK) {
            count_result(&es7210_split_data,
                         i2c_master_receive(es7210, &value, 1, 100));
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    print_counts("split PCA pointer 0x01", &pca_split_pointer);
    print_counts("split PCA data", &pca_split_data);
    print_counts("split ES7210 pointer 0x0B", &es7210_split_pointer);
    print_counts("split ES7210 data", &es7210_split_data);

    for (unsigned i = 0; i < 128; ++i) {
        count_result(&pca_read, read_register(pca, 0x01, &value));
        count_result(&pca_write,
                     i2c_master_transmit(pca, safe_output,
                                         sizeof(safe_output), 100));
        count_result(&es7210_read,
                     read_register(es7210, 0x0B, &value));
        count_result(&qmi_read, read_register(qmi, 0x00, &value));
        if ((i % 8) == 0) {
            count_result(&pca_probe, i2c_master_probe(bus, 0x19, 100));
            count_result(&es7210_probe, i2c_master_probe(bus, 0x41, 100));
            count_result(&es8311_probe, i2c_master_probe(bus, 0x18, 100));
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    print_counts("mixed PCA read 0x01", &pca_read);
    print_counts("mixed PCA safe write 0x40", &pca_write);
    print_counts("mixed ES7210 read 0x0B", &es7210_read);
    print_counts("mixed QMI8658A read 0x00", &qmi_read);
    print_counts("mixed PCA probe 0x19", &pca_probe);
    print_counts("mixed ES7210 probe 0x41", &es7210_probe);
    print_counts("mixed ES8311 probe 0x18", &es8311_probe);
    ESP_LOGI(TAG, "phase=%d done SDA=%d SCL=%d", phase,
             gpio_get_level(GPIO_NUM_10), gpio_get_level(GPIO_NUM_11));

cleanup:
    if (qmi != NULL) {
        i2c_master_bus_rm_device(qmi);
    }
    if (es7210 != NULL) {
        i2c_master_bus_rm_device(es7210);
    }
    if (pca != NULL) {
        i2c_master_bus_rm_device(pca);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_23, 0));
    ESP_LOGI(TAG, "start PA_EN=0; no display/audio/motor/PN532 commands");

    const i2c_master_bus_config_t config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_10,
        .scl_io_num = GPIO_NUM_11,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus = NULL;
    const esp_err_t result = i2c_new_master_bus(&config, &bus);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "create bus failed: %s", esp_err_to_name(result));
    } else {
        const uint32_t speeds[] = {100000, 25000, 25000, 100000};
        for (int phase = 0; phase < 4; ++phase) {
            run_phase(bus, speeds[phase], phase);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        run_idle_discriminator(bus);
        ESP_ERROR_CHECK(i2c_del_master_bus(bus));
    }
    ESP_LOGI(TAG, "complete PA_EN=0; idle forever");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
