#include "dsmr_task.h"

#include "dsmr_uart.h"
#include "dsmr_parser.h"
#include "gvc_mqtt_client.h"
#include "led_status.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "dsmr_task";

#define DSMR_TASK_STACK 4096
#define DSMR_TASK_PRIO  5

static void dsmr_task(void *arg)
{
    ESP_LOGI(TAG, "DSMR task started");

    QueueHandle_t q = dsmr_uart_get_queue();
    if (!q) {
        ESP_LOGE(TAG, "DSMR telegram queue not initialized");
        vTaskDelete(NULL);
    }

    while (1) {
        char *telegram = NULL;

        if (xQueueReceive(q, &telegram, portMAX_DELAY) == pdTRUE) {
            if (telegram) {
                dsmr_data_t data = dsmr_parse(telegram);
                mqtt_publish_dsmr(&data);
                free(telegram);
                led_status_set(LED_STATUS_ALL_OK);
            } else {
                led_status_set(LED_STATUS_DSMR_ERROR);
            }
        }
    }
}

void dsmr_task_start(void)
{
    xTaskCreate(dsmr_task,
                "dsmr_task",
                DSMR_TASK_STACK,
                NULL,
                DSMR_TASK_PRIO,
                NULL);
}

