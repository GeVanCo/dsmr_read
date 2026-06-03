#include "dsmr_task.h"

#include "dsmr_uart.h"
#include "dsmr_parser.h"
#include "gvc_mqtt_client.h"
#include "led_status.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "dsmr_task";

#define DSMR_TASK_STACK   4096
#define DSMR_TASK_PRIO    5
#define DSMR_INTERVAL_SEC 10

static void dsmr_task(void *arg)
{
    ESP_LOGI(TAG, "DSMR task started");

    while (1) 
    {

        char *telegram = dsmr_uart_read_telegram();

        if (telegram) 
        {
            ESP_LOGI(TAG, "Received DSMR telegram");
            led_status_set(LED_STATUS_ALL_OK);

            dsmr_data_t data = dsmr_parse(telegram);

            mqtt_publish_dsmr(&data);

            free(telegram);
        } 
        else 
        {
            ESP_LOGW(TAG, "No valid DSMR telegram received");
            led_status_set(LED_STATUS_DSMR_ERROR);
        }

        vTaskDelay(pdMS_TO_TICKS(DSMR_INTERVAL_SEC * 1000));
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
