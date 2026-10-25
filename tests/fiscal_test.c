/* Fiscal years, quarters and their labels (ALGORITHM section 8b), against the C# golden dates. */
#include "harness.h"
#include "vectors.h"

#include <nepdate.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static nepdate_date date_of(int year, int month, int day) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_bs(year, month, day, &date), NEPDATE_OK);
    return date;
}

static void check_date(int year, int month, int day, nepdate_status status, nepdate_date actual) {
    CHECK_EQ_INT(status, NEPDATE_OK);
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(actual, &ymd), NEPDATE_OK);
    CHECK_EQ_INT(ymd.year, year);
    CHECK_EQ_INT(ymd.month, month);
    CHECK_EQ_INT(ymd.day, day);
}

static void check_expected(const char *expected, nepdate_status status, nepdate_date actual) {
    if (strcmp(expected, "ERR") == 0) {
        CHECK_EQ_INT(status, NEPDATE_E_OUT_OF_RANGE);
        return;
    }
    int year = 0;
    int month = 0;
    int day = 0;
    CHECK(vector_parse_ymd(expected, &year, &month, &day));
    check_date(year, month, day, status, actual);
}

/* A row of dates.tsv: the fiscal columns are fields 12 to 15. */
TEST(golden_fiscal_columns) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/dates.tsv"));
    char *header = vector_next(&reader);
    CHECK(header != NULL);
    char *header_fields[16];
    CHECK_EQ_INT(vector_split(header, '\t', header_fields, 16), 16);
    int rows = 0;
    int fy_start_errors = 0;
    int fy_end_errors = 0;
    for (char *line = vector_next(&reader); line != NULL; line = vector_next(&reader)) {
        char *fields[16];
        CHECK_EQ_INT(vector_split(line, '\t', fields, 16), 16);
        int year = 0;
        int month = 0;
        int day = 0;
        CHECK(vector_parse_ymd(fields[0], &year, &month, &day));
        const nepdate_date date = date_of(year, month, day);

        int fiscal_year = 0;
        CHECK_EQ_INT(nepdate_fiscal_year_of(date, &fiscal_year), NEPDATE_OK);

        nepdate_date start = {0};
        nepdate_status status = nepdate_fiscal_year_start(fiscal_year, &start);
        check_expected(fields[12], status, start);
        if (status != NEPDATE_OK) {
            fy_start_errors++;
        }

        nepdate_date end = {0};
        status = nepdate_fiscal_year_end(fiscal_year, &end);
        check_expected(fields[13], status, end);
        if (status != NEPDATE_OK) {
            fy_end_errors++;
        }

        nepdate_quarter quarter = NEPDATE_Q1;
        CHECK_EQ_INT(nepdate_quarter_of(date, &quarter), NEPDATE_OK);
        nepdate_date quarter_start = {0};
        status = nepdate_quarter_start(fiscal_year, quarter, &quarter_start);
        check_expected(fields[14], status, quarter_start);

        nepdate_date quarter_end = {0};
        status = nepdate_quarter_end(fiscal_year, quarter, &quarter_end);
        check_expected(fields[15], status, quarter_end);
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 1018);
    CHECK_EQ_INT(fy_start_errors, 10);
    CHECK_EQ_INT(fy_end_errors, 28);
}

