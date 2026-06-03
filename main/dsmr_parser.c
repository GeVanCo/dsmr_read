#include "dsmr_parser.h"

#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "dsmr_parser";

static float extract_float(const char *line)
{
    const char *start = strchr(line, '(');
    const char *end   = strchr(line, '*');   // DSMR uses *kWh, *V, *A, etc.

    if (!start || !end || end <= start)
        return 0.0f;

    char buf[32];
    size_t len = end - (start + 1);
    if (len >= sizeof(buf))
        len = sizeof(buf) - 1;

    memcpy(buf, start + 1, len);
    buf[len] = '\0';

    return atof(buf);
}

dsmr_data_t dsmr_parse(const char *telegram)
{
    dsmr_data_t data = {0};

    char *copy = strdup(telegram);
    if (!copy)
        return data;

    char *line = strtok(copy, "\r\n");
    while (line) {

        // 1-0:1.8.0 = total import kWh
        if (strstr(line, "1-0:1.8.0")) {
            data.power_import = extract_float(line);
        }

        // 1-0:2.8.0 = total export kWh
        else if (strstr(line, "1-0:2.8.0")) {
            data.power_export = extract_float(line);
        }

        // 1-0:32.7.0 = voltage L1
        else if (strstr(line, "1-0:32.7.0")) {
            data.voltage_l1 = extract_float(line);
        }

        // 1-0:31.7.0 = current L1
        else if (strstr(line, "1-0:31.7.0")) {
            data.current_l1 = extract_float(line);
        }

        line = strtok(NULL, "\r\n");
    }

    free(copy);
    ESP_LOGI(TAG, "Parsed DSMR: import=%.3f, export=%.3f, U=%.1fV, I=%.1fA",
             data.power_import, data.power_export,
             data.voltage_l1, data.current_l1);

    return data;
}
