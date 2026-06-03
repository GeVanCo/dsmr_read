#include "dsmr_uart.h"

#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

#define DSMR_UART_NUM      UART_NUM_1
#define DSMR_UART_RX_PIN   4
#define DSMR_UART_TX_PIN   5   // unused but required
#define DSMR_BUF_SIZE      2048

static const char *TAG = "dsmr_uart";
static TaskHandle_t dsmr_task_handle = NULL;

void dsmr_uart_set_task_handle(TaskHandle_t handle)
{
    dsmr_task_handle = handle;
}

void dsmr_uart_init(void)
{
    uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    ESP_ERROR_CHECK(uart_driver_install(DSMR_UART_NUM, DSMR_BUF_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(DSMR_UART_NUM, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(DSMR_UART_NUM, DSMR_UART_TX_PIN, DSMR_UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // DSMR P1 uses inverted RX
    ESP_ERROR_CHECK(uart_set_line_inverse(DSMR_UART_NUM, UART_SIGNAL_RXD_INV));

    ESP_LOGI(TAG, "DSMR UART initialized on GPIO%d (RX), inverted", DSMR_UART_RX_PIN);
}

char *dsmr_uart_read_telegram(void)
{
    uint8_t buf[DSMR_BUF_SIZE];
    int len = uart_read_bytes(DSMR_UART_NUM, buf, sizeof(buf) - 1, pdMS_TO_TICKS(2000));

    if (len <= 0) {
        return NULL; // no data
    }

    buf[len] = '\0';

    // DSMR telegrams start with '/' and end with '!'
    char *start = strchr((char *)buf, '/');
    char *end   = strrchr((char *)buf, '!');

    if (!start || !end || end <= start) {
        ESP_LOGW(TAG, "Invalid DSMR telegram received");
        return NULL;
    }

    // include '!' and newline
    end += 1;

    size_t telegram_len = end - start;
    char *telegram = malloc(telegram_len + 1);
    if (!telegram) return NULL;

    memcpy(telegram, start, telegram_len);
    telegram[telegram_len] = '\0';

    ESP_LOGI(TAG, "Received DSMR telegram (%d bytes)", telegram_len);
    return telegram;
}

