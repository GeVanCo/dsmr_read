#include "led_status.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "gvc_led_status";

#define LED_GPIO 8   // adjust if needed

static led_status_t current_led_status = LED_STATUS_HEARTBEAT;
static SemaphoreHandle_t led_mutex;
static bool led_ready = false;

static void led_set(int level)
{
    gpio_set_level(LED_GPIO, level);
}

static void pattern_heartbeat(void)
{
    led_set(1);
    vTaskDelay(pdMS_TO_TICKS(80));
    led_set(0);
    vTaskDelay(pdMS_TO_TICKS(920));
}

static void pattern_wifi_disconnected(void)
{
    for (int i = 0; i < 2; i++) {
        led_set(1);
        vTaskDelay(pdMS_TO_TICKS(200));
        led_set(0);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    vTaskDelay(pdMS_TO_TICKS(700));
}

static void pattern_wifi_retrying(void)
{
    led_set(1);
    vTaskDelay(pdMS_TO_TICKS(150));
    led_set(0);
    vTaskDelay(pdMS_TO_TICKS(150));
}

static void pattern_mqtt_disconnected(void)
{
    led_set(1);
    vTaskDelay(pdMS_TO_TICKS(50));
    led_set(0);
    vTaskDelay(pdMS_TO_TICKS(50));
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
    led_set(0);
    vTaskDelay(pdMS_TO_TICKS(1000));
}

static void led_task(void *arg)
{
    ESP_LOGI(TAG, "LED status task started");

    while (1) 
    {
        led_status_t status;
        xSemaphoreTake(led_mutex, portMAX_DELAY);
        status = current_led_status;
        xSemaphoreGive(led_mutex);

        switch (status) {
            case LED_STATUS_HEARTBEAT:
                pattern_heartbeat();
                break;

            case LED_STATUS_WIFI_RETRYING:
                pattern_wifi_retrying();
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

        taskYIELD(); // allow other tasks to run
    }
}

void led_status_init(void)
{
    led_mutex = xSemaphoreCreateMutex();

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&cfg);
    led_set(0);

    led_ready = true;

    ESP_LOGI(TAG, "LED initialized on GPIO%d", LED_GPIO);
}

void led_status_set(led_status_t status)
{
    if (!led_ready) {
        ESP_LOGW(TAG, "LED not ready, ignoring status set");
        return;
    }
    
    if (status != current_led_status)
    {
        ESP_LOGI(TAG, "LED status changed: was [%d] → is [%d]", current_led_status, status);

        xSemaphoreTake(led_mutex, portMAX_DELAY);
        current_led_status = status;
        xSemaphoreGive(led_mutex);
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
