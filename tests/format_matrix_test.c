/* The formatting matrix: every order, separator, padding and language, and the truncated-buffer
 * case of the span formatter. */
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

static nepdate_order order_of(const char *name) {
    const struct {
        const char *name;
        nepdate_order order;
    } orders[] = {{"Ymd", NEPDATE_YMD}, {"Ydm", NEPDATE_YDM}, {"Myd", NEPDATE_MYD},
                  {"Mdy", NEPDATE_MDY}, {"Dym", NEPDATE_DYM}, {"Dmy", NEPDATE_DMY}};
    for (size_t i = 0; i < sizeof orders / sizeof orders[0]; i++) {
        if (strcmp(name, orders[i].name) == 0) {
            return orders[i].order;
        }
    }
    CHECK(!"unknown order");
    return NEPDATE_YMD;
}

static nepdate_separator separator_of(const char *name) {
    const struct {
        const char *name;
        nepdate_separator separator;
    } separators[] = {{"Slash", NEPDATE_SEP_SLASH}, {"Backslash", NEPDATE_SEP_BACKSLASH},
                      {"Dot", NEPDATE_SEP_DOT},     {"Underscore", NEPDATE_SEP_UNDERSCORE},
                      {"Dash", NEPDATE_SEP_DASH},   {"Space", NEPDATE_SEP_SPACE}};
    for (size_t i = 0; i < sizeof separators / sizeof separators[0]; i++) {
        if (strcmp(name, separators[i].name) == 0) {
            return separators[i].separator;
        }
    }
    CHECK(!"unknown separator");
    return NEPDATE_SEP_SLASH;
}

TEST(format_matrix) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/format-matrix.tsv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    int short_rows = 0;
    int try_format_rows = 0;
    int too_small = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[11];
        CHECK_EQ_INT(vector_split(line, '\t', fields, 11), 10);
        int year = 0;
        int month = 0;
        int day = 0;
        CHECK(vector_parse_ymd(fields[1], &year, &month, &day));
        const nepdate_date date = date_of(year, month, day);
        const nepdate_lang lang = strcmp(fields[5], "ne") == 0 ? NEPDATE_NEPALI : NEPDATE_ENGLISH;

        if (strcmp(fields[0], "short") == 0) {
            const nepdate_order order = order_of(fields[2]);
            const nepdate_separator separator = separator_of(fields[3]);
            const bool pad = strcmp(fields[4], "true") == 0;
            char buffer[64];
            memset(buffer, 0x5A, sizeof buffer);
            const size_t length =
                nepdate_format_short(date, order, separator, pad, lang, buffer, sizeof buffer);
            CHECK_EQ_INT(length, strlen(fields[8]));
            CHECK_EQ_STR(buffer, fields[8]);
            /* Nothing beyond the terminator is touched. */
            CHECK_EQ_INT((unsigned char)buffer[length], 0);
            CHECK_EQ_INT((unsigned char)buffer[length + 1], 0x5A);
            short_rows++;
        } else if (strcmp(fields[0], "try_format") == 0) {
            const int capacity = atoi(fields[7]);
            CHECK(capacity > 0);
            const bool default_pattern = strcmp(fields[6], "default") == 0;
            char buffer[64];
            memset(buffer, 0x5A, sizeof buffer);
            /* The capacity is the number of characters the caller has room for, so the C buffer is
             * one byte larger than that and the text fits when its length is at most capacity. */
            const size_t length = nepdate_format(date, default_pattern ? NULL : fields[6], lang,
                                                 buffer, (size_t)capacity + 1);
            if (strcmp(fields[8], "ERR") == 0) {
                CHECK(length > (size_t)capacity);
                /* The text is cut at the capacity and still terminated there. */
                CHECK_EQ_INT((unsigned char)buffer[capacity], 0);
                CHECK_EQ_INT((unsigned char)buffer[capacity + 1], 0x5A);
                too_small++;
            } else {
                CHECK(length <= (size_t)capacity);
                CHECK_EQ_INT(length, strlen(fields[8]));
                CHECK_EQ_STR(buffer, fields[8]);
            }
            try_format_rows++;
        } else {
            CHECK(!"unknown format matrix kind");
        }
    }
    vector_close(&reader);
    CHECK_EQ_INT(short_rows, 94);
    CHECK_EQ_INT(try_format_rows, 8);
    CHECK_EQ_INT(too_small, 4);
}

TEST(every_order_and_separator) {
    const nepdate_date date = date_of(2081, 4, 15);
    /* Where the two separators sit in a ten-character text, per order. */
    const int positions[6][2] = {{4, 7}, {4, 7}, {2, 7}, {2, 5}, {2, 7}, {2, 5}};
    for (int order = NEPDATE_YMD; order <= NEPDATE_DMY; order++) {
        for (int separator = 0; separator < 6; separator++) {
            const char separators[6] = {'/', '\\', '.', '_', '-', ' '};
            char buffer[16];
            const size_t length = nepdate_format_short(
                date, (nepdate_order)order, (nepdate_separator)separators[separator], true,
                NEPDATE_ENGLISH, buffer, sizeof buffer);
            CHECK_EQ_INT(length, 10);
            CHECK_EQ_INT(strlen(buffer), 10);
            CHECK_EQ_INT(buffer[positions[order][0]], separators[separator]);
            CHECK_EQ_INT(buffer[positions[order][1]], separators[separator]);
        }
    }
    /* A value outside the enums falls back to year-month-day with a slash. */
    char buffer[16];
    CHECK_EQ_INT(nepdate_format_short(date, (nepdate_order)99, (nepdate_separator)'?', true,
                                      NEPDATE_ENGLISH, buffer, sizeof buffer),
                 10);
    CHECK_EQ_STR(buffer, "2081/04/15");
}

int main(void) {
    RUN_TEST(format_matrix);
    RUN_TEST(every_order_and_separator);
    return harness_finish();
}
