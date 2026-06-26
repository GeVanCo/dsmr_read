#pragma once

#include "dsmr_parser.h"

void mqtt_client_init(void);
void mqtt_publish_dsmr(const char* json);
