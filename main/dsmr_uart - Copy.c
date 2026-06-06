#include "dsmr_uart.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "dsmr_uart";

// Select which UART to use:
// - UART_NUM_0: simulator via USB-CDC (no GPIO pins)
// - UART_NUM_1: real P1 port via GPIO
#define DSMR_UART_NUM      UART_NUM_1

// Only used when DSMR_UART_NUM != UART_NUM_0
#define DSMR_UART_TX_GPIO  UART_PIN_NO_CHANGE
#define DSMR_UART_RX_GPIO  4      // your P1 RX pin when using UART1

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
    ESP_LOGI(TAG, "handle_full_telegram: len=%d", (int)len);

    char *telegram = malloc(len + 1);
    if (!telegram) {
        ESP_LOGE(TAG, "Failed to allocate memory for telegram");
        return;
    }
    ESP_LOGI(TAG, "TELEGRAM CONTENT:\n%s", telegram);

    memcpy(telegram, buf, len);
    telegram[len] = '\0';

    if (s_telegram_queue) {
        if (xQueueSend(s_telegram_queue, &telegram, 0) != pdPASS) {
            ESP_LOGW(TAG, "Telegram queue full, dropping telegram");
            free(telegram);
        } else {
            ESP_LOGI(TAG, "Queued DSMR telegram (%d bytes)", (int)len);
        }
    } else {
        free(telegram);
    }
}

static void process_incoming_bytes(const uint8_t *data, size_t len)
{
    ESP_LOGI(TAG, "process_incoming_bytes: len=%d", (int)len);

    for (size_t i = 0; i < len; i++) {
        ESP_LOGI(TAG, "  byte[%d] = 0x%02X ('%c')",
                (int)i,
                data[i],
                (data[i] >= 32 && data[i] <= 126) ? data[i] : '.');

        char c = (char)data[i];
        if (c == '\n') {
            ESP_LOGI(TAG, "  NEWLINE detected, s_tlg_len=%d", (int)s_tlg_len);
        }  
        if (c == '!') {
            ESP_LOGI(TAG, "  EXCLAMATION MARK detected at pos %d", (int)s_tlg_len);
        }      

        if (s_tlg_len < DSMR_TLG_BUF_SIZE - 1) {
            s_tlg_buf[s_tlg_len++] = c;
        } else {
            ESP_LOGW(TAG, "Telegram buffer overflow, resetting");
            s_tlg_len = 0;
            continue;
        }

        if (c == '\n' && s_tlg_len > 0) {
            // Handle both "!\n" and "!\r\n"
            size_t pos = s_tlg_len;

            // Skip trailing '\r' if present
            if (pos >= 2 && s_tlg_buf[pos - 2] == '\r') {
                pos--;
            }

            if (pos >= 2 && s_tlg_buf[pos - 2] == '!') {
                handle_full_telegram(s_tlg_buf, s_tlg_len);
                s_tlg_len = 0;
            }
        }
    }
}

// static void process_incoming_bytes(const uint8_t *data, size_t len)
// {
//     for (size_t i = 0; i < len; i++) {
//         char c = (char)data[i];

//         if (s_tlg_len < DSMR_TLG_BUF_SIZE - 1) {
//             s_tlg_buf[s_tlg_len++] = c;
//         } else {
//             ESP_LOGW(TAG, "Telegram buffer overflow, resetting");
//             s_tlg_len = 0;
//             continue;
//         }

//         if (c == '\n' && s_tlg_len > 0) {
//             if (s_tlg_len >= 2 && s_tlg_buf[s_tlg_len - 2] == '!') {
//                 handle_full_telegram(s_tlg_buf, s_tlg_len);
//                 s_tlg_len = 0;
//             }
//         }
//     }
// }

static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t data[256];

    ESP_LOGI(TAG, "UART event task started (UART%d)", DSMR_UART_NUM);

    while (1) {
        if (xQueueReceive(s_uart_event_queue, &event, portMAX_DELAY)) {
            switch (event.type) {

                case UART_DATA: {
                    ESP_LOGE(TAG, "------------------------------------------------------");
                    int len = uart_read_bytes(DSMR_UART_NUM,
                                              data,
                                              event.size < sizeof(data) ? event.size : sizeof(data),
                                              0);
                    if (len > 0) {
                        // 🔍 DEBUG: print raw bytes (HEX)
                        printf("UART_DATA (%d bytes): HEX: ", len);
                        for (int i = 0; i < len; i++) {
                            printf("%02X ", data[i]);
                        }
                        printf("\n");

                        // 🔍 DEBUG: print raw bytes (ASCII best effort)
                        printf("UART_DATA ASCII: ");
                        for (int i = 0; i < len; i++) {
                            char c = (data[i] >= 32 && data[i] <= 126) ? data[i] : '.';
                            printf("%c", c);
                        }
                        printf("\n");

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

    ESP_LOGI(TAG, "DSMR UART initialized on UART%d", DSMR_UART_NUM);
}
