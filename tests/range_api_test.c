/* The range API: every row of the five range vector files, then the caller-buffer and data-end
 * rules around the set operations, the splits and the day cursors. */
#include "harness.h"
#include "vectors.h"

#include <nepdate.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct text_builder {
    char text[2048];
    size_t length;
    bool overflow;
} text_builder;

static void append(text_builder *builder, const char *text) {
    const size_t length = strlen(text);
    if (builder->length + length + 1 >= sizeof builder->text) {
        builder->overflow = true;
        return;
    }
    memcpy(builder->text + builder->length, text, length + 1);
    builder->length += length;
}

static void append_separator(text_builder *builder) {
    if (builder->length > 0) {
        append(builder, ";");
    }
}

static void append_date(text_builder *builder, nepdate_date date) {
    char text[16];
    vector_format_date(date, text, sizeof text);
    append(builder, text);
}

static void append_range(text_builder *builder, nepdate_range range) {
    char text[40];
    vector_format_range(range, text, sizeof text);
    append(builder, text);
}

/* The text a range list is compared with: the pieces joined by ';', or "none" when there are
 * none. */
static const char *list_text(text_builder *builder) {
    CHECK(!builder->overflow);
    return builder->length == 0 ? "none" : builder->text;
}

static bool parse_bool(const char *text, bool *out) {
    if (strcmp(text, "true") == 0) {
        *out = true;
        return true;
    }
    if (strcmp(text, "false") == 0) {
        *out = false;
        return true;
    }
    return false;
}

static bool result_is(const char *field, const char *value) { return strcmp(field, value) == 0; }

/* A range field that may be absent, because the row holds an invalid date or a reversed pair. */
static bool optional_range(const char *start, const char *end, nepdate_range *out) {
    nepdate_date first = {0};
    nepdate_date last = {0};
    if (!vector_parse_date(start, &first) || !vector_parse_date(end, &last)) {
        return false;
    }
    return nepdate_range_make(first, last, out) == NEPDATE_OK;
}

/* Compares a boolean result field with what the call produced. */
static void check_bool_field(const char *field, bool actual) {
    bool expected = false;
    CHECK(parse_bool(field, &expected));
    CHECK_EQ_INT(actual, expected);
}

