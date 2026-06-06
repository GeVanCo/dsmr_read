#include "dsmr_uart.h"
#include "dsmr_parser.h"
#include "wifi_manager.h"
#include "gvc_mqtt_client.h"
#include "dsmr_task.h"
#include "led_status.h"
#include "esp_log.h"

static const char *TAG = "app";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting DSMR Reader...");

    led_status_init();
    led_status_start();

    // wifi_manager_init();
    // mqtt_client_init();
    dsmr_uart_init();

    dsmr_task_start();

    // while (true) {
    //     char *telegram = dsmr_uart_read_telegram();
    //     if (telegram) {
    //         dsmr_data_t data = dsmr_parse(telegram);
    //         mqtt_publish_dsmr(&data);
    //         free(telegram);
    //     }
    // }
}
