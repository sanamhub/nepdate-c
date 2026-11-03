/* The API v1 rows of a single date: the boundary dates, the BS year length, the day difference in
 * both directions, the packed YYYYMMDD integer, the interchange string and the clock predicates. */
#include "harness.h"
#include "vectors.h"

#include <nepdate.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static nepdate_date date_of(int year, int month, int day) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_bs(year, month, day, &date), NEPDATE_OK);
    return date;
}

static void check_date(nepdate_date actual, int year, int month, int day) {
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(actual, &ymd), NEPDATE_OK);
    CHECK_EQ_INT(ymd.year, year);
    CHECK_EQ_INT(ymd.month, month);
    CHECK_EQ_INT(ymd.day, day);
}

/* 2024-07-30 18:30 UTC: 2081-04-15 in UTC and 2081-04-16 in Nepal, the injected clock of the
 * relative-date rows. */
static const int64_t fixed_instant = INT64_C(1722364200);
static const int32_t nepal_offset = 345;

TEST(boundary_dates) {
    const nepdate_date min = NEPDATE_MIN;
    const nepdate_date max = NEPDATE_MAX;
    check_date(min, 1901, 1, 1);
    check_date(max, 2199, 12, 30);

    const nepdate_date from_function = nepdate_min();
    const nepdate_date last = nepdate_max();
    CHECK_EQ_INT(from_function.value, min.value);
    CHECK_EQ_INT(last.value, max.value);

    int compare = 0;
    CHECK_EQ_INT(nepdate_compare(min, from_function, &compare), NEPDATE_OK);
    CHECK_EQ_INT(compare, 0);
    CHECK_EQ_INT(nepdate_compare(min, max, &compare), NEPDATE_OK);
    CHECK_EQ_INT(compare, -1);

    nepdate_ymd ad = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_ad(min, &ad), NEPDATE_OK);
    CHECK(ad.year == 1844 && ad.month == 4 && ad.day == 11);
    CHECK_EQ_INT(nepdate_to_ad(max, &ad), NEPDATE_OK);
    CHECK(ad.year == 2143 && ad.month == 4 && ad.day == 15);

    /* The day before the first and the day after the last are outside the supported span. */
    nepdate_date earlier = {0};
    nepdate_date later = {0};
    CHECK_EQ_INT(nepdate_add_days(min, -1, &earlier), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_add_days(max, 1, &later), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_add_days(min, INT32_MIN, &earlier), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_add_days(max, INT32_MAX, &later), NEPDATE_E_OUT_OF_RANGE);
}

TEST(year_lengths) {
    vector_reader reader;
    CHECK(vector_open(&reader, "year-length.tsv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    int rows = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[3];
        CHECK_EQ_INT(vector_split(line, '\t', fields, 3), 2);
        const int year = atoi(fields[0]);
        const int expected = atoi(fields[1]);
        int length = 0;
        /* Any day of the year gives the same length: the first, a middle month and the last. */
        CHECK_EQ_INT(nepdate_year_length(date_of(year, 1, 1), &length), NEPDATE_OK);
        CHECK_EQ_INT(length, expected);
        CHECK_EQ_INT(nepdate_year_length(date_of(year, 7, 1), &length), NEPDATE_OK);
        CHECK_EQ_INT(length, expected);
        nepdate_range year_range = {{0}, {0}};
        CHECK_EQ_INT(nepdate_range_of_year(year, &year_range), NEPDATE_OK);
        CHECK_EQ_INT(nepdate_year_length(year_range.end, &length), NEPDATE_OK);
        CHECK_EQ_INT(length, expected);
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 299);

    int length = 0;
    CHECK_EQ_INT(nepdate_year_length(nepdate_min(), &length), NEPDATE_OK);
    CHECK(length >= 364 && length <= 367);
    CHECK_EQ_INT(nepdate_year_length(nepdate_max(), &length), NEPDATE_OK);

    nepdate_date invalid = {0};
    CHECK_EQ_INT(nepdate_year_length(invalid, &length), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_year_length(nepdate_min(), NULL), NEPDATE_E_NULL);
}

TEST(day_differences) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/day-diff.tsv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    int rows = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[6];
        CHECK_EQ_INT(vector_split(line, '\t', fields, 6), 5);
        nepdate_date a = {0};
        nepdate_date b = {0};
        CHECK(vector_parse_date(fields[0], &a));
        CHECK(vector_parse_date(fields[1], &b));
        const int32_t days_until = (int32_t)atoi(fields[2]);
        const int32_t operator_minus = (int32_t)atoi(fields[3]);
        /* D-07: the named difference is other - this, so days_until is b - a. */
        int32_t days = INT32_MAX;
        CHECK_EQ_INT(nepdate_diff_days(a, b, &days), NEPDATE_OK);
        CHECK_EQ_INT(days, days_until);
        /* The operator direction, this - other, is the reverse call. */
        CHECK_EQ_INT(nepdate_diff_days(b, a, &days), NEPDATE_OK);
        CHECK_EQ_INT(days, operator_minus);
        CHECK_EQ_INT(days_until, -operator_minus);
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 16);
}

