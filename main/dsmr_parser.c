#include "dsmr_parser.h"

#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "gvc_dsmr_parser";

MeterFeatures detect_obis_features(const char *telegram)
{
    MeterFeatures f = {0};

    if (strstr(telegram, "52.7.0") || strstr(telegram, "72.7.0"))
        f.isThreePhase = true;

    if (strstr(telegram, "0-1:24.2.1"))
        f.hasGas = true;

    if (strstr(telegram, "0-2:24.2.3"))
        f.hasWater = true;

    if (strstr(telegram, "0-3:24.2.4"))
        f.hasHeat = true;

    if (strstr(telegram, "1-0:2.7.0"))
        f.hasSolar = true;

    return f;
}

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

    data.features = detect_obis_features(telegram);

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
        
        // 1-0:32.7.0 = voltage L1
        else if (strstr(line, "1-0:32.7.0")) {
            data.voltage_l1 = extract_float(line);
        }

        // 1-0:52.7.0 = voltage L2
        else if (strstr(line, "1-0:52.7.0")) {
            data.voltage_l2 = extract_float(line);
        }

        // 1-0:72.7.0 = voltage L3
        else if (strstr(line, "1-0:72.7.0")) {
            data.voltage_l3 = extract_float(line);
        }

        // 1-0:31.7.0 = current L1
        else if (strstr(line, "1-0:31.7.0")) {
            data.current_l1 = extract_float(line);
        }

        // 1-0:51.7.0 = current L2
        else if (strstr(line, "1-0:51.7.0")) {
            data.current_l2 = extract_float(line);
        }

        // 1-0:71.7.0 = current L3
        else if (strstr(line, "1-0:71.7.0")) {
            data.current_l3 = extract_float(line);
        }

        line = strtok(NULL, "\r\n");
    }

    free(copy);
    ESP_LOGI(TAG, "Parsed DSMR: import=%.3f, export=%.3f, U=%.1fV, I=%.1fA",
             data.power_import, data.power_export,
             data.voltage_l1, data.current_l1);

    return data;
}
