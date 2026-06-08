#include "dsmr_uart.h"
#include "dsmr_parser.h"
#include "wifi_manager.h"
#include "gvc_mqtt_client.h"
#include "dsmr_task.h"
#include "led_status.h"
#include "system_status.h"
#include "esp_log.h"

static const char *TAG = "gvc_app";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting DSMR Reader...");

    // 1. LED subsystem first
    led_status_init();
    led_status_start();

    // 2. System status (will call led_status_set safely)
    system_status_init();

    // 3. Wi-Fi
    wifi_manager_init();

    // 4. MQTT
    mqtt_client_init();

    // 5. DSMR UART
    dsmr_uart_init();

    // 6. DSMR Task
    dsmr_task_start();
}