TEST(packed_ymd_integer) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/ymd-int.tsv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    int rows = 0;
    int rejected = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[6];
        CHECK_EQ_INT(vector_split(line, '\t', fields, 6), 5);
        const bool as_date = strcmp(fields[0], "date") == 0;
        nepdate_date expected = {0};
        const bool expect_date = strcmp(fields[3], "ERR") != 0;
        if (expect_date) {
            CHECK(vector_parse_date(fields[3], &expected));
        }
        if (as_date) {
            nepdate_date date = {0};
            CHECK(vector_parse_date(fields[1], &date));
            int64_t packed = 0;
            CHECK_EQ_INT(nepdate_to_ymd_int(date, &packed), NEPDATE_OK);
            CHECK_EQ_INT(packed, atoll(fields[2]));
            nepdate_date roundtrip = {0};
            CHECK_EQ_INT(nepdate_from_ymd_int(packed, &roundtrip), NEPDATE_OK);
            CHECK_EQ_INT(roundtrip.value, date.value);
        } else {
            nepdate_date actual = {0};
            const nepdate_status status = nepdate_from_ymd_int(atoll(fields[2]), &actual);
            if (expect_date) {
                CHECK_EQ_INT(status, NEPDATE_OK);
                CHECK_EQ_INT(actual.value, expected.value);
            } else {
                CHECK(status != NEPDATE_OK);
                CHECK_EQ_INT(actual.value, 0);
                rejected++;
            }
        }
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 14);
    CHECK_EQ_INT(rejected, 8);

    /* No part is truncated: the widest and the narrowest integer are read whole and refused. */
    nepdate_date date = {date_of(2081, 4, 15).value};
    CHECK_EQ_INT(nepdate_from_ymd_int(INT64_MAX, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(date.value, date_of(2081, 4, 15).value);
    CHECK_EQ_INT(nepdate_from_ymd_int(INT64_MIN, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_from_ymd_int(INT32_MAX, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_from_ymd_int(INT32_MIN, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_from_ymd_int(20810415, NULL), NEPDATE_E_NULL);

    int64_t packed = 0;
    CHECK_EQ_INT(nepdate_to_ymd_int(date_of(2199, 12, 30), &packed), NEPDATE_OK);
    CHECK_EQ_INT(packed, 21991230);
    CHECK_EQ_INT(nepdate_to_ymd_int(date_of(1901, 1, 1), &packed), NEPDATE_OK);
    CHECK_EQ_INT(packed, 19010101);
    CHECK_EQ_INT(nepdate_to_ymd_int(nepdate_max(), &packed), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_ymd_int(date, NULL), NEPDATE_E_NULL);
    nepdate_date invalid = {0};
    CHECK_EQ_INT(nepdate_to_ymd_int(invalid, &packed), NEPDATE_E_INVALID_DATE);
}

/* The body of a JSON string, quotes included in the vector field, or false when the field is not a
 * string. The interchange hook is a string, so a JSON null has no form here (D-01). */
static bool json_string(const char *field, const char **body, size_t *length) {
    const size_t field_length = strlen(field);
    if (field_length < 2 || field[0] != '"' || field[field_length - 1] != '"') {
        return false;
    }
    *body = field + 1;
    *length = field_length - 2;
    return true;
}

TEST(interchange_string) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/serialization.tsv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    int rows = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[6];
        CHECK_EQ_INT(vector_split(line, '\t', fields, 6), 5);
        const bool write = strcmp(fields[0], "write") == 0;
        const bool read = strcmp(fields[0], "read") == 0;
        const bool roundtrip = strcmp(fields[0], "roundtrip") == 0;

        if (write || roundtrip) {
            nepdate_date date = {0};
            CHECK(vector_parse_date(fields[1], &date));
            char text[16];
            const size_t length = nepdate_format_iso(date, text, sizeof text);
            CHECK_EQ_INT(length, 10);
            const char *body = NULL;
            size_t body_length = 0;
            CHECK(json_string(fields[2], &body, &body_length));
            CHECK_EQ_INT(body_length, 10);
            CHECK_EQ_INT(length, body_length);
            CHECK_EQ_INT(memcmp(text, body, body_length), 0);
            if (roundtrip) {
                nepdate_date parsed = {0};
                CHECK_EQ_INT(nepdate_parse(text, length, &parsed), NEPDATE_OK);
                nepdate_date expected = {0};
                CHECK(vector_parse_date(fields[3], &expected));
                CHECK_EQ_INT(parsed.value, expected.value);
            }
        } else if (read) {
            const char *body = NULL;
            size_t body_length = 0;
            nepdate_date parsed = {0};
            const bool is_string = json_string(fields[1], &body, &body_length);
            const nepdate_status status =
                is_string ? nepdate_parse(body, body_length, &parsed) : NEPDATE_E_NULL;
            if (strcmp(fields[3], "ERR") == 0) {
                CHECK(status != NEPDATE_OK);
            } else {
                CHECK_EQ_INT(status, NEPDATE_OK);
                nepdate_date expected = {0};
                CHECK(vector_parse_date(fields[3], &expected));
                CHECK_EQ_INT(parsed.value, expected.value);
            }
        } else {
            FAIL("unknown serialization row kind");
        }
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 11);
}

TEST(interchange_string_buffers) {
    const nepdate_date date = date_of(2081, 4, 15);
    char buffer[16];
    memset(buffer, 0x5A, sizeof buffer);
    CHECK_EQ_INT(nepdate_format_iso(date, buffer, 0), 10);
    CHECK_EQ_INT((unsigned char)buffer[0], 0x5A);
    CHECK_EQ_INT(nepdate_format_iso(date, buffer, 9), 10);
    CHECK_EQ_INT(memcmp(buffer, "2081-04-", 8), 0);
    CHECK_EQ_INT((unsigned char)buffer[8], 0);
    CHECK_EQ_INT(nepdate_format_iso(date, NULL, 4), 10);
    CHECK_EQ_INT(nepdate_format_iso(date, buffer, sizeof buffer), 10);
    CHECK_EQ_INT(strcmp(buffer, "2081-04-15"), 0);
    const nepdate_date invalid = {-1};
    CHECK_EQ_INT(nepdate_format_iso(invalid, buffer, sizeof buffer), 0);
    CHECK_EQ_INT(memcmp(buffer, "2081-04-15", 10), 0);
}

TEST(today_at_an_offset) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_today(nepal_offset, fixed_instant, &date), NEPDATE_OK);
    check_date(date, 2081, 4, 16);
    CHECK_EQ_INT(nepdate_today(0, fixed_instant, &date), NEPDATE_OK);
    check_date(date, 2081, 4, 15);
    CHECK_EQ_INT(nepdate_today(-600, fixed_instant, &date), NEPDATE_OK);
    check_date(date, 2081, 4, 15);

    /* The offset can carry the instant over midnight in either direction. */
    const int64_t late_utc = fixed_instant + INT64_C(19200);
    CHECK_EQ_INT(nepdate_today(nepal_offset, late_utc, &date), NEPDATE_OK);
    check_date(date, 2081, 4, 16);
    CHECK_EQ_INT(nepdate_today(0, late_utc, &date), NEPDATE_OK);
    check_date(date, 2081, 4, 15);

    /* The same date as nepdate_today_nepal, which knows Nepal's own offsets. */
    nepdate_date nepal = {0};
    CHECK_EQ_INT(nepdate_today_nepal(fixed_instant, &nepal), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_today(nepal_offset, fixed_instant, &date), NEPDATE_OK);
    CHECK_EQ_INT(date.value, nepal.value);

    CHECK_EQ_INT(nepdate_today(nepal_offset, fixed_instant, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_today(0, INT64_MIN, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_today(0, INT64_MAX, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_today(INT32_MIN, INT64_MIN, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_today(INT32_MAX, INT64_MAX, &date), NEPDATE_E_OUT_OF_RANGE);
}

TEST(relative_days) {
    const nepdate_date today = date_of(2081, 4, 16);
    const nepdate_date yesterday = date_of(2081, 4, 15);
    const nepdate_date tomorrow = date_of(2081, 4, 17);
    bool actual = false;

    CHECK_EQ_INT(nepdate_is_today(today, nepal_offset, fixed_instant, &actual), NEPDATE_OK);
    CHECK(actual);
    CHECK_EQ_INT(nepdate_is_today(yesterday, nepal_offset, fixed_instant, &actual), NEPDATE_OK);
    CHECK(!actual);
    CHECK_EQ_INT(nepdate_is_today(tomorrow, nepal_offset, fixed_instant, &actual), NEPDATE_OK);
    CHECK(!actual);

    CHECK_EQ_INT(nepdate_is_yesterday(yesterday, nepal_offset, fixed_instant, &actual), NEPDATE_OK);
    CHECK(actual);
    CHECK_EQ_INT(nepdate_is_yesterday(today, nepal_offset, fixed_instant, &actual), NEPDATE_OK);
    CHECK(!actual);

    CHECK_EQ_INT(nepdate_is_tomorrow(tomorrow, nepal_offset, fixed_instant, &actual), NEPDATE_OK);
    CHECK(actual);
    CHECK_EQ_INT(nepdate_is_tomorrow(today, nepal_offset, fixed_instant, &actual), NEPDATE_OK);
    CHECK(!actual);

    /* The zone is the caller's: the same instant holds a different local day at UTC. */
    CHECK_EQ_INT(nepdate_is_today(yesterday, 0, fixed_instant, &actual), NEPDATE_OK);
    CHECK(actual);
    CHECK_EQ_INT(nepdate_is_tomorrow(today, 0, fixed_instant, &actual), NEPDATE_OK);
    CHECK(actual);

    /* At the last supported day there is no tomorrow, and at the first there is no yesterday. */
    const nepdate_date first = NEPDATE_MIN;
    const nepdate_date last = NEPDATE_MAX;
    CHECK_EQ_INT(nepdate_is_today(first, 0, -INT64_C(3967488000), &actual), NEPDATE_OK);
    CHECK(actual);
    CHECK_EQ_INT(nepdate_is_yesterday(first, 0, -INT64_C(3967488000), &actual), NEPDATE_OK);
    CHECK(!actual);
    CHECK_EQ_INT(nepdate_is_today(last, nepal_offset, INT64_C(5468342400), &actual), NEPDATE_OK);
    CHECK(actual);
    CHECK_EQ_INT(nepdate_is_tomorrow(last, nepal_offset, INT64_C(5468342400), &actual), NEPDATE_OK);
    CHECK(!actual);

    CHECK_EQ_INT(nepdate_is_today(today, nepal_offset, fixed_instant, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_is_yesterday(today, nepal_offset, fixed_instant, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_is_tomorrow(today, nepal_offset, fixed_instant, NULL), NEPDATE_E_NULL);
    nepdate_date invalid = {0};
    CHECK_EQ_INT(nepdate_is_today(invalid, nepal_offset, fixed_instant, &actual),
                 NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_is_yesterday(invalid, nepal_offset, fixed_instant, &actual),
                 NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_is_tomorrow(invalid, nepal_offset, fixed_instant, &actual),
                 NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_is_today(today, 0, INT64_MIN, &actual), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_is_today(today, 0, INT64_MAX, &actual), NEPDATE_E_OUT_OF_RANGE);
}

int main(void) {
    RUN_TEST(boundary_dates);
    RUN_TEST(year_lengths);
    RUN_TEST(day_differences);
    RUN_TEST(packed_ymd_integer);
    RUN_TEST(interchange_string);
    RUN_TEST(interchange_string_buffers);
    RUN_TEST(today_at_an_offset);
    RUN_TEST(relative_days);
    return harness_finish();
}
