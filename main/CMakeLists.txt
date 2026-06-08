idf_component_register(
    SRCS 
        "main.c"
        "dsmr_uart.c"
        "dsmr_parser.c"
        "dsmr_crc16.c"
        "wifi_manager.c"
        "gvc_mqtt_client.c"
        "dsmr_task.c"
        "led_status.c"
        "system_status.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_netif esp_event esp_driver_uart esp_driver_gpio nvs_flash)