TEST(golden_range_rows) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/ranges.tsv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    int rows = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[7];
        CHECK_EQ_INT(vector_split(line, '\t', fields, 7), 6);
        const char *operation = fields[2];
        const char *argument = fields[3];
        const char *expected = fields[4];

        nepdate_range range = {{0}, {0}};
        const bool range_ok = optional_range(fields[0], fields[1], &range);

        if (strcmp(operation, "construct") == 0) {
            if (result_is(expected, "ERR")) {
                CHECK(!range_ok);
            } else {
                CHECK(range_ok);
                char text[40];
                vector_format_range(range, text, sizeof text);
                CHECK_EQ_STR(text, expected);
            }
            rows++;
            continue;
        }

        CHECK(range_ok);
        if (!range_ok) {
            continue;
        }

        if (strcmp(operation, "length") == 0) {
            int length = 0;
            CHECK_EQ_INT(nepdate_range_length(range, &length), NEPDATE_OK);
            CHECK_EQ_INT(length, atoi(expected));
        } else if (strcmp(operation, "contains_date") == 0) {
            nepdate_date date = {0};
            CHECK(vector_parse_date(argument, &date));
            bool contains = false;
            CHECK_EQ_INT(nepdate_range_contains(range, date, &contains), NEPDATE_OK);
            check_bool_field(expected, contains);
        } else if (strcmp(operation, "contains_range") == 0 || strcmp(operation, "overlaps") == 0 ||
                   strcmp(operation, "is_adjacent_to") == 0 || strcmp(operation, "equals") == 0) {
            nepdate_range other = {{0}, {0}};
            CHECK(vector_parse_range(argument, &other));
            bool actual = false;
            nepdate_status status = NEPDATE_OK;
            if (strcmp(operation, "contains_range") == 0) {
                status = nepdate_range_contains_range(range, other, &actual);
            } else if (strcmp(operation, "overlaps") == 0) {
                status = nepdate_range_overlaps(range, other, &actual);
            } else if (strcmp(operation, "is_adjacent_to") == 0) {
                status = nepdate_range_is_adjacent_to(range, other, &actual);
            } else {
                status = nepdate_range_equal(range, other, &actual);
            }
            CHECK_EQ_INT(status, NEPDATE_OK);
            check_bool_field(expected, actual);
        } else if (strcmp(operation, "intersect") == 0) {
            nepdate_range other = {{0}, {0}};
            CHECK(vector_parse_range(argument, &other));
            nepdate_range actual = {{0}, {0}};
            const nepdate_status status = nepdate_range_intersect(range, other, &actual);
            if (result_is(expected, "none")) {
                /* D-01 and D-16: a port has no empty range, so a disjoint pair has no result. */
                CHECK_EQ_INT(status, NEPDATE_E_NO_OVERLAP);
                CHECK_EQ_INT(actual.start.value, 0);
                CHECK_EQ_INT(actual.end.value, 0);
            } else {
                CHECK_EQ_INT(status, NEPDATE_OK);
                char text[40];
                vector_format_range(actual, text, sizeof text);
                CHECK_EQ_STR(text, expected);
            }
        } else if (strcmp(operation, "union") == 0) {
            nepdate_range other = {{0}, {0}};
            CHECK(vector_parse_range(argument, &other));
            nepdate_range actual = {{0}, {0}};
            CHECK_EQ_INT(nepdate_range_union(range, other, &actual), NEPDATE_OK);
            char text[40];
            vector_format_range(actual, text, sizeof text);
            CHECK_EQ_STR(text, expected);
        } else if (strcmp(operation, "except") == 0) {
            nepdate_range other = {{0}, {0}};
            CHECK(vector_parse_range(argument, &other));
            nepdate_range pieces[2] = {{{0}, {0}}, {{0}, {0}}};
            size_t written = 99;
            CHECK_EQ_INT(nepdate_range_except(range, other, pieces, &written), NEPDATE_OK);
            text_builder builder = {{0}, 0, false};
            for (size_t i = 0; i < written; i++) {
                append_separator(&builder);
                append_range(&builder, pieces[i]);
            }
            CHECK_EQ_STR(list_text(&builder), expected);
        } else if (strcmp(operation, "to_string") == 0) {
            char text[64];
            const size_t length = nepdate_range_format(range, text, sizeof text);
            CHECK_EQ_INT(length, strlen(expected));
            CHECK_EQ_STR(text, expected);
        } else if (strcmp(operation, "enumerate") == 0) {
            nepdate_range_iterator iterator = {0, 0};
            CHECK_EQ_INT(nepdate_range_begin(range, &iterator), NEPDATE_OK);
            text_builder builder = {{0}, 0, false};
            nepdate_date date = {0};
            while (nepdate_range_next(&iterator, &date)) {
                append_separator(&builder);
                append_date(&builder, date);
            }
            CHECK_EQ_STR(list_text(&builder), expected);
        } else {
            FAIL("unknown range operation");
        }
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 54);
}

