/* Reads and checks shared/data/calendar/bs-calendar.json, derives the tables of ALGORITHM section 2
 * and renders src/generated/calendar_tables.h (ADR-0001, ADR-0006). */
#ifndef NEPDATE_CODEGEN_CALENDAR_H
#define NEPDATE_CODEGEN_CALENDAR_H

#include <stdbool.h>
#include <stddef.h>

enum {
    CODEGEN_MIN_YEAR = 1901,
    CODEGEN_MAX_YEAR = 2199,
    CODEGEN_YEARS = CODEGEN_MAX_YEAR - CODEGEN_MIN_YEAR + 1,
    CODEGEN_MONTHS = 12,
    CODEGEN_ALL_MONTHS = CODEGEN_YEARS * CODEGEN_MONTHS,
    /* AD 1844-04 to 2143-04, the AD months that hold a supported day. */
    CODEGEN_AD_FIRST_YEAR = 1844,
    CODEGEN_AD_FIRST_MONTH = 4,
    CODEGEN_AD_MONTHS = 3589,
    /* The month index of day count s is (s * estimate_mul) >> CODEGEN_ESTIMATE_SHIFT, or one
     * more or one less. */
    CODEGEN_ESTIMATE_SHIFT = 22
};

typedef struct {
    int min_year;
    int max_year;
    unsigned char month_lengths[CODEGEN_YEARS][CODEGEN_MONTHS];
    /* The rest is derived by codegen_derive_tables. month_start[i] is the day count (days since
     * BS 1901-01-01) of the first day of month index i = (year - 1901) * 12 + month - 1; the last
     * entry is the number of days in the calendar. */
    long month_start[CODEGEN_ALL_MONTHS + 1];
    long year_start[CODEGEN_YEARS + 1];
    /* Day count of the first day of AD month j = (year - 1844) * 12 + month - 4. */
    long ad_month_start[CODEGEN_AD_MONTHS];
    /* Unix day of the epoch, AD 1844-04-11. */
    long epoch_unix_days;
    unsigned long estimate_mul;
} codegen_calendar;

/* Parses the JSON text of bs-calendar.json into out and derives its tables. Returns false with a
 * message naming the year and month at fault in error (always NUL-terminated when error_size is
 * not 0). */
bool codegen_parse_calendar(const char *json_text, size_t length, codegen_calendar *out,
                            char *error, size_t error_size);

/* Derives the tables of calendar from its month lengths. Returns false with a message in error
 * when the month estimate is off by more than one month for some day. */
bool codegen_derive_tables(codegen_calendar *calendar, char *error, size_t error_size);

/* Days since 1970-01-01 of a proleptic Gregorian date (ALGORITHM section 5). */
long codegen_days_from_civil(long year, long month, long day);

/* Renders the header text. Returns a malloc'd string, or NULL when out of memory. */
char *codegen_render_calendar_tables(const codegen_calendar *calendar);

#endif /* NEPDATE_CODEGEN_CALENDAR_H */
