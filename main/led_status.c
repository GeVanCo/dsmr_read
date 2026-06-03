#include "led_status.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "led_status";

#define LED_GPIO 8   // adjust if needed

static led_status_t current_status = LED_STATUS_HEARTBEAT;

static void led_set(int level)
{
    gpio_set_level(LED_GPIO, level);
}

static void pattern_heartbeat(void)
{
    led_set(1);
    vTaskDelay(pdMS_TO_TICKS(200));
    led_set(0);
    vTaskDelay(pdMS_TO_TICKS(800));
}

static void pattern_wifi_disconnected(void)
{
    for (int i = 0; i < 2; i++) {
        led_set(1);
        vTaskDelay(pdMS_TO_TICKS(150));
        led_set(0);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
    vTaskDelay(pdMS_TO_TICKS(700));
}

static void pattern_mqtt_disconnected(void)
{
    led_set(1);
    vTaskDelay(pdMS_TO_TICKS(100));
    led_set(0);
    vTaskDelay(pdMS_TO_TICKS(100));
}

static void pattern_dsmr_error(void)
{
    led_set(1);
    vTaskDelay(pdMS_TO_TICKS(500));
    led_set(0);
    vTaskDelay(pdMS_TO_TICKS(200));
    led_set(1);
    vTaskDelay(pdMS_TO_TICKS(150));
    led_set(0);
    vTaskDelay(pdMS_TO_TICKS(500));
}

static void pattern_all_ok(void)
{
    led_set(1);
    vTaskDelay(pdMS_TO_TICKS(1000));
}

static void led_task(void *arg)
{
    ESP_LOGI(TAG, "LED status task started");

    while (1) {
        switch (current_status) {
            case LED_STATUS_HEARTBEAT:
                pattern_heartbeat();
                break;

            case LED_STATUS_WIFI_DISCONNECTED:
                pattern_wifi_disconnected();
                break;

            case LED_STATUS_MQTT_DISCONNECTED:
                pattern_mqtt_disconnected();
                break;

            case LED_STATUS_DSMR_ERROR:
                pattern_dsmr_error();
                break;

            case LED_STATUS_ALL_OK:
                pattern_all_ok();
                break;
        }
    }
}

void led_status_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&cfg);
    led_set(0);

    ESP_LOGI(TAG, "LED initialized on GPIO%d", LED_GPIO);
}

void led_status_set(led_status_t status)
{
    if (status != current_status)
    {
        ESP_LOGI(TAG, "LED status changed: was [%d] → is [%d]", current_status, status);
        // taskENTER_CRITICAL();
        current_status = status;
        // taskEXIT_CRITICAL();
    }
}

void led_status_start(void)
{
    xTaskCreate(led_task,
                "led_task",
                2048,
                NULL,
                1,
                NULL);
}
