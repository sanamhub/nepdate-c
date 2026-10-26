/* Formatting: the golden name columns and every pattern row, with snprintf-like buffers. */
#include "harness.h"
#include "vectors.h"

#include <nepdate.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SHARED_DIR NEPDATE_C_SHARED_DIR

static nepdate_date date_of(int year, int month, int day) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_bs(year, month, day, &date), NEPDATE_OK);
    return date;
}

/* Values outside an enum's enumerators, built without an out-of-range cast (the analyzers reject
 * one) so the documented fallback is still tested. */
static nepdate_order order_from_int(int value) {
    nepdate_order order = NEPDATE_YMD;
    memcpy(&order, &value, sizeof order);
    return order;
}

static nepdate_separator separator_from_int(int value) {
    nepdate_separator separator = NEPDATE_SEP_SLASH;
    memcpy(&separator, &value, sizeof separator);
    return separator;
}

/* Formats into a buffer large enough for every test string. */
static size_t format_text(nepdate_date date, const char *pattern, nepdate_lang lang, char *buffer,
                          size_t size) {
    return nepdate_format(date, pattern, lang, buffer, size);
}

/* Prints the text of a format into a buffer and fails when it doesn't equal expected. */
static void check_format(nepdate_date date, const char *pattern, nepdate_lang lang,
                         const char *expected) {
    char buffer[1024];
    const size_t length = format_text(date, pattern, lang, buffer, sizeof buffer);
    CHECK_EQ_INT(length, strlen(expected));
    CHECK_EQ_STR(buffer, expected);
}

static void check_short(nepdate_date date, nepdate_order order, nepdate_separator separator,
                        bool pad, nepdate_lang lang, const char *expected) {
    char buffer[128];
    const size_t length =
        nepdate_format_short(date, order, separator, pad, lang, buffer, sizeof buffer);
    CHECK_EQ_INT(length, strlen(expected));
    CHECK_EQ_STR(buffer, expected);
}

static void check_long(nepdate_date date, nepdate_long_options options, nepdate_lang lang,
                       const char *expected) {
    char buffer[256];
    const size_t length = nepdate_format_long(date, options, lang, buffer, sizeof buffer);
    CHECK_EQ_INT(length, strlen(expected));
    CHECK_EQ_STR(buffer, expected);
}

/* True when pattern has a run of exactly three M outside a quoted literal. */
static bool has_three_ms(const char *pattern) {
    size_t i = 0;
    while (pattern[i] != '\0') {
        if (pattern[i] == '\'') {
            i++;
            while (pattern[i] != '\0' && pattern[i] != '\'') {
                i++;
            }
            if (pattern[i] == '\'') {
                i++;
            }
        } else if (pattern[i] == 'M') {
            size_t end = i;
            while (pattern[end] == 'M') {
                end++;
            }
            if (end - i == 3U) {
                return true;
            }
            i = end;
        } else {
            i++;
        }
    }
    return false;
}

TEST(golden_dates_names) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/dates.tsv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    int rows = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[16];
        const int count = vector_split(line, '\t', fields, 16);
        CHECK_EQ_INT(count, 16);
        int year = 0;
        int month = 0;
        int day = 0;
        CHECK(vector_parse_ymd(fields[0], &year, &month, &day));
        const nepdate_date date = date_of(year, month, day);

        check_format(date, "", NEPDATE_ENGLISH, fields[5]);
        check_format(date, "d", NEPDATE_ENGLISH, fields[5]);
        check_short(date, NEPDATE_YMD, NEPDATE_SEP_SLASH, true, NEPDATE_NEPALI, fields[6]);
        const nepdate_long_options plain = {false, true, true};
        const nepdate_long_options weekday = {true, true, true};
        check_long(date, plain, NEPDATE_ENGLISH, fields[7]);
        check_long(date, weekday, NEPDATE_ENGLISH, fields[8]);
        check_long(date, plain, NEPDATE_NEPALI, fields[9]);
        check_long(date, weekday, NEPDATE_NEPALI, fields[10]);
        check_short(date, NEPDATE_DMY, NEPDATE_SEP_DASH, false, NEPDATE_ENGLISH, fields[11]);
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 1018);
}

/* Formats pattern into buffers of every size from 0 to its full length and checks the truncation.
 */