TEST(golden_range_factories) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/range-factories.tsv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    int rows = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[5];
        CHECK_EQ_INT(vector_split(line, '\t', fields, 5), 4);
        const char *factory = fields[0];
        char *argument = fields[1];
        const char *expected = fields[2];

        char *parts[3];
        const int count = vector_split(argument, ',', parts, 3);
        nepdate_range actual = {{0}, {0}};
        nepdate_status status = NEPDATE_OK;
        if (strcmp(factory, "single_day") == 0) {
            nepdate_date date = {0};
            CHECK_EQ_INT(count, 1);
            status = vector_parse_date(parts[0], &date) ? nepdate_range_make(date, date, &actual)
                                                        : NEPDATE_E_OUT_OF_RANGE;
        } else if (strcmp(factory, "from_day_count") == 0) {
            nepdate_date start = {0};
            CHECK_EQ_INT(count, 2);
            CHECK(vector_parse_date(parts[0], &start));
            status = nepdate_range_from_day_count(start, (int32_t)atoll(parts[1]), &actual);
        } else if (strcmp(factory, "for_month") == 0) {
            CHECK_EQ_INT(count, 2);
            status = nepdate_range_of_month(atoi(parts[0]), atoi(parts[1]), &actual);
        } else if (strcmp(factory, "for_calendar_year") == 0) {
            CHECK_EQ_INT(count, 1);
            status = nepdate_range_of_year(atoi(parts[0]), &actual);
        } else if (strcmp(factory, "for_fiscal_year") == 0) {
            CHECK_EQ_INT(count, 1);
            status = nepdate_range_of_fiscal_year(atoi(parts[0]), &actual);
        } else if (strcmp(factory, "construct") == 0) {
            nepdate_date first = {0};
            nepdate_date last = {0};
            CHECK_EQ_INT(count, 2);
            CHECK(vector_parse_date(parts[0], &first));
            CHECK(vector_parse_date(parts[1], &last));
            status = nepdate_range_make(first, last, &actual);
        } else {
            FAIL("unknown range factory");
            continue;
        }

        if (result_is(expected, "ERR")) {
            CHECK(status != NEPDATE_OK);
        } else {
            CHECK_EQ_INT(status, NEPDATE_OK);
            char text[40];
            vector_format_range(actual, text, sizeof text);
            CHECK_EQ_STR(text, expected);
        }
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 27);
}

TEST(golden_range_splits) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/range-splits.tsv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    int rows = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[6];
        CHECK_EQ_INT(vector_split(line, '\t', fields, 6), 5);
        nepdate_range range = {{0}, {0}};
        CHECK(optional_range(fields[0], fields[1], &range));
        nepdate_range pieces[32] = {{{0}, {0}}};
        size_t written = 0;
        nepdate_status status = NEPDATE_OK;
        if (strcmp(fields[2], "split_by_month") == 0) {
            status = nepdate_range_split_by_month(range, pieces, 32, &written);
        } else if (strcmp(fields[2], "split_by_fiscal_quarter") == 0) {
            status = nepdate_range_split_by_fiscal_quarter(range, pieces, 32, &written);
        } else {
            FAIL("unknown range split");
            continue;
        }
        CHECK_EQ_INT(status, NEPDATE_OK);
        CHECK(written <= 32);
        text_builder builder = {{0}, 0, false};
        for (size_t i = 0; i < written; i++) {
            append_separator(&builder);
            append_range(&builder, pieces[i]);
        }
        CHECK_EQ_STR(list_text(&builder), fields[3]);
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 9);
}

