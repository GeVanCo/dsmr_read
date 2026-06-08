#include "dsmr_uart.h"

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "system_status.h"
#include "dsmr_crc16.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h> // isxdigit()

static const char *TAG = "gvc_dsmr_uart";

// UART1 on GPIO4 (RX), TX unused
#define DSMR_UART_NUM      UART_NUM_1
#define DSMR_UART_TX_GPIO  UART_PIN_NO_CHANGE
#define DSMR_UART_RX_GPIO  4

#define DSMR_BAUD_RATE     115200

#define DSMR_RX_BUF_SIZE   1024
#define DSMR_TLG_BUF_SIZE  2048
#define DSMR_QUEUE_LEN     4

static QueueHandle_t s_uart_event_queue = NULL;
static QueueHandle_t s_telegram_queue   = NULL;

static char   s_tlg_buf[DSMR_TLG_BUF_SIZE];
static size_t s_tlg_len = 0;

QueueHandle_t dsmr_uart_get_queue(void)
{
    return s_telegram_queue;
}

static void handle_full_telegram(const char *buf, size_t len)
{
    // Find '!' again (safe because telegram is complete)
    char *excl = strrchr(buf, '!');
    if (!excl) {
        ESP_LOGW(TAG, "Telegram missing '!'");
        system_status_set(SYSTEM_STATUS_DSMR, false);
        return;
    }

    size_t pos = excl - buf;

    // Extract transmitted CRC (4 hex chars)
    if (pos + 4 >= len) {
        ESP_LOGW(TAG, "Telegram too short for CRC");
        system_status_set(SYSTEM_STATUS_DSMR, false);
        return;
    }

    char crc_str[5];
    memcpy(crc_str, &buf[pos + 1], 4);
    crc_str[4] = '\0';

    uint16_t crc_rx = (uint16_t)strtol(crc_str, NULL, 16);

    // pos = index of '!'
    uint16_t crc_calc = dsmr_crc16((const uint8_t *)buf, pos + 1); // include '!' in CRC

    if (crc_rx != crc_calc) {
        ESP_LOGW(TAG, "CRC mismatch: rx=%04X calc=%04X", crc_rx, crc_calc);
        system_status_set(SYSTEM_STATUS_DSMR, false);
        return;
    }

    // CRC OK → restore normal LED state
    ESP_LOGI(TAG, "SYSTEM_STATUS_DSMR_OK");
    system_status_set(SYSTEM_STATUS_DSMR, true);

    // Allocate and queue telegram    
    char *telegram = malloc(len + 1);
    if (!telegram) {
        ESP_LOGE(TAG, "Failed to allocate memory for telegram");
        return;
    }

    memcpy(telegram, buf, len);
    telegram[len] = '\0';

    if (xQueueSend(s_telegram_queue, &telegram, 0) != pdPASS) {
        ESP_LOGW(TAG, "Telegram queue full, dropping telegram");
        free(telegram);
    // } else {
    //     ESP_LOGI(TAG, "Queued DSMR telegram (%d bytes)", (int)len);
    }
}

static void process_incoming_bytes(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        char c = (char)data[i];

        // Append to buffer
        if (s_tlg_len < DSMR_TLG_BUF_SIZE - 1) {
            s_tlg_buf[s_tlg_len++] = c;
        } else {
            ESP_LOGW(TAG, "Telegram buffer overflow, resetting");
            s_tlg_len = 0;
            continue;
        }

        // Detect end of telegram on newline
        if (c == '\n') {

            // Find last '!' in buffer
            char *excl = strrchr(s_tlg_buf, '!');
            if (excl) {
                size_t pos = excl - s_tlg_buf;  // index of '!'

                // Ensure 4 chars after '!'
                if (pos + 4 < s_tlg_len) {

                    // Validate 4 hex chars
                    bool hex_ok = true;
                    for (int j = 1; j <= 4; j++) {
                        // char h = s_tlg_buf[pos + j];
                        // if (!((h >= '0' && h <= '9') ||
                        //       (h >= 'A' && h <= 'F'))) {
                        //     hex_ok = false;
                        //     break;
                        // }
                        if (!isxdigit((unsigned char)s_tlg_buf[pos + j])) {
                            hex_ok = false;
                            break;
                        }
                    }

                    if (hex_ok) {
                        handle_full_telegram(s_tlg_buf, s_tlg_len);
                        s_tlg_len = 0;
                        continue;
                    }
                }
            }
        }
    }
}

static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t data[256];

    ESP_LOGI(TAG, "UART event task started (UART%d)", DSMR_UART_NUM);

    while (1) {
        if (xQueueReceive(s_uart_event_queue, &event, portMAX_DELAY)) {
            switch (event.type) {

                case UART_DATA: {
                    int len = uart_read_bytes(DSMR_UART_NUM,
                                              data,
                                              event.size < sizeof(data) ? event.size : sizeof(data),
                                              0);
                    if (len > 0) {
                        process_incoming_bytes(data, len);
                    }
                    break;
                }

                case UART_FIFO_OVF:
                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "UART overflow, flushing");
                    uart_flush_input(DSMR_UART_NUM);
                    xQueueReset(s_uart_event_queue);
                    s_tlg_len = 0;
                    break;

                default:
                    break;
            }
        }
    }
}

void dsmr_uart_init(void)
{
    uart_config_t cfg = {
        .baud_rate = DSMR_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    ESP_ERROR_CHECK(uart_param_config(DSMR_UART_NUM, &cfg));

    ESP_ERROR_CHECK(uart_set_pin(DSMR_UART_NUM,
                                 DSMR_UART_TX_GPIO,
                                 DSMR_UART_RX_GPIO,
                                 UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE));

    ESP_ERROR_CHECK(uart_driver_install(DSMR_UART_NUM,
                                        DSMR_RX_BUF_SIZE,
                                        DSMR_RX_BUF_SIZE,
                                        20,
                                        &s_uart_event_queue,
                                        0));

    s_telegram_queue = xQueueCreate(DSMR_QUEUE_LEN, sizeof(char *));
    if (!s_telegram_queue) {
        ESP_LOGE(TAG, "Failed to create telegram queue");
    }

    xTaskCreate(uart_event_task,
                "uart_event_task",
                4096,
                NULL,
                6,
                NULL);

    s_tlg_len = 0;

    ESP_LOGI(TAG, "DSMR UART initialized on UART%d (RX GPIO%d)",
             DSMR_UART_NUM, DSMR_UART_RX_GPIO);
}