static size_t sweep_pattern(const nepdate_date date, const char *pattern, const char *expected) {
    const size_t full = strlen(expected);
    size_t checks = 0;
    char buffer[1024];
    for (size_t size = 0; size <= full; size++) {
        memset(buffer, 0xA5, sizeof buffer);
        const size_t length = format_text(date, pattern, NEPDATE_ENGLISH, buffer, size);
        CHECK_EQ_INT(length, full);
        if (size == 0) {
            CHECK_EQ_INT((unsigned char)buffer[0], 0xA5);
        } else {
            const size_t kept = size - 1U < full ? size - 1U : full;
            CHECK_EQ_INT(memcmp(buffer, expected, kept), 0);
            CHECK_EQ_INT((unsigned char)buffer[kept], 0);
        }
        checks++;
    }
    CHECK_EQ_INT(format_text(date, pattern, NEPDATE_ENGLISH, NULL, 0), full);
    CHECK_EQ_INT(format_text(date, pattern, NEPDATE_ENGLISH, NULL, 37), full);
    return checks;
}

TEST(golden_patterns) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/format-pattern.tsv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    int rows = 0;
    int skipped = 0;
    size_t sweeps = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[4];
        const int count = vector_split(line, '\t', fields, 4);
        CHECK_EQ_INT(count, 3);
        int year = 0;
        int month = 0;
        int day = 0;
        CHECK(vector_parse_ymd(fields[0], &year, &month, &day));
        if ((month == 3 || month == 6) && has_three_ms(fields[1])) {
            /* D-04: the C# abbreviation is "Ash" for Ashad and Ashoj; this port uses "Asa" and
             * "Aso". */
            skipped++;
            continue;
        }
        const nepdate_date date = date_of(year, month, day);
        check_format(date, fields[1], NEPDATE_ENGLISH, fields[2]);
        sweeps += sweep_pattern(date, fields[1], fields[2]);
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 444);
    CHECK_EQ_INT(skipped, 7);
    CHECK_EQ_INT(sweeps, 4540);
    (void)printf("format-pattern sweep: %d rows, %zu buffer sizes checked\n", rows, sweeps);
}

TEST(short_format_examples) {
    const nepdate_date date = date_of(2080, 5, 15);
    check_short(date, NEPDATE_YMD, NEPDATE_SEP_DASH, true, NEPDATE_ENGLISH, "2080-05-15");
    check_short(date, NEPDATE_DMY, NEPDATE_SEP_DASH, false, NEPDATE_ENGLISH, "15-5-2080");
    check_short(date, NEPDATE_YMD, NEPDATE_SEP_SLASH, true, NEPDATE_NEPALI, "२०८०/०५/१५");
    check_short(date, NEPDATE_YDM, NEPDATE_SEP_DOT, true, NEPDATE_ENGLISH, "2080.15.05");
    check_short(date, NEPDATE_MYD, NEPDATE_SEP_SPACE, true, NEPDATE_ENGLISH, "05 2080 15");
    check_short(date, NEPDATE_MDY, NEPDATE_SEP_UNDERSCORE, true, NEPDATE_ENGLISH, "05_15_2080");
    check_short(date, NEPDATE_DYM, NEPDATE_SEP_BACKSLASH, true, NEPDATE_ENGLISH, "15\\2080\\05");
    check_short(date, NEPDATE_YMD, NEPDATE_SEP_SLASH, false, NEPDATE_ENGLISH, "2080/5/15");
    check_short(date, order_from_int(99), separator_from_int('?'), true, NEPDATE_ENGLISH,
                "2080/05/15");
}

TEST(long_format_examples) {
    const nepdate_long_options plain = {false, true, true};
    const nepdate_long_options weekday = {true, true, true};
    const nepdate_long_options short_year = {true, false, false};
    check_long(date_of(2081, 4, 15), plain, NEPDATE_ENGLISH, "Shrawan 15, 2081");
    check_long(date_of(2081, 4, 15), weekday, NEPDATE_ENGLISH, "Tuesday, Shrawan 15, 2081");
    check_long(date_of(1901, 1, 1), plain, NEPDATE_ENGLISH, "Baishakh 01, 1901");
    check_long(date_of(1901, 1, 1), plain, NEPDATE_NEPALI, "बैशाख ०१, १९०१");
    check_long(date_of(2079, 2, 6), short_year, NEPDATE_ENGLISH, "Friday, Jestha 6");
}

