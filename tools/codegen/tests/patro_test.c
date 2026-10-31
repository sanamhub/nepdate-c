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

static codegen_patro_summary *shared_summary(void) {
    size_t length = 0;
    char *text = read_file(NEPDATE_C_SHARED_DIR "/spec/vectors/patro-summary.json", &length);
    codegen_patro_summary *summary = malloc(sizeof *summary);
    char json_error[128] = "";
    char error[256] = "";
    json_value *doc = text == NULL ? NULL : json_parse(text, length, json_error, sizeof json_error);
    free(text);
    const bool ok = doc != NULL && summary != NULL &&
                    codegen_patro_summary_parse(doc, summary, error, sizeof error);
    json_free(doc);
    if (!ok) {
        free(summary);
        return NULL;
    }
    return summary;
}

static bool load_patro(const codegen_patro_summary *summary, codegen_patro_input *input) {
    for (int year = summary->first_year; year <= summary->last_year; year++) {
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

static bool day_parts(const char *text, int *year, int *month, int *day) {
    return text != NULL && sscanf(text, "%d-%d-%d", year, month, day) == 3;
}

/* True when every event of day has a name that another day carries too. Removing such a day's
 * events changes the event-day count without changing the name table. */
static bool events_are_shared(const codegen_patro_input *input, size_t index) {
    for (size_t k = 0; k < input->days[index].event_count; k++) {
        const codegen_patro_name *name = &input->days[index].events[k];
        bool found = false;
        for (size_t i = 0; i < input->count && !found; i++) {
            if (i == index) {
                continue;
            }
            for (size_t n = 0; n < input->days[i].event_count && !found; n++) {
                found = strcmp(input->days[i].events[n].np, name->np) == 0 &&
                        strcmp(input->days[i].events[n].en, name->en) == 0;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

static bool drop_one_shared_event_day(codegen_patro_input *input) {
    for (size_t i = 0; i < input->count; i++) {
        if (input->days[i].event_count > 0 && events_are_shared(input, i)) {
            codegen_patro_day *day = &input->days[i];
            for (size_t k = 0; k < day->event_count; k++) {
                free(day->events[k].np);
                free(day->events[k].en);
            }
            free(day->events);
            day->events = NULL;
            day->event_count = 0;
            day->event_capacity = 0;
            return true;
        }
    }
    return false;
}

TEST(shared_patro_builds) {
    codegen_calendar *calendar = shared_calendar();
    codegen_patro_summary *summary = shared_summary();
    codegen_patro_input input = {0};
    CHECK(calendar != NULL);
    CHECK(summary != NULL);
    if (calendar == NULL || summary == NULL || !load_patro(summary, &input)) {
        codegen_patro_input_free(&input);
        free(summary);
        free(calendar);
        return;
    }
    codegen_patro_tables tables;
    char error[256] = "";
    CHECK(codegen_build_patro_tables(&input, calendar, summary, &tables, error, sizeof error));
    CHECK_EQ_STR(error, "");
    CHECK_EQ_INT(tables.first, summary->first_serial);
    CHECK_EQ_INT(tables.days, summary->days);
    CHECK_EQ_INT(tables.name_count, summary->names);
    CHECK_EQ_INT(tables.name_blob_size, summary->name_bytes);
    CHECK_EQ_INT(tables.holiday_count, summary->holidays);
    CHECK_EQ_INT(tables.event_count, summary->events);

    int ty = 0, tm = 0, td = 0;
    CHECK(day_parts(summary->tithi_first_day, &ty, &tm, &td));
    const uint32_t first_tithi = serial_of(calendar, ty, tm, td) - tables.first;
    CHECK(day_parts(summary->tithi_last_day, &ty, &tm, &td));
    const uint32_t last_tithi = serial_of(calendar, ty, tm, td) - tables.first;
    CHECK(tables.day_tithi[first_tithi] != 0xFF);
    CHECK(tables.day_tithi[last_tithi] != 0xFF);
    CHECK(first_tithi == 0 || tables.day_tithi[first_tithi - 1U] == 0xFF);
    CHECK(tables.name_offsets[0] == 0);

    char *header = codegen_render_patro_tables(&tables, calendar, summary);
    CHECK(header != NULL);
    if (header != NULL) {
        char expected[128];
        (void)snprintf(expected, sizeof expected, "#define NEPDATE_PATRO_FIRST_SERIAL %uU\n",
                       (unsigned)summary->first_serial);
        CHECK(strstr(header, expected) != NULL);
        (void)snprintf(expected, sizeof expected, "#define NEPDATE_PATRO_DAYS %uU\n",
                       (unsigned)summary->days);
        CHECK(strstr(header, expected) != NULL);
        (void)snprintf(expected, sizeof expected, "#define NEPDATE_PATRO_HOLIDAYS %uU\n",
                       (unsigned)summary->holidays);
        CHECK(strstr(header, expected) != NULL);
        (void)snprintf(expected, sizeof expected, "static const char nepdate_patro_name_blob[%zu]",
                       tables.name_blob_size);
        CHECK(strstr(header, expected) != NULL);
        CHECK(strchr(header, '\r') == NULL);
        free(header);
    }
    codegen_patro_tables_free(&tables);
    codegen_patro_input_free(&input);
    free(summary);
    free(calendar);
}

TEST(tithi_count_is_checked) {
    codegen_calendar *calendar = shared_calendar();
    codegen_patro_summary *summary = shared_summary();
    codegen_patro_input input = {0};
    CHECK(calendar != NULL);
    CHECK(summary != NULL);
    if (calendar == NULL || summary == NULL || !load_patro(summary, &input)) {
        codegen_patro_input_free(&input);
        free(summary);
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
    CHECK(!codegen_build_patro_tables(&input, calendar, summary, &tables, error, sizeof error));
    char expected[128];
    (void)snprintf(expected, sizeof expected, "patro data: T = %u, summary says %u",
                   (unsigned)(summary->tithi_days - 1U), (unsigned)summary->tithi_days);
    CHECK_EQ_STR(error, expected);
    codegen_patro_tables_free(&tables);
    codegen_patro_input_free(&input);
    free(summary);
    free(calendar);
}

TEST(holiday_count_is_checked) {
    codegen_calendar *calendar = shared_calendar();
    codegen_patro_summary *summary = shared_summary();
    codegen_patro_input input = {0};
    CHECK(calendar != NULL);
    CHECK(summary != NULL);
    if (calendar == NULL || summary == NULL || !load_patro(summary, &input)) {
        codegen_patro_input_free(&input);
        free(summary);
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
    CHECK(!codegen_build_patro_tables(&input, calendar, summary, &tables, error, sizeof error));
    char expected[128];
    (void)snprintf(expected, sizeof expected, "patro data: H = %u, summary says %u",
                   (unsigned)(summary->holidays - 1U), (unsigned)summary->holidays);
    CHECK_EQ_STR(error, expected);
    codegen_patro_tables_free(&tables);
    codegen_patro_input_free(&input);
    free(summary);
    free(calendar);
}

TEST(event_day_count_is_checked) {
    codegen_calendar *calendar = shared_calendar();
    codegen_patro_summary *summary = shared_summary();
    codegen_patro_input input = {0};
    CHECK(calendar != NULL);
    CHECK(summary != NULL);
    if (calendar == NULL || summary == NULL || !load_patro(summary, &input)) {
        codegen_patro_input_free(&input);
        free(summary);
        free(calendar);
        return;
    }
    CHECK(drop_one_shared_event_day(&input));
    codegen_patro_tables tables;
    char error[256] = "";
    CHECK(!codegen_build_patro_tables(&input, calendar, summary, &tables, error, sizeof error));
    char expected[128];
    (void)snprintf(expected, sizeof expected, "patro data: E = %u, summary says %u",
                   (unsigned)(summary->event_days - 1U), (unsigned)summary->event_days);
    CHECK_EQ_STR(error, expected);
    codegen_patro_tables_free(&tables);
    codegen_patro_input_free(&input);
    free(summary);
    free(calendar);
}

/* The summary is the port's only source of counts, so a summary that disagrees with the data is a
 * failed build even when every value in it is well formed. */
TEST(summary_mismatch_is_rejected) {
    codegen_calendar *calendar = shared_calendar();
    codegen_patro_summary *summary = shared_summary();
    codegen_patro_input input = {0};
    CHECK(calendar != NULL);
    CHECK(summary != NULL);
    if (calendar == NULL || summary == NULL || !load_patro(summary, &input)) {
        codegen_patro_input_free(&input);
        free(summary);
        free(calendar);
        return;
    }
    summary->names++;
    codegen_patro_tables tables;
    char error[256] = "";
    CHECK(!codegen_build_patro_tables(&input, calendar, summary, &tables, error, sizeof error));
    char expected[128];
    (void)snprintf(expected, sizeof expected, "patro data: %u names, summary says %u",
                   (unsigned)(summary->names - 1U), (unsigned)summary->names);
    CHECK_EQ_STR(error, expected);
    codegen_patro_tables_free(&tables);
    codegen_patro_input_free(&input);
    free(summary);
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
    RUN_TEST(summary_mismatch_is_rejected);
    RUN_TEST(malformed_year_is_rejected);
    RUN_TEST(year_mismatch_is_rejected);
    return harness_finish();
}
