#include "dsmr_task.h"

#include "dsmr_uart.h"
#include "dsmr_parser.h"
#include "gvc_mqtt_client.h"
#include "json_p1.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "#_task";

#define DSMR_TASK_STACK 4096
#define DSMR_TASK_PRIO  5

static void dsmr_task(void *arg)
{
    ESP_LOGI(TAG, "DSMR task started");

    QueueHandle_t q = dsmr_uart_get_queue();
    if (!q)
    {
        ESP_LOGE(TAG, "DSMR telegram queue not initialized");
        vTaskDelete(NULL);
    }

    while (1)
    {
        char *telegram = NULL;

        if (xQueueReceive(q, &telegram, portMAX_DELAY) == pdTRUE)
        {
            if (telegram)
            {
                // Reset JSON state per telegram
                json_p1_reset();

                // Parse telegram → fills OBIS table via json_p1_set_obis()
                (void)dsmr_parse(telegram);

                // Build JSON
                bool is_full = false;
                char *json = json_p1_build(&is_full);

                if (json)
                {
                    mqtt_publish_dsmr(json);
                    free(json);
                }

                free(telegram);
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
