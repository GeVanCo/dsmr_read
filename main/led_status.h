#pragma once

typedef enum {
    LED_STATUS_HEARTBEAT,
    LED_STATUS_WIFI_RETRYING,
    LED_STATUS_WIFI_DISCONNECTED,
    LED_STATUS_MQTT_DISCONNECTED,
    LED_STATUS_DSMR_ERROR,
    LED_STATUS_ALL_OK
} led_status_t;

void led_status_init(void);
void led_status_set(led_status_t status);
void led_status_start(void);
