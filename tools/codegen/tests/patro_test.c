#include "calendar.h"
#include "harness.h"
#include "json.h"
#include "patro.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path, size_t *length) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    size_t capacity = 4096;
    size_t size = 0;
    char *text = malloc(capacity);
    while (text != NULL) {
        const size_t n = fread(text + size, 1, capacity - size - 1U, file);
        size += n;
        if (n == 0) {
            break;
        }
        if (capacity - size - 1U == 0) {
            capacity *= 2U;
            char *bigger = realloc(text, capacity);
            if (bigger == NULL) {
                free(text);
                text = NULL;
                break;
            }
            text = bigger;
        }
    }
    (void)fclose(file);
    if (text != NULL) {
        text[size] = '\0';
        *length = size;
    }
    return text;
}

static codegen_calendar *shared_calendar(void) {
    size_t length = 0;
    char *text = read_file(NEPDATE_C_SHARED_DIR "/data/calendar/bs-calendar.json", &length);
    codegen_calendar *calendar = malloc(sizeof *calendar);
    char error[256] = "";
    const bool ok = text != NULL && calendar != NULL &&
                    codegen_parse_calendar(text, length, calendar, error, sizeof error);
    free(text);
    if (!ok) {
        free(calendar);
        return NULL;
    }
    return calendar;
}

static bool load_patro(codegen_patro_input *input) {
    for (int year = CODEGEN_PATRO_FIRST_YEAR; year <= CODEGEN_PATRO_LAST_YEAR; year++) {
        char path[128];
        (void)snprintf(path, sizeof path, NEPDATE_C_SHARED_DIR "/data/patro/%d.json", year);
        size_t length = 0;
        char *text = read_file(path, &length);
        if (text == NULL) {
            return false;
        }
        char json_error[128];
        json_value *doc = json_parse(text, length, json_error, sizeof json_error);
        free(text);
        if (doc == NULL) {
            return false;
        }
        char error[256] = "";
        const bool added = codegen_patro_add_year(doc, year, input, error, sizeof error);
        json_free(doc);
        if (!added) {
            return false;
        }
    }
    return true;
}

static uint32_t serial_of(const codegen_calendar *calendar, int year, int month, int day) {
    const uint32_t index =
        ((uint32_t)year - (uint32_t)CODEGEN_MIN_YEAR) * (uint32_t)CODEGEN_MONTHS + (uint32_t)month -
        1U;
    return (uint32_t)(calendar->month_start[index] + (long)day - 1L);
}

TEST(shared_patro_builds) {
    codegen_calendar *calendar = shared_calendar();
    codegen_patro_input input = {0};
    CHECK(calendar != NULL);
    if (calendar == NULL || !load_patro(&input)) {
        codegen_patro_input_free(&input);
        free(calendar);
        return;
    }
    codegen_patro_tables tables;
    char error[256] = "";
    CHECK(codegen_build_patro_tables(&input, calendar, &tables, error, sizeof error));
    CHECK_EQ_STR(error, "");
    CHECK_EQ_INT(tables.first, 64285);
    CHECK_EQ_INT(tables.days, 2793);
    CHECK_EQ_INT(tables.name_count, 753);
    CHECK_EQ_INT(tables.name_blob_size, 59800);
    CHECK_EQ_INT(tables.holiday_count, 294);
    CHECK_EQ_INT(tables.event_count, 3032);
    CHECK_EQ_INT(tables.day_tithi[0], 0xFF);
    const uint32_t first_tithi = serial_of(calendar, 2078, 1, 2) - tables.first;
    const uint32_t last_tithi = serial_of(calendar, 2083, 12, 30) - tables.first;
    CHECK(tables.day_tithi[first_tithi] != 0xFF);
    CHECK(tables.day_tithi[last_tithi] != 0xFF);
    CHECK(tables.day_tithi[first_tithi - 1U] == 0xFF);
    CHECK(tables.name_offsets[0] == 0);

    char *header = codegen_render_patro_tables(&tables, calendar);
    CHECK(header != NULL);
    if (header != NULL) {
        CHECK(strstr(header, "#define NEPDATE_PATRO_FIRST_SERIAL 64285U\n") != NULL);
        CHECK(strstr(header, "#define NEPDATE_PATRO_DAYS 2793U\n") != NULL);
        CHECK(strstr(header, "#define NEPDATE_PATRO_HOLIDAYS 294U\n") != NULL);
        CHECK(strstr(header, "static const char nepdate_patro_name_blob[59800]") != NULL);
        CHECK(strchr(header, '\r') == NULL);
        free(header);
    }
    codegen_patro_tables_free(&tables);
    codegen_patro_input_free(&input);
    free(calendar);
}

