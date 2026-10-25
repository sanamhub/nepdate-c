/* Reads and checks shared/data/calendar/bs-calendar.json and renders
 * src/generated/calendar_tables.h (ADR-0001). */
#ifndef NEPDATE_CODEGEN_CALENDAR_H
#define NEPDATE_CODEGEN_CALENDAR_H

#include <stdbool.h>
#include <stddef.h>

enum {
    CODEGEN_MIN_YEAR = 1901,
    CODEGEN_MAX_YEAR = 2199,
    CODEGEN_YEARS = CODEGEN_MAX_YEAR - CODEGEN_MIN_YEAR + 1,
    CODEGEN_MONTHS = 12
};

typedef struct {
    int min_year;
    int max_year;
    unsigned char month_lengths[CODEGEN_YEARS][CODEGEN_MONTHS];
} codegen_calendar;

/* Parses the JSON text of bs-calendar.json into out. Returns false with a message naming the year
 * and month at fault in error (always NUL-terminated when error_size is not 0). */
bool codegen_parse_calendar(const char *json_text, size_t length, codegen_calendar *out,
                            char *error, size_t error_size);

/* Renders the header text. Returns a malloc'd string, or NULL when out of memory. */
char *codegen_render_calendar_tables(const codegen_calendar *calendar);

#endif /* NEPDATE_CODEGEN_CALENDAR_H */