TEST(fiscal_year_boundaries) {
    int fiscal_year = 0;
    nepdate_date date = {0};

    CHECK_EQ_INT(nepdate_fiscal_year_of(date_of(2081, 3, 31), &fiscal_year), NEPDATE_OK);
    CHECK_EQ_INT(fiscal_year, 2080);
    CHECK_EQ_INT(nepdate_fiscal_year_of(date_of(2081, 4, 1), &fiscal_year), NEPDATE_OK);
    CHECK_EQ_INT(fiscal_year, 2081);
    CHECK_EQ_INT(nepdate_fiscal_year_of(date_of(1901, 1, 1), &fiscal_year), NEPDATE_OK);
    CHECK_EQ_INT(fiscal_year, 1900);
    CHECK_EQ_INT(nepdate_fiscal_year_of(date_of(2199, 12, 30), &fiscal_year), NEPDATE_OK);
    CHECK_EQ_INT(fiscal_year, 2199);

    nepdate_status status = nepdate_fiscal_year_start(2080, &date);
    check_date(2080, 4, 1, status, date);
    status = nepdate_fiscal_year_end(2080, &date);
    check_date(2081, 3, 31, status, date);
    status = nepdate_quarter_start(1900, NEPDATE_Q4, &date);
    check_date(1901, 1, 1, status, date);
    status = nepdate_quarter_end(1900, NEPDATE_Q4, &date);
    check_date(1901, 3, 32, status, date);
    status = nepdate_quarter_end(2199, NEPDATE_Q3, &date);
    check_date(2199, 12, 30, status, date);

    CHECK_EQ_INT(nepdate_fiscal_year_start(1900, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_fiscal_year_start(1843, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_fiscal_year_start(2200, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_fiscal_year_end(2199, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_fiscal_year_end(2200, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_fiscal_year_end(INT32_MIN, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_fiscal_year_end(INT32_MAX, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_fiscal_year_start(INT32_MIN, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_fiscal_year_start(INT32_MAX, &date), NEPDATE_E_OUT_OF_RANGE);
}

TEST(quarter_of_every_month) {
    const nepdate_quarter expected[] = {
        NEPDATE_Q4, NEPDATE_Q4, NEPDATE_Q4, NEPDATE_Q1, NEPDATE_Q1, NEPDATE_Q1,
        NEPDATE_Q2, NEPDATE_Q2, NEPDATE_Q2, NEPDATE_Q3, NEPDATE_Q3, NEPDATE_Q3,
    };
    nepdate_quarter quarter = NEPDATE_Q1;
    for (int month = 1; month <= 12; month++) {
        CHECK_EQ_INT(nepdate_quarter_of(date_of(2081, month, 1), &quarter), NEPDATE_OK);
        CHECK_EQ_INT(quarter, expected[month - 1]);
    }
}

TEST(quarter_boundaries) {
    nepdate_date date = {0};
    nepdate_status status = nepdate_quarter_start(2080, NEPDATE_Q1, &date);
    check_date(2080, 4, 1, status, date);
    status = nepdate_quarter_end(2080, NEPDATE_Q1, &date);
    check_date(2080, 6, 30, status, date);
    status = nepdate_quarter_start(2080, NEPDATE_Q2, &date);
    check_date(2080, 7, 1, status, date);
    status = nepdate_quarter_end(2080, NEPDATE_Q2, &date);
    check_date(2080, 9, 29, status, date);
    status = nepdate_quarter_start(2080, NEPDATE_Q3, &date);
    check_date(2080, 10, 1, status, date);
    status = nepdate_quarter_end(2080, NEPDATE_Q3, &date);
    check_date(2080, 12, 30, status, date);
    status = nepdate_quarter_start(2080, NEPDATE_Q4, &date);
    check_date(2081, 1, 1, status, date);
    status = nepdate_quarter_end(2080, NEPDATE_Q4, &date);
    check_date(2081, 3, 31, status, date);

    CHECK_EQ_INT(nepdate_quarter_start(1900, NEPDATE_Q1, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_quarter_end(1900, NEPDATE_Q1, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_quarter_start(2199, NEPDATE_Q4, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_quarter_end(2199, NEPDATE_Q4, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_quarter_end(2200, NEPDATE_Q1, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_quarter_start(INT32_MAX, NEPDATE_Q4, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_quarter_end(INT32_MAX, NEPDATE_Q4, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_quarter_start(INT32_MIN, NEPDATE_Q1, &date), NEPDATE_E_OUT_OF_RANGE);

    /* NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange): testing an unknown quarter */
    const nepdate_quarter invalid = (nepdate_quarter)0;
    CHECK_EQ_INT(nepdate_quarter_start(2080, invalid, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_quarter_end(2080, invalid, &date), NEPDATE_E_OUT_OF_RANGE);
    /* NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange): testing an unknown quarter */
    const nepdate_quarter too_large = (nepdate_quarter)5;
    CHECK_EQ_INT(nepdate_quarter_start(2080, too_large, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_quarter_end(2080, too_large, &date), NEPDATE_E_OUT_OF_RANGE);
}

static void check_label(int fiscal_year, nepdate_lang lang, const char *expected) {
    char buffer[64];
    memset(buffer, 'x', sizeof buffer);
    const size_t length = nepdate_fiscal_year_label(fiscal_year, lang, buffer, sizeof buffer);
    CHECK_EQ_INT(length, strlen(expected));
    CHECK_EQ_STR(buffer, expected);
    CHECK(buffer[length] == '\0');
}

TEST(fiscal_labels) {
    check_label(2082, NEPDATE_ENGLISH, "2082/83");
    check_label(2082, NEPDATE_NEPALI, "२०८२/८३");
    check_label(2099, NEPDATE_ENGLISH, "2099/00");
    check_label(2100, NEPDATE_ENGLISH, "2100/01");
    check_label(1900, NEPDATE_NEPALI, "१९००/०१");
    check_label(0, NEPDATE_ENGLISH, "0/01");
    check_label(-1, NEPDATE_ENGLISH, "-1/00");
    check_label(INT32_MIN, NEPDATE_ENGLISH, "-2147483648/47");
    check_label(INT32_MAX, NEPDATE_ENGLISH, "2147483647/48");
}

TEST(fiscal_label_truncation) {
    const char *expected = "2082/83";
    const size_t length = strlen(expected);
    for (size_t size = 0; size <= length + 1; size++) {
        char buffer[32];
        memset(buffer, 'x', sizeof buffer);
        const size_t needed = nepdate_fiscal_year_label(2082, NEPDATE_ENGLISH, buffer, size);
        CHECK_EQ_INT(needed, length);
        if (size == 0) {
            CHECK(buffer[0] == 'x');
        } else {
            const size_t written = size - 1 < length ? size - 1 : length;
            CHECK(memcmp(buffer, expected, written) == 0);
            CHECK(buffer[written] == '\0');
        }
    }

    char buffer[32];
    memset(buffer, 'x', sizeof buffer);
    CHECK_EQ_INT(nepdate_fiscal_year_label(2082, NEPDATE_ENGLISH, NULL, 0), length);
    CHECK_EQ_INT(nepdate_fiscal_year_label(2082, NEPDATE_ENGLISH, NULL, sizeof buffer), length);
    CHECK(buffer[0] == 'x');

    /* An unknown language is English, not undefined behaviour. */
    /* NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange): testing an unknown language */
    const nepdate_lang other = (nepdate_lang)7;
    CHECK_EQ_INT(nepdate_fiscal_year_label(2082, other, buffer, sizeof buffer), length);
    CHECK_EQ_STR(buffer, expected);
}

int main(void) {
    RUN_TEST(golden_fiscal_columns);
    RUN_TEST(fiscal_year_boundaries);
    RUN_TEST(quarter_of_every_month);
    RUN_TEST(quarter_boundaries);
    RUN_TEST(fiscal_labels);
    RUN_TEST(fiscal_label_truncation);
    return harness_finish();
}
