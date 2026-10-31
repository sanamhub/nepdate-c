/* Reads and checks the JSON files under shared/data/patro against the counts in
 * shared/spec/vectors/patro-summary.json and builds the dense per-day tables of PATRO.md section 3
 * (the layout the C++ port uses). The generator never includes or links the library. */
#ifndef NEPDATE_CODEGEN_PATRO_H
#define NEPDATE_CODEGEN_PATRO_H

#include "calendar.h"
#include "json.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The counts every port reads from spec/vectors/patro-summary.json, which the hub writes from
 * data/patro/ (tools/make_vectors.py). The generator validates the data against it and writes the
 * values into the generated header; nothing here is a count of the day. */
typedef struct {
    int first_year;
    int last_year;
    char first_day[11];
    char last_day[11];
    uint32_t first_serial;
    uint32_t last_serial;
    uint32_t days;
    char tithi_first_day[11];
    char tithi_last_day[11];
    uint32_t tithi_first_serial;
    uint32_t tithi_days;
    uint32_t tithi_names;
    uint32_t holidays;
    uint32_t event_days;
    uint32_t events;
    uint32_t event_names;
    uint32_t names;
    uint32_t name_bytes;
} codegen_patro_summary;

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

/* Parses spec/vectors/patro-summary.json into out. Returns false with a message in error (always
 * NUL-terminated when error_size is not 0) when a key is missing or has the wrong shape. */
bool codegen_patro_summary_parse(const json_value *doc, codegen_patro_summary *out, char *error,
                                 size_t error_size);

/* Appends the days of one year file to input. Returns false with a message in error (always
 * NUL-terminated when error_size is not 0) when the file is malformed. */
bool codegen_patro_add_year(const json_value *doc, int expected_year, codegen_patro_input *input,
                            char *error, size_t error_size);

/* Builds the tables of PATRO.md section 3 from input and the calendar, checking every count
 * against summary. Returns false with a message in error when the data does not match the
 * summary, has a tithi gap, or is malformed. */
bool codegen_build_patro_tables(codegen_patro_input *input, const codegen_calendar *calendar,
                                const codegen_patro_summary *summary, codegen_patro_tables *tables,
                                char *error, size_t error_size);

/* Renders src/generated/patro_tables.h. Returns a malloc'd string, or NULL when out of memory. */
char *codegen_render_patro_tables(const codegen_patro_tables *tables,
                                  const codegen_calendar *calendar,
                                  const codegen_patro_summary *summary);

void codegen_patro_input_free(codegen_patro_input *input);
void codegen_patro_tables_free(codegen_patro_tables *tables);

#endif /* NEPDATE_CODEGEN_PATRO_H */
