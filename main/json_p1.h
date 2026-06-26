#ifndef JSON_P1_H
#define JSON_P1_H

#include <stdbool.h>

//
// Maximum number of OBIS entries we track.
// You can increase this if needed.
//
#define OBIS_MAX_COUNT 64

//
// Peak history entries (optional)
//
#define PEAK_HISTORY_MAX 13

//
// Store a single OBIS entry
//
typedef struct
{
    char obis[16];
    char value[32];
}
json_obis_entry_t;

//
// Peak history entry
//
typedef struct
{
    char timestamp[16];
    char value[16];
}
json_peak_entry_t;

//
// Public API
//
void json_p1_reset(void);
void json_p1_set_obis(const char *obis, const char *value);
void json_p1_add_peak_event(const char *timestamp, const char *value);

char *json_p1_build(bool *is_full);

#endif // JSON_P1_H