TEST(golden_range_days) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/range-days.tsv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    int rows = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[7];
        CHECK_EQ_INT(vector_split(line, '\t', fields, 7), 6);
        nepdate_range range = {{0}, {0}};
        CHECK(optional_range(fields[0], fields[1], &range));
        const char *operation = fields[2];
        const char *expected = fields[4];
        text_builder builder = {{0}, 0, false};

        if (strcmp(operation, "enumerate") == 0) {
            nepdate_range_iterator iterator = {0, 0};
            CHECK_EQ_INT(nepdate_range_begin(range, &iterator), NEPDATE_OK);
            nepdate_date date = {0};
            while (nepdate_range_next(&iterator, &date)) {
                append_separator(&builder);
                append_date(&builder, date);
            }
        } else {
            nepdate_range_day_cursor cursor = {0, 0, 0, 0};
            nepdate_status status = NEPDATE_OK;
            if (strcmp(operation, "dates_with_interval") == 0) {
                status =
                    nepdate_range_dates_with_interval(range, (int32_t)atoll(fields[3]), &cursor);
            } else if (strcmp(operation, "working_days") == 0) {
                bool exclude_sunday = false;
                CHECK(parse_bool(fields[3], &exclude_sunday));
                status = nepdate_range_working_days(range, exclude_sunday, &cursor);
            } else if (strcmp(operation, "weekend_days") == 0) {
                bool include_sunday = false;
                CHECK(parse_bool(fields[3], &include_sunday));
                status = nepdate_range_weekend_days(range, include_sunday, &cursor);
            } else {
                FAIL("unknown range day operation");
                continue;
            }
            if (result_is(expected, "ERR")) {
                CHECK(status != NEPDATE_OK);
                rows++;
                continue;
            }
            CHECK_EQ_INT(status, NEPDATE_OK);
            nepdate_date date = {0};
            while (nepdate_range_day_next(&cursor, &date)) {
                append_separator(&builder);
                append_date(&builder, date);
            }
        }
        CHECK_EQ_STR(list_text(&builder), expected);
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 18);
}

TEST(golden_range_format) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/range-format.tsv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    int rows = 0;
    int rejected = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[9];
        CHECK_EQ_INT(vector_split(line, '\t', fields, 9), 8);
        nepdate_range range = {{0}, {0}};
        const bool range_ok = optional_range(fields[0], fields[1], &range);
        char text[64];
        size_t length = 0;
        if (!range_ok) {
            /* D-01: the C# empty range cannot be built here, so the row is an error. */
            CHECK(result_is(fields[6], "ERR"));
            CHECK_EQ_INT(nepdate_range_format(range, text, sizeof text), 0);
            rejected++;
            rows++;
            continue;
        }
        if (strcmp(fields[2], "default") == 0) {
            length = nepdate_range_format(range, text, sizeof text);
        } else if (strcmp(fields[2], "short") == 0) {
            const struct {
                const char *name;
                nepdate_order order;
            } orders[] = {{"Ymd", NEPDATE_YMD}, {"Ydm", NEPDATE_YDM}, {"Myd", NEPDATE_MYD},
                          {"Mdy", NEPDATE_MDY}, {"Dym", NEPDATE_DYM}, {"Dmy", NEPDATE_DMY}};
            const struct {
                const char *name;
                nepdate_separator separator;
            } separators[] = {{"Slash", NEPDATE_SEP_SLASH}, {"Backslash", NEPDATE_SEP_BACKSLASH},
                              {"Dot", NEPDATE_SEP_DOT},     {"Underscore", NEPDATE_SEP_UNDERSCORE},
                              {"Dash", NEPDATE_SEP_DASH},   {"Space", NEPDATE_SEP_SPACE}};
            nepdate_order order = NEPDATE_YMD;
            nepdate_separator separator = NEPDATE_SEP_SLASH;
            for (size_t i = 0; i < sizeof orders / sizeof orders[0]; i++) {
                if (strcmp(fields[3], orders[i].name) == 0) {
                    order = orders[i].order;
                }
            }
            for (size_t i = 0; i < sizeof separators / sizeof separators[0]; i++) {
                if (strcmp(fields[4], separators[i].name) == 0) {
                    separator = separators[i].separator;
                }
            }
            const nepdate_lang lang =
                strcmp(fields[5], "ne") == 0 ? NEPDATE_NEPALI : NEPDATE_ENGLISH;
            length = nepdate_range_format_short(range, order, separator, lang, text, sizeof text);
        } else {
            FAIL("unknown range format kind");
            continue;
        }
        CHECK_EQ_INT(length, strlen(fields[6]));
        CHECK_EQ_STR(text, fields[6]);
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 18);
    CHECK_EQ_INT(rejected, 1);
}

