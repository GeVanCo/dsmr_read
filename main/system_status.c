#include "system_status.h"
#include "led_status.h"

static system_status_t sys_status = {
    .wifi_ok = false,
    .mqtt_ok = false,
    .dsmr_ok = false,
    .last_bad_item = SYSTEM_STATUS_WIFI
};

// LED update logic: "last bad state wins"
static void system_status_update_led(void)
{
    bool any_bad = (
           !sys_status.wifi_ok 
        || !sys_status.mqtt_ok 
        || !sys_status.dsmr_ok
    );

    if (any_bad) {
        switch (sys_status.last_bad_item) {

            case SYSTEM_STATUS_WIFI:
                led_status_set(LED_STATUS_WIFI_DISCONNECTED);
                return;

            case SYSTEM_STATUS_MQTT:
                led_status_set(LED_STATUS_MQTT_DISCONNECTED);
                return;

            case SYSTEM_STATUS_DSMR:
                led_status_set(LED_STATUS_DSMR_ERROR);
                return;
        }
    }

    // All subsystems OK
    led_status_set(LED_STATUS_ALL_OK);
}

void system_status_init(void)
{
    sys_status.wifi_ok = false;
    sys_status.mqtt_ok = false;
    sys_status.dsmr_ok = false;
    sys_status.last_bad_item = SYSTEM_STATUS_WIFI;

    system_status_update_led();
}

void system_status_set(system_status_item_t item, bool ok)
{
    // Update the subsystem state
    switch (item) {

        case SYSTEM_STATUS_WIFI:
            sys_status.wifi_ok = ok;
            break;

        case SYSTEM_STATUS_MQTT:
            sys_status.mqtt_ok = ok;
            break;

        case SYSTEM_STATUS_DSMR:
            sys_status.dsmr_ok = ok;
            break;
    }

    // If this subsystem just became BAD → remember it
    if (!ok) {
        sys_status.last_bad_item = item;
    }

    system_status_update_led();
}

system_status_t system_status_get(void)
{
    return sys_status;
}
