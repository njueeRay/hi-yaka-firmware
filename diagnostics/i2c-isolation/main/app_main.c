#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "HiYaKaI2cIsolation";

typedef struct {
    unsigned ok;
    unsigned nack;
    unsigned timeout;
    unsigned other;
} result_counts_t;

static void count_result(result_counts_t *counts, esp_err_t result)
{
    if (result == ESP_OK) {
        ++counts->ok;
    } else if (result == ESP_ERR_INVALID_RESPONSE) {
        ++counts->nack;
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

static void print_counts(const char *name, const result_counts_t *counts)
{
    ESP_LOGI(TAG, "%s ok=%u nack=%u timeout=%u other=%u", name, counts->ok,
             counts->nack, counts->timeout, counts->other);
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

    const uint8_t safe_output[] = {0x01, 0x40};
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
        ESP_ERROR_CHECK(i2c_del_master_bus(bus));
    }
    ESP_LOGI(TAG, "complete PA_EN=0; idle forever");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