TEST(pattern_tokens) {
    const nepdate_date date = date_of(2081, 4, 15);
    check_format(date, "yyyy-MM-dd", NEPDATE_ENGLISH, "2081-04-15");
    check_format(date, "M/d/yyyy", NEPDATE_ENGLISH, "4/15/2081");
    check_format(date, "yyyy'BS'", NEPDATE_ENGLISH, "2081BS");
    check_format(date, "yyyy\\M\\M-\\d\\d", NEPDATE_ENGLISH, "2081MM-dd");
    check_format(date, "MMM yyyy", NEPDATE_ENGLISH, "Shr 2081");
    check_format(date, "dd.MM.yy", NEPDATE_ENGLISH, "15.04.81");
    check_format(date, "ddd", NEPDATE_ENGLISH, "15");
    check_format(date, "yyy", NEPDATE_ENGLISH, "81");
    check_format(date, "MMMM", NEPDATE_ENGLISH, "Shrawan");
    check_format(date, "M", NEPDATE_ENGLISH, "4");
    check_format(date, "MM", NEPDATE_ENGLISH, "04");
    check_format(date, "dd", NEPDATE_ENGLISH, "15");
    check_format(date, "d", NEPDATE_ENGLISH, "2081/04/15");
    check_format(date, "D", NEPDATE_ENGLISH, "Shrawan 15, 2081");
    check_format(date, "s", NEPDATE_ENGLISH, "2081-04-15");
    check_format(date, "G", NEPDATE_ENGLISH, "2081/04/15");
    check_format(date, "g", NEPDATE_ENGLISH, "2081/04/15");
    check_format(date, "", NEPDATE_ENGLISH, "2081/04/15");
    check_format(date, NULL, NEPDATE_ENGLISH, "2081/04/15");
    check_format(date, "yyyy'BS'", NEPDATE_NEPALI, "२०८१BS");
    check_format(date, "MMM yyyy", NEPDATE_NEPALI, "साउन २०८१");
    check_format(date, "'unterminated", NEPDATE_ENGLISH, "unterminated");
    check_format(date, "'unterminated'x", NEPDATE_ENGLISH, "unterminatedx");
    check_format(date, "a\\", NEPDATE_ENGLISH, "a");
    check_format(date, "\\", NEPDATE_ENGLISH, "");
}

TEST(null_and_truncated_buffers) {
    const nepdate_date date = date_of(2081, 4, 15);
    char buffer[16];
    char sentinel[16];
    memset(sentinel, 0x5A, sizeof sentinel);

    memset(buffer, 0x5A, sizeof buffer);
    CHECK_EQ_INT(nepdate_format(date, "yyyy-MM-dd", NEPDATE_ENGLISH, buffer, 0), 10);
    CHECK_EQ_INT((unsigned char)buffer[0], 0x5A);

    memset(buffer, 0x5A, sizeof buffer);
    CHECK_EQ_INT(nepdate_format(date, "yyyy-MM-dd", NEPDATE_ENGLISH, buffer, 4), 10);
    CHECK_EQ_INT(memcmp(buffer, "208", 3), 0);
    CHECK_EQ_INT((unsigned char)buffer[3], 0);
    CHECK_EQ_INT((unsigned char)buffer[4], 0x5A);
    CHECK_EQ_INT(nepdate_format(date, "yyyy-MM-dd", NEPDATE_ENGLISH, NULL, 4), 10);
    CHECK_EQ_INT(
        nepdate_format_short(date, NEPDATE_YMD, NEPDATE_SEP_SLASH, true, NEPDATE_ENGLISH, NULL, 4),
        10);
    const nepdate_long_options options = {false, true, true};
    CHECK_EQ_INT(nepdate_format_long(date, options, NEPDATE_ENGLISH, NULL, 4), 16);
}

TEST(invalid_date_writes_nothing) {
    const nepdate_date invalid = {-1};
    const nepdate_long_options options = {false, true, true};
    char buffer[16];
    memset(buffer, 0x5A, sizeof buffer);
    CHECK_EQ_INT(nepdate_format(invalid, "yyyy-MM-dd", NEPDATE_ENGLISH, buffer, sizeof buffer), 0);
    CHECK_EQ_INT(nepdate_format_short(invalid, NEPDATE_YMD, NEPDATE_SEP_SLASH, true,
                                      NEPDATE_ENGLISH, buffer, sizeof buffer),
                 0);
    CHECK_EQ_INT(nepdate_format_long(invalid, options, NEPDATE_ENGLISH, buffer, sizeof buffer), 0);
    for (size_t i = 0; i < sizeof buffer; i++) {
        CHECK_EQ_INT((unsigned char)buffer[i], 0x5A);
    }
}

int main(void) {
    RUN_TEST(golden_dates_names);
    RUN_TEST(golden_patterns);
    RUN_TEST(short_format_examples);
    RUN_TEST(long_format_examples);
    RUN_TEST(pattern_tokens);
    RUN_TEST(null_and_truncated_buffers);
    RUN_TEST(invalid_date_writes_nothing);
    return harness_finish();
}
