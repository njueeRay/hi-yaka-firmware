#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "HiYaKaNfcAB";
static const uint8_t GET_FIRMWARE_VERSION[] = {
    0x00, 0x00, 0xFF, 0x02, 0xFE, 0xD4, 0x02, 0x2A, 0x00,
};

static void probe_i2c(void)
{
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_10,
        .scl_io_num = GPIO_NUM_11,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus = NULL;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus));
    const uint8_t addresses[] = {0x18, 0x19, 0x24, 0x41, 0x6B};

    for (int round = 0; round < 3; ++round) {
        for (size_t i = 0; i < sizeof(addresses); ++i) {
            const esp_err_t result = i2c_master_probe(bus, addresses[i], 100);
            ESP_LOGI(TAG, "I2C round=%d addr=0x%02X result=%s", round,
                     addresses[i], esp_err_to_name(result));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x24,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t device = NULL;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &device_config, &device));
    const esp_err_t command_result = i2c_master_transmit(
        device, GET_FIRMWARE_VERSION, sizeof(GET_FIRMWARE_VERSION), 500);
    ESP_LOGI(TAG, "I2C raw GetFirmwareVersion result=%s",
             esp_err_to_name(command_result));
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(device));
    ESP_ERROR_CHECK(i2c_del_master_bus(bus));
}

static void probe_hsu(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    // PN532 pin 27 (HSU_RX) is wired to GPIO11; pin 28 (HSU_TX) to GPIO10.
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, GPIO_NUM_11, GPIO_NUM_10,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    for (int round = 0; round < 3; ++round) {
        uint8_t response[128] = {0};
        uart_flush_input(UART_NUM_1);
        const uint8_t wake = 0x55;
        uart_write_bytes(UART_NUM_1, &wake, 1);
        uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(100));
        vTaskDelay(pdMS_TO_TICKS(10));
        uart_write_bytes(UART_NUM_1, GET_FIRMWARE_VERSION,
                         sizeof(GET_FIRMWARE_VERSION));
        uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(100));
        const int read_count = uart_read_bytes(
            UART_NUM_1, response, sizeof(response), pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "HSU round=%d rx_bytes=%d", round, read_count);
        if (read_count > 0) {
            ESP_LOG_BUFFER_HEX(TAG, response, read_count);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_ERROR_CHECK(uart_driver_delete(UART_NUM_1));
}

void app_main(void)
{
    // L7 is physically removed on the test board. Force PA_EN low as early as
    // possible; this experiment does not start audio, display or the RF field.
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_23, 0));
    ESP_LOGI(TAG, "start: PA_EN=0; I2C then HSU on GPIO10/11");
    vTaskDelay(pdMS_TO_TICKS(1000));
    probe_i2c();
    probe_hsu();
    ESP_LOGI(TAG, "complete; PA_EN remains low");
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
