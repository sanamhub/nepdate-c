/* Reads and checks the JSON files under shared/data/patro and builds the dense per-day tables of
 * PATRO.md section 3 (the layout the C++ port uses). The generator never includes or links the
 * library. */
#ifndef NEPDATE_CODEGEN_PATRO_H
#define NEPDATE_CODEGEN_PATRO_H

#include "calendar.h"
#include "json.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    CODEGEN_PATRO_FIRST_YEAR = 2077,
    CODEGEN_PATRO_LAST_YEAR = 2084,
    CODEGEN_PATRO_FIRST_SERIAL = 64285,
    CODEGEN_PATRO_DAYS = 2793,
    CODEGEN_PATRO_TITHI_DAYS = 2190,
    CODEGEN_PATRO_TITHI_NAMES = 32,
    CODEGEN_PATRO_HOLIDAYS = 294,
    CODEGEN_PATRO_EVENT_DAYS = 1671,
    CODEGEN_PATRO_EVENTS = 3032,
    CODEGEN_PATRO_NAMES = 753,
    CODEGEN_PATRO_NAME_BYTES = 59800
};

typedef struct {
    char *np;
    char *en;
} codegen_patro_name;

typedef struct {
    int year;
    int month;
    int day;
    bool has_tithi;
    codegen_patro_name tithi;
    bool holiday;
    codegen_patro_name *events;
    size_t event_count;
    size_t event_capacity;
} codegen_patro_day;

typedef struct {
    codegen_patro_day *days;
    size_t count;
    size_t capacity;
} codegen_patro_input;

typedef struct {
    uint32_t first;
    uint32_t days;
    uint8_t *day_tithi;
    uint64_t *holiday_bits;
    size_t holiday_bit_count;
    uint16_t *day_event_start;
    uint16_t *events;
    size_t event_count;
    uint32_t *holiday_serials;
    size_t holiday_count;
    codegen_patro_name *names;
    size_t name_count;
    char *name_blob;
    size_t name_blob_size;
    uint32_t *name_offsets;
    size_t name_offset_count;
} codegen_patro_tables;

/* Appends the days of one year file to input. Returns false with a message in error (always
 * NUL-terminated when error_size is not 0) when the file is malformed. */
bool codegen_patro_add_year(const json_value *doc, int expected_year, codegen_patro_input *input,
                            char *error, size_t error_size);

/* Builds the tables of PATRO.md section 3 from input and the calendar. Returns false with a
 * message in error when the data is not the exact coverage, has a tithi gap, or is not T = 2190,
 * H = 294 and E = 1671. */
bool codegen_build_patro_tables(codegen_patro_input *input, const codegen_calendar *calendar,
                                codegen_patro_tables *tables, char *error, size_t error_size);

/* Renders src/generated/patro_tables.h. Returns a malloc'd string, or NULL when out of memory. */
char *codegen_render_patro_tables(const codegen_patro_tables *tables,
                                  const codegen_calendar *calendar);

void codegen_patro_input_free(codegen_patro_input *input);
void codegen_patro_tables_free(codegen_patro_tables *tables);

#endif /* NEPDATE_CODEGEN_PATRO_H */
