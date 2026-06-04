#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void dsmr_uart_init(void);
QueueHandle_t dsmr_uart_get_queue(void);
