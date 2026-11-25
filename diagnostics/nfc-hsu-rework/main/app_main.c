#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "HiYaKaNfcHsu";
static const uint8_t VERSION[] =
    {0x00, 0x00, 0xFF, 0x02, 0xFE, 0xD4, 0x02, 0x2A, 0x00};
static const uint8_t WAKE_AND_SAM_CONFIG[] =
    {0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
     0x05, 0xFB, 0xD4, 0x14, 0x01, 0x14, 0x01, 0x02, 0x00};
static const uint8_t IN_LIST_TYPE_A[] =
    {0x00, 0x00, 0xFF, 0x04, 0xFC, 0xD4, 0x4A, 0x01, 0x00, 0xE1, 0x00};
static const uint8_t HOST_ACK[] =
    {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};

static bool command(const char *name, const uint8_t *frame, size_t frame_size,
                    uint8_t response_code, uint32_t wait_ms)
{
    uint8_t reply[128] = {0};
    ESP_LOGI(TAG, "%s before tx GPIO10 level=%d", name,
             gpio_get_level(GPIO_NUM_10));
    uart_flush_input(UART_NUM_1);
    uart_write_bytes(UART_NUM_1, frame, frame_size);
    uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(200));
    int count = 0;
    const int64_t deadline = esp_timer_get_time() + (int64_t)wait_ms * 1000;
    while (count < sizeof(reply) && esp_timer_get_time() < deadline) {
        const int next = uart_read_bytes(UART_NUM_1, reply + count,
                                         sizeof(reply) - count,
                                         pdMS_TO_TICKS(200));
        if (next > 0) {
            count += next;
            for (int i = 0; i + 6 < count; ++i) {
                if (reply[i] == 0x00 && reply[i + 1] == 0x00 &&
                    reply[i + 2] == 0xFF && reply[i + 5] == 0xD5 &&
                    reply[i + 6] == response_code) {
                    ESP_LOGI(TAG, "%s rx_bytes=%d", name, count);
                    ESP_LOG_BUFFER_HEX(TAG, reply, count);
                    uart_write_bytes(UART_NUM_1, HOST_ACK, sizeof(HOST_ACK));
                    uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(100));
                    ESP_LOGI(TAG, "%s after ACK GPIO10 level=%d", name,
                             gpio_get_level(GPIO_NUM_10));
                    ESP_LOGI(TAG, "%s response frame found", name);
                    return true;
                }
            }
        }
    }
    ESP_LOGI(TAG, "%s rx_bytes=%d", name, count);
    ESP_LOGI(TAG, "%s timeout GPIO10 level=%d", name,
             gpio_get_level(GPIO_NUM_10));
    if (count > 0) {
        ESP_LOG_BUFFER_HEX(TAG, reply, count);
    }
    ESP_LOGW(TAG, "%s response frame absent", name);
    return false;
}

void app_main(void)
{
    // L7 is removed on this sample. Keep the amplifier power path disabled.
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_23, 0));
    vTaskDelay(pdMS_TO_TICKS(1000));
    const uart_config_t config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &config));
    // PN532.27 HSU_RX <- MCU GPIO11; PN532.28 HSU_TX -> MCU GPIO10.
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, GPIO_NUM_11, GPIO_NUM_10,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "HSU-only start, PA_EN=0, SET1:SET0 expected 00");
    ESP_LOGI(TAG, "C106 LowVbat exit: SAM first, version second");
    const bool sam_ok = command("WakeAndSAMConfiguration", WAKE_AND_SAM_CONFIG,
                                sizeof(WAKE_AND_SAM_CONFIG), 0x15, 1500);
    vTaskDelay(pdMS_TO_TICKS(500));
    const bool version_ok = command("GetFirmwareVersion", VERSION,
                                    sizeof(VERSION), 0x03, 1500);
    ESP_LOGI(TAG, "ready: version=%d sam=%d; present Type A tag now",
             version_ok, sam_ok);
    if (sam_ok) {
        for (int round = 0; round < 12; ++round) {
            ESP_LOGI(TAG, "InListPassiveTarget round=%d", round);
            if (command("InListPassiveTarget", IN_LIST_TYPE_A,
                        sizeof(IN_LIST_TYPE_A), 0x4B, 3000)) {
                ESP_LOGI(TAG, "Type A response received; stop polling");
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    ESP_LOGI(TAG, "complete; PA_EN remains low");
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