TEST(set_operation_buffers) {
    nepdate_range range = {{0}, {0}};
    nepdate_range other = {{0}, {0}};
    nepdate_date first = {0};
    nepdate_date last = {0};
    CHECK(vector_parse_date("2081-04-10", &first));
    CHECK(vector_parse_date("2081-04-20", &last));
    CHECK_EQ_INT(nepdate_range_make(first, last, &range), NEPDATE_OK);

    /* except writes zero, one or two ranges into a caller array of two. */
    nepdate_range pieces[2] = {{{0}, {0}}, {{0}, {0}}};
    size_t written = 99;
    CHECK(vector_parse_range("2081-04-10..2081-04-30", &other));
    CHECK_EQ_INT(nepdate_range_except(range, other, pieces, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 0);

    CHECK(vector_parse_range("2081-04-01..2081-04-30", &other));
    CHECK_EQ_INT(nepdate_range_except(range, other, pieces, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 0);

    CHECK(vector_parse_range("2081-04-13..2081-04-17", &other));
    CHECK_EQ_INT(nepdate_range_except(range, other, pieces, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 2);
    char text[40];
    vector_format_range(pieces[0], text, sizeof text);
    CHECK_EQ_STR(text, "2081-04-10..2081-04-12");
    vector_format_range(pieces[1], text, sizeof text);
    CHECK_EQ_STR(text, "2081-04-18..2081-04-20");

    CHECK_EQ_INT(nepdate_range_except(range, other, NULL, &written), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_except(range, other, pieces, NULL), NEPDATE_E_NULL);
    const nepdate_range forged = {{0}, {10}};
    CHECK_EQ_INT(nepdate_range_except(forged, other, pieces, &written), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_except(range, forged, pieces, &written), NEPDATE_E_INVALID_DATE);
}

TEST(split_buffers_and_the_data_end) {
    nepdate_range range = {{0}, {0}};
    nepdate_date first = {0};
    nepdate_date last = {0};
    CHECK(vector_parse_date("2199-11-15", &first));
    CHECK(vector_parse_date("2199-12-30", &last));
    CHECK_EQ_INT(nepdate_range_make(first, last, &range), NEPDATE_OK);

    /* The last piece ends at the last supported day; the call does not step past it (D-16). */
    nepdate_range pieces[8] = {{{0}, {0}}};
    size_t written = 0;
    CHECK_EQ_INT(nepdate_range_split_by_month(range, pieces, 8, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 2);
    char text[40];
    vector_format_range(pieces[1], text, sizeof text);
    CHECK_EQ_STR(text, "2199-12-01..2199-12-30");

    /* A capacity below the piece count truncates the output but still reports every piece. */
    written = 0;
    CHECK_EQ_INT(nepdate_range_split_by_month(range, pieces, 1, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 2);
    written = 0;
    CHECK_EQ_INT(nepdate_range_split_by_month(range, NULL, 0, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 2);

    CHECK_EQ_INT(nepdate_range_split_by_month(range, pieces, 8, NULL), NEPDATE_E_NULL);
    const nepdate_range forged = {{0}, {10}};
    CHECK_EQ_INT(nepdate_range_split_by_month(forged, pieces, 8, &written), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_split_by_fiscal_quarter(forged, pieces, 8, &written),
                 NEPDATE_E_INVALID_DATE);
}

TEST(day_cursor_boundaries) {
    nepdate_range range = {{0}, {0}};
    nepdate_date first = {0};
    nepdate_date last = {0};
    CHECK(vector_parse_date("2199-12-25", &first));
    CHECK(vector_parse_date("2199-12-30", &last));
    CHECK_EQ_INT(nepdate_range_make(first, last, &range), NEPDATE_OK);

    /* A step at the top of int32_t is accepted and stops before it would pass the end. */
    nepdate_range_day_cursor cursor = {0, 0, 0, 0};
    CHECK_EQ_INT(nepdate_range_dates_with_interval(range, INT32_MAX, &cursor), NEPDATE_OK);
    nepdate_date date = {0};
    CHECK(nepdate_range_day_next(&cursor, &date));
    CHECK_EQ_INT(date.value, first.value);
    CHECK(!nepdate_range_day_next(&cursor, &date));

    CHECK_EQ_INT(nepdate_range_dates_with_interval(range, 0, &cursor), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_dates_with_interval(range, -1, &cursor), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_dates_with_interval(range, INT32_MIN, &cursor),
                 NEPDATE_E_OUT_OF_RANGE);

    /* Every day help stops at the last supported day instead of failing. */
    CHECK_EQ_INT(nepdate_range_working_days(range, false, &cursor), NEPDATE_OK);
    int count = 0;
    while (nepdate_range_day_next(&cursor, &date)) {
        count++;
    }
    CHECK_EQ_INT(count, 5);
    CHECK_EQ_INT(nepdate_range_weekend_days(range, true, &cursor), NEPDATE_OK);
    count = 0;
    while (nepdate_range_day_next(&cursor, &date)) {
        count++;
    }
    CHECK_EQ_INT(count, 2);

    CHECK_EQ_INT(nepdate_range_working_days(range, false, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_weekend_days(range, false, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_dates_with_interval(range, 1, NULL), NEPDATE_E_NULL);
    const nepdate_range forged = {{0}, {10}};
    CHECK_EQ_INT(nepdate_range_working_days(forged, false, &cursor), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_weekend_days(forged, false, &cursor), NEPDATE_E_INVALID_DATE);

    /* A forged cursor yields nothing and leaves the out object alone. */
    nepdate_range_day_cursor zero = {0, 0, 1, 0};
    nepdate_date unchanged = {7};
    CHECK(!nepdate_range_day_next(&zero, &unchanged));
    CHECK_EQ_INT(unchanged.value, 7);
    nepdate_range_day_cursor reversed = {10, 5, 1, 0};
    CHECK(!nepdate_range_day_next(&reversed, &unchanged));
    nepdate_range_day_cursor negative = {INT32_MIN, 5, 1, 0};
    CHECK(!nepdate_range_day_next(&negative, &unchanged));
    nepdate_range_day_cursor beyond = {109213, 109212, 1, 0};
    CHECK(!nepdate_range_day_next(&beyond, &unchanged));
    nepdate_range_day_cursor no_step = {10, 109212, 0, 0};
    CHECK(!nepdate_range_day_next(&no_step, &unchanged));
    CHECK(!nepdate_range_day_next(NULL, &unchanged));
    CHECK(!nepdate_range_day_next(&zero, NULL));
}

TEST(adjacency_at_the_data_end) {
    nepdate_range before = {{0}, {0}};
    nepdate_range at_end = {{0}, {0}};
    nepdate_date first = {0};
    nepdate_date last = {0};
    CHECK(vector_parse_date("2199-12-25", &first));
    CHECK(vector_parse_date("2199-12-30", &last));
    CHECK_EQ_INT(nepdate_range_make(first, last, &at_end), NEPDATE_OK);
    CHECK(vector_parse_date("2081-04-10", &first));
    CHECK(vector_parse_date("2081-04-20", &last));
    CHECK_EQ_INT(nepdate_range_make(first, last, &before), NEPDATE_OK);

    bool adjacent = true;
    bool overlaps = true;
    nepdate_range intersection = {{0}, {0}};
    /* No day follows the data end, so nothing can start the day after a range that ends there. */
    CHECK_EQ_INT(nepdate_range_is_adjacent_to(at_end, before, &adjacent), NEPDATE_OK);
    CHECK(!adjacent);
    CHECK_EQ_INT(nepdate_range_is_adjacent_to(before, at_end, &adjacent), NEPDATE_OK);
    CHECK(!adjacent);
    CHECK_EQ_INT(nepdate_range_overlaps(at_end, before, &overlaps), NEPDATE_OK);
    CHECK(!overlaps);
    CHECK_EQ_INT(nepdate_range_intersect(at_end, before, &intersection), NEPDATE_E_NO_OVERLAP);
    CHECK_EQ_INT(intersection.start.value, 0);

    /* The first supported day starts the span, so nothing can end the day before it either. */
    nepdate_range at_start = {{0}, {0}};
    CHECK(vector_parse_date("1901-01-01", &first));
    CHECK(vector_parse_date("1901-01-05", &last));
    CHECK_EQ_INT(nepdate_range_make(first, last, &at_start), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_range_is_adjacent_to(at_start, before, &adjacent), NEPDATE_OK);
    CHECK(!adjacent);
    CHECK_EQ_INT(nepdate_range_intersect(at_start, before, &intersection), NEPDATE_E_NO_OVERLAP);

    /* A pair that does touch is adjacent in exactly one direction. */
    nepdate_range next_day = {{0}, {0}};
    CHECK(vector_parse_date("2081-04-21", &first));
    CHECK(vector_parse_date("2081-04-25", &last));
    CHECK_EQ_INT(nepdate_range_make(first, last, &next_day), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_range_is_adjacent_to(before, next_day, &adjacent), NEPDATE_OK);
    CHECK(adjacent);
    CHECK_EQ_INT(nepdate_range_is_adjacent_to(next_day, before, &adjacent), NEPDATE_OK);
    CHECK(adjacent);
}

TEST(range_errors_and_identity) {
    const nepdate_date valid = {1};
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_make(valid, valid, &range), NEPDATE_OK);

    bool actual = true;
    CHECK_EQ_INT(nepdate_range_equal(range, range, &actual), NEPDATE_OK);
    CHECK(actual);
    CHECK_EQ_INT(nepdate_range_equal(range, range, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_contains_range(range, range, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_overlaps(range, range, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_is_adjacent_to(range, range, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_union(range, range, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_intersect(range, range, NULL), NEPDATE_E_NULL);

    const nepdate_range forged = {{0}, {10}};
    nepdate_range out = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_intersect(forged, range, &out), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_union(range, forged, &out), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_contains_range(forged, range, &actual), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_overlaps(forged, range, &actual), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_contains_range(range, forged, &actual), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_overlaps(range, forged, &actual), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_is_adjacent_to(range, forged, &actual), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_equal(forged, range, &actual), NEPDATE_E_INVALID_DATE);

    /* A reversed range is not a range this library made. */
    const nepdate_range reversed = {{20}, {10}};
    CHECK_EQ_INT(nepdate_range_intersect(reversed, range, &out), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_union(reversed, range, &out), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_is_adjacent_to(reversed, range, &actual), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_equal(reversed, range, &actual), NEPDATE_E_INVALID_DATE);
}

TEST(from_day_count_and_current_factories) {
    nepdate_date start = {0};
    CHECK(vector_parse_date("2081-04-15", &start));
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_from_day_count(start, 5, &range), NEPDATE_OK);
    char text[40];
    vector_format_range(range, text, sizeof text);
    CHECK_EQ_STR(text, "2081-04-15..2081-04-19");
    CHECK_EQ_INT(nepdate_range_from_day_count(start, 0, &range), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_from_day_count(start, -1, &range), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_from_day_count(start, INT32_MAX, &range), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_from_day_count(start, INT32_MIN, &range), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_from_day_count(start, 5, NULL), NEPDATE_E_NULL);
    const nepdate_date forged = {0};
    CHECK_EQ_INT(nepdate_range_from_day_count(forged, 5, &range), NEPDATE_E_INVALID_DATE);

    /* The current factories read the day from the instant and offset they are given, so the test
     * never depends on the machine clock. */
    CHECK_EQ_INT(nepdate_range_current_month(345, INT64_C(1722364200), &range), NEPDATE_OK);
    vector_format_range(range, text, sizeof text);
    CHECK_EQ_STR(text, "2081-04-01..2081-04-32");
    CHECK_EQ_INT(nepdate_range_current_fiscal_year(345, INT64_C(1722364200), &range), NEPDATE_OK);
    vector_format_range(range, text, sizeof text);
    CHECK_EQ_STR(text, "2081-04-01..2082-03-32");
    CHECK_EQ_INT(nepdate_range_current_calendar_year(345, INT64_C(1722364200), &range), NEPDATE_OK);
    vector_format_range(range, text, sizeof text);
    CHECK_EQ_STR(text, "2081-01-01..2081-12-31");
    CHECK_EQ_INT(nepdate_range_current_month(0, INT64_C(1722364200), &range), NEPDATE_OK);
    vector_format_range(range, text, sizeof text);
    CHECK_EQ_STR(text, "2081-04-01..2081-04-32");

    CHECK_EQ_INT(nepdate_range_current_month(345, INT64_C(1722364200), NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_current_fiscal_year(345, INT64_C(1722364200), NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_current_calendar_year(345, INT64_C(1722364200), NULL),
                 NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_current_month(0, INT64_MIN, &range), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_current_fiscal_year(0, INT64_MAX, &range), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_current_calendar_year(0, INT64_MAX, &range), NEPDATE_E_OUT_OF_RANGE);
}

TEST(range_format_buffers) {
    nepdate_range range = {{0}, {0}};
    nepdate_date first = {0};
    nepdate_date last = {0};
    CHECK(vector_parse_date("2081-04-10", &first));
    CHECK(vector_parse_date("2081-04-20", &last));
    CHECK_EQ_INT(nepdate_range_make(first, last, &range), NEPDATE_OK);

    char buffer[32];
    memset(buffer, 0x5A, sizeof buffer);
    CHECK_EQ_INT(nepdate_range_format(range, buffer, 0), 23);
    CHECK_EQ_INT((unsigned char)buffer[0], 0x5A);
    CHECK_EQ_INT(nepdate_range_format(range, buffer, 10), 23);
    CHECK_EQ_INT(memcmp(buffer, "2081/04/1", 9), 0);
    CHECK_EQ_INT((unsigned char)buffer[9], 0);
    CHECK_EQ_INT(nepdate_range_format(range, NULL, 8), 23);
    CHECK_EQ_INT(nepdate_range_format(range, buffer, sizeof buffer), 23);
    CHECK_EQ_STR(buffer, "2081/04/10 - 2081/04/20");

    memset(buffer, 0x5A, sizeof buffer);
    CHECK_EQ_INT(nepdate_range_format_short(range, NEPDATE_DMY, NEPDATE_SEP_DASH, NEPDATE_ENGLISH,
                                            buffer, sizeof buffer),
                 23);
    CHECK_EQ_STR(buffer, "10-04-2081 - 20-04-2081");

    const nepdate_range forged = {{0}, {10}};
    memset(buffer, 0x5A, sizeof buffer);
    CHECK_EQ_INT(nepdate_range_format(forged, buffer, sizeof buffer), 0);
    CHECK_EQ_INT((unsigned char)buffer[0], 0x5A);
    CHECK_EQ_INT(nepdate_range_format_short(forged, NEPDATE_YMD, NEPDATE_SEP_SLASH, NEPDATE_ENGLISH,
                                            buffer, sizeof buffer),
                 0);
}

int main(void) {
    RUN_TEST(golden_range_rows);
    RUN_TEST(golden_range_factories);
    RUN_TEST(golden_range_splits);
    RUN_TEST(golden_range_days);
    RUN_TEST(golden_range_format);
    RUN_TEST(set_operation_buffers);
    RUN_TEST(split_buffers_and_the_data_end);
    RUN_TEST(day_cursor_boundaries);
    RUN_TEST(adjacency_at_the_data_end);
    RUN_TEST(range_errors_and_identity);
    RUN_TEST(from_day_count_and_current_factories);
    RUN_TEST(range_format_buffers);
    return harness_finish();
}