TEST(tithi_count_is_checked) {
    codegen_calendar *calendar = shared_calendar();
    codegen_patro_input input = {0};
    CHECK(calendar != NULL);
    if (calendar == NULL || !load_patro(&input)) {
        codegen_patro_input_free(&input);
        free(calendar);
        return;
    }
    for (size_t i = 0; i < input.count; i++) {
        if (input.days[i].has_tithi) {
            input.days[i].has_tithi = false;
            break;
        }
    }
    codegen_patro_tables tables;
    char error[256] = "";
    CHECK(!codegen_build_patro_tables(&input, calendar, &tables, error, sizeof error));
    CHECK_EQ_STR(error, "patro data: T = 2189, spec says 2190");
    codegen_patro_tables_free(&tables);
    codegen_patro_input_free(&input);
    free(calendar);
}

TEST(holiday_count_is_checked) {
    codegen_calendar *calendar = shared_calendar();
    codegen_patro_input input = {0};
    CHECK(calendar != NULL);
    if (calendar == NULL || !load_patro(&input)) {
        codegen_patro_input_free(&input);
        free(calendar);
        return;
    }
    for (size_t i = 0; i < input.count; i++) {
        if (input.days[i].holiday) {
            input.days[i].holiday = false;
            break;
        }
    }
    codegen_patro_tables tables;
    char error[256] = "";
    CHECK(!codegen_build_patro_tables(&input, calendar, &tables, error, sizeof error));
    CHECK_EQ_STR(error, "patro data: H = 293, spec says 294");
    codegen_patro_tables_free(&tables);
    codegen_patro_input_free(&input);
    free(calendar);
}

TEST(event_day_count_is_checked) {
    codegen_calendar *calendar = shared_calendar();
    codegen_patro_input input = {0};
    CHECK(calendar != NULL);
    if (calendar == NULL || !load_patro(&input)) {
        codegen_patro_input_free(&input);
        free(calendar);
        return;
    }
    for (size_t i = 0; i < input.count; i++) {
        codegen_patro_day *day = &input.days[i];
        /* Its only event name appears on other days too, so the name table stays complete. */
        if (day->year != 2077 || day->month != 1 || day->day != 3) {
            continue;
        }
        for (size_t k = 0; k < day->event_count; k++) {
            free(day->events[k].np);
            free(day->events[k].en);
        }
        free(day->events);
        day->events = NULL;
        day->event_count = 0;
        day->event_capacity = 0;
        break;
    }
    codegen_patro_tables tables;
    char error[256] = "";
    CHECK(!codegen_build_patro_tables(&input, calendar, &tables, error, sizeof error));
    CHECK_EQ_STR(error, "patro data: E = 1670, spec says 1671");
    codegen_patro_tables_free(&tables);
    codegen_patro_input_free(&input);
    free(calendar);
}

TEST(malformed_year_is_rejected) {
    static const char text[] = "{\"schema\": 1, \"year\": 2081,"
                               " \"days\": {\"2081-01-01\": {\"holiday\": 1}}}";
    char json_error[128];
    json_value *doc = json_parse(text, strlen(text), json_error, sizeof json_error);
    CHECK(doc != NULL);
    if (doc == NULL) {
        return;
    }
    codegen_patro_input input = {0};
    char error[256] = "";
    CHECK(!codegen_patro_add_year(doc, 2081, &input, error, sizeof error));
    CHECK_EQ_STR(error, "patro data: 2081-01-01: holiday must be a boolean");
    codegen_patro_input_free(&input);
    json_free(doc);
}

TEST(year_mismatch_is_rejected) {
    static const char text[] = "{\"schema\": 1, \"year\": 2081,"
                               " \"days\": {\"2081-01-01\": {\"holiday\": true}}}";
    char json_error[128];
    json_value *doc = json_parse(text, strlen(text), json_error, sizeof json_error);
    CHECK(doc != NULL);
    if (doc == NULL) {
        return;
    }
    codegen_patro_input input = {0};
    char error[256] = "";
    CHECK(!codegen_patro_add_year(doc, 2082, &input, error, sizeof error));
    CHECK_EQ_STR(error, "patro data: year must be 2082");
    codegen_patro_input_free(&input);
    json_free(doc);
}

int main(void) {
    RUN_TEST(shared_patro_builds);
    RUN_TEST(tithi_count_is_checked);
    RUN_TEST(holiday_count_is_checked);
    RUN_TEST(event_day_count_is_checked);
    RUN_TEST(malformed_year_is_rejected);
    RUN_TEST(year_mismatch_is_rejected);
    return harness_finish();
}
