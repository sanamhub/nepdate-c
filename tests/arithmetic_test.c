/* Day, month and year arithmetic (ALGORITHM section 8) and the calendar breakdown (section 8a),
 * against the C# golden vectors and the examples in the spec. */
#include "harness.h"
#include "vectors.h"

#include <nepdate.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static nepdate_date date_of(int year, int month, int day) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_bs(year, month, day, &date), NEPDATE_OK);
    return date;
}

static void check_ymd(nepdate_status status, nepdate_ymd actual, int year, int month, int day) {
    CHECK_EQ_INT(status, NEPDATE_OK);
    CHECK_EQ_INT(actual.year, year);
    CHECK_EQ_INT(actual.month, month);
    CHECK_EQ_INT(actual.day, day);
}

static int parse_int(const char *text) {
    char *end = NULL;
    const long value = strtol(text, &end, 10);
    CHECK(end != text && *end == '\0');
    return (int)value;
}

/* A line of add-days.tsv: bs, days, result. */
TEST(golden_add_days) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/add-days.tsv"));
    CHECK_EQ_STR(vector_next(&reader), "bs\tdays\tresult");
    int rows = 0;
    for (char *line = vector_next(&reader); line != NULL; line = vector_next(&reader)) {
        char *fields[3];
        CHECK_EQ_INT(vector_split(line, '\t', fields, 3), 3);
        int year = 0;
        int month = 0;
        int day = 0;
        CHECK(vector_parse_ymd(fields[0], &year, &month, &day));
        const int days = parse_int(fields[1]);
        nepdate_date result = {0};
        const nepdate_status status = nepdate_add_days(date_of(year, month, day), days, &result);
        if (strcmp(fields[2], "ERR") == 0) {
            CHECK_EQ_INT(status, NEPDATE_E_OUT_OF_RANGE);
        } else {
            int ey = 0;
            int em = 0;
            int ed = 0;
            CHECK(vector_parse_ymd(fields[2], &ey, &em, &ed));
            nepdate_ymd actual = {0, 0, 0};
            CHECK_EQ_INT(status, NEPDATE_OK);
            CHECK_EQ_INT(nepdate_to_bs(result, &actual), NEPDATE_OK);
            check_ymd(NEPDATE_OK, actual, ey, em, ed);
        }
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 9162);
}

static void check_add_months_row(const char *bs, const char *months, const char *expected,
                                 nepdate_overflow overflow) {
    int year = 0;
    int month = 0;
    int day = 0;
    CHECK(vector_parse_ymd(bs, &year, &month, &day));
    nepdate_date result = {0};
    const nepdate_status status =
        nepdate_add_months(date_of(year, month, day), parse_int(months), overflow, &result);
    if (strcmp(expected, "ERR") == 0) {
        CHECK_EQ_INT(status, NEPDATE_E_OUT_OF_RANGE);
        return;
    }
    nepdate_ymd actual = {0, 0, 0};
    CHECK_EQ_INT(status, NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_bs(result, &actual), NEPDATE_OK);
    int ey = 0;
    int em = 0;
    int ed = 0;
    CHECK(vector_parse_ymd(expected, &ey, &em, &ed));
    check_ymd(NEPDATE_OK, actual, ey, em, ed);
}

/* A line of add-months.tsv: bs, months, clamp, overflow. */
TEST(golden_add_months) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/add-months.tsv"));
    CHECK_EQ_STR(vector_next(&reader), "bs\tmonths\tclamp\toverflow");
    int rows = 0;
    for (char *line = vector_next(&reader); line != NULL; line = vector_next(&reader)) {
        char *fields[4];
        CHECK_EQ_INT(vector_split(line, '\t', fields, 4), 4);
        check_add_months_row(fields[0], fields[1], fields[2], NEPDATE_CLAMP);
        check_add_months_row(fields[0], fields[1], fields[3], NEPDATE_OVERFLOW);
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 10180);
}

TEST(algorithm_8_examples) {
    const nepdate_date shrawan = date_of(2081, 4, 32);
    nepdate_date result = {0};
    nepdate_ymd ymd = {0, 0, 0};

    CHECK_EQ_INT(nepdate_add_months(shrawan, 2, NEPDATE_CLAMP, &result), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_bs(result, &ymd), NEPDATE_OK);
    check_ymd(NEPDATE_OK, ymd, 2081, 6, 30);
    CHECK_EQ_INT(nepdate_add_months(shrawan, 2, NEPDATE_OVERFLOW, &result), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_bs(result, &ymd), NEPDATE_OK);
    check_ymd(NEPDATE_OK, ymd, 2081, 7, 2);

    CHECK_EQ_INT(nepdate_add_months(shrawan, 5, NEPDATE_CLAMP, &result), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_bs(result, &ymd), NEPDATE_OK);
    check_ymd(NEPDATE_OK, ymd, 2081, 9, 29);
    CHECK_EQ_INT(nepdate_add_months(shrawan, 5, NEPDATE_OVERFLOW, &result), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_bs(result, &ymd), NEPDATE_OK);
    check_ymd(NEPDATE_OK, ymd, 2081, 10, 3);

    CHECK_EQ_INT(nepdate_add_days(date_of(2080, 12, 30), 5, &result), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_bs(result, &ymd), NEPDATE_OK);
    check_ymd(NEPDATE_OK, ymd, 2081, 1, 5);
    CHECK_EQ_INT(nepdate_add_days(date_of(2081, 1, 5), -5, &result), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_bs(result, &ymd), NEPDATE_OK);
    check_ymd(NEPDATE_OK, ymd, 2080, 12, 30);
}

TEST(add_years_uses_months) {
    const nepdate_date shrawan = date_of(2081, 4, 32);
    nepdate_date result = {0};
    nepdate_ymd ymd = {0, 0, 0};

    CHECK_EQ_INT(nepdate_add_years(shrawan, 1, NEPDATE_CLAMP, &result), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_bs(result, &ymd), NEPDATE_OK);
    check_ymd(NEPDATE_OK, ymd, 2082, 4, 31);
    CHECK_EQ_INT(nepdate_add_years(shrawan, 1, NEPDATE_OVERFLOW, &result), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_bs(result, &ymd), NEPDATE_OK);
    check_ymd(NEPDATE_OK, ymd, 2082, 5, 1);
    CHECK_EQ_INT(nepdate_add_years(shrawan, -1, NEPDATE_CLAMP, &result), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_bs(result, &ymd), NEPDATE_OK);
    check_ymd(NEPDATE_OK, ymd, 2080, 4, 32);
}

TEST(arithmetic_extremes) {
    const nepdate_date first = date_of(1901, 1, 1);
    const nepdate_date last = date_of(2199, 12, 30);
    nepdate_date result = {0};
    nepdate_ymd ymd = {0, 0, 0};

    CHECK_EQ_INT(nepdate_add_days(first, -1, &result), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_add_days(last, 1, &result), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_add_days(first, INT32_MIN, &result), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_add_days(first, INT32_MAX, &result), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_add_days(last, INT32_MIN, &result), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_add_days(last, INT32_MAX, &result), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_add_days(first, 109211, &result), NEPDATE_OK);
    CHECK_EQ_INT(result.value, last.value);
    CHECK_EQ_INT(nepdate_add_days(last, -109211, &result), NEPDATE_OK);
    CHECK_EQ_INT(result.value, first.value);

    for (int mode = 0; mode < 2; mode++) {
        const nepdate_overflow overflow = mode == 0 ? NEPDATE_CLAMP : NEPDATE_OVERFLOW;
        CHECK_EQ_INT(nepdate_add_months(first, -1, overflow, &result), NEPDATE_E_OUT_OF_RANGE);
        CHECK_EQ_INT(nepdate_add_months(last, 1, overflow, &result), NEPDATE_E_OUT_OF_RANGE);
        CHECK_EQ_INT(nepdate_add_months(first, INT32_MIN, overflow, &result),
                     NEPDATE_E_OUT_OF_RANGE);
        CHECK_EQ_INT(nepdate_add_months(last, INT32_MAX, overflow, &result),
                     NEPDATE_E_OUT_OF_RANGE);
        CHECK_EQ_INT(nepdate_add_months(last, INT32_MIN, overflow, &result),
                     NEPDATE_E_OUT_OF_RANGE);
        CHECK_EQ_INT(nepdate_add_months(first, INT32_MAX, overflow, &result),
                     NEPDATE_E_OUT_OF_RANGE);
        CHECK_EQ_INT(nepdate_add_months(first, 3587, overflow, &result), NEPDATE_OK);
        CHECK_EQ_INT(nepdate_add_months(last, -3587, overflow, &result), NEPDATE_OK);
        CHECK_EQ_INT(nepdate_add_years(first, INT32_MIN, overflow, &result),
                     NEPDATE_E_OUT_OF_RANGE);
        CHECK_EQ_INT(nepdate_add_years(last, INT32_MAX, overflow, &result), NEPDATE_E_OUT_OF_RANGE);
    }

    /* Chaitra 2199 has 30 days, so the last day of Bhadra 2199 spills past the table. */
    const nepdate_date bhadra = date_of(2199, 5, 31);
    CHECK_EQ_INT(nepdate_add_months(bhadra, 7, NEPDATE_OVERFLOW, &result), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_add_months(bhadra, 7, NEPDATE_CLAMP, &result), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_bs(result, &ymd), NEPDATE_OK);
    check_ymd(NEPDATE_OK, ymd, 2199, 12, 30);

    /* MAX_VALUE + 1 in the day count must wrap out of range, not into a date. */
    const nepdate_date max_value = {INT32_MAX};
    CHECK_EQ_INT(nepdate_add_days(max_value, 1, &result), NEPDATE_E_INVALID_DATE);
}

TEST(invalid_overflow_mode) {
    nepdate_date result = {-7};
    const nepdate_date date = date_of(2081, 4, 15);
    /* NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange): testing an unknown mode */
    const nepdate_overflow invalid = (nepdate_overflow)2;
    CHECK_EQ_INT(nepdate_add_months(date, 1, invalid, &result), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(result.value, -7);
    CHECK_EQ_INT(nepdate_add_years(date, 1, invalid, &result), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(result.value, -7);
}

TEST(diff_days_is_signed) {
    const nepdate_date earlier = date_of(2080, 5, 1);
    const nepdate_date later = date_of(2080, 5, 16);
    int32_t days = 0;
    CHECK_EQ_INT(nepdate_diff_days(earlier, later, &days), NEPDATE_OK);
    CHECK_EQ_INT(days, 15);
    CHECK_EQ_INT(nepdate_diff_days(later, earlier, &days), NEPDATE_OK);
    CHECK_EQ_INT(days, -15);
    CHECK_EQ_INT(nepdate_diff_days(earlier, earlier, &days), NEPDATE_OK);
    CHECK_EQ_INT(days, 0);
}

TEST(diff_days_antisymmetry_sample) {
    for (uint32_t i = 0; i < 10000U; i++) {
        const uint32_t a_value = 1U + (i * 2654435761U) % 109212U;
        const uint32_t b_value = 1U + ((i + 5000U) * 40503U) % 109212U;
        const nepdate_date a = {(int32_t)a_value};
        const nepdate_date b = {(int32_t)b_value};
        int32_t forward = 0;
        int32_t backward = 0;
        CHECK_EQ_INT(nepdate_diff_days(a, b, &forward), NEPDATE_OK);
        CHECK_EQ_INT(nepdate_diff_days(b, a, &backward), NEPDATE_OK);
        CHECK_EQ_INT(backward, -forward);
    }
}

TEST(diff_algorithm_8a_examples) {
    const struct {
        int ay;
        int am;
        int ad;
        int by;
        int bm;
        int bd;
        int sign;
        int years;
        int months;
        int days;
        int32_t total;
    } cases[] = {
        {2080, 1, 1, 2081, 4, 15, 1, 1, 3, 14, 473}, {2081, 4, 15, 2080, 1, 1, -1, 1, 3, 14, -473},
        {2081, 4, 32, 2081, 5, 31, 1, 0, 1, 0, 31},  {2081, 4, 32, 2081, 6, 1, 1, 0, 1, 1, 32},
        {2081, 1, 31, 2081, 2, 30, 1, 0, 0, 30, 30}, {2081, 3, 15, 2081, 3, 15, 1, 0, 0, 0, 0},
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        nepdate_date_diff diff = {0, 0, 0, 0};
        int32_t total = 0;
        const nepdate_date a = date_of(cases[i].ay, cases[i].am, cases[i].ad);
        const nepdate_date b = date_of(cases[i].by, cases[i].bm, cases[i].bd);
        CHECK_EQ_INT(nepdate_diff(a, b, &diff), NEPDATE_OK);
        CHECK_EQ_INT(diff.sign, cases[i].sign);
        CHECK_EQ_INT(diff.years, cases[i].years);
        CHECK_EQ_INT(diff.months, cases[i].months);
        CHECK_EQ_INT(diff.days, cases[i].days);
        CHECK_EQ_INT(nepdate_diff_days(a, b, &total), NEPDATE_OK);
        CHECK_EQ_INT(total, cases[i].total);
    }
}

TEST(diff_reversed_matches) {
    const nepdate_date a = date_of(2081, 4, 32);
    const nepdate_date b = date_of(2083, 2, 7);
    nepdate_date_diff forward = {0, 0, 0, 0};
    nepdate_date_diff backward = {0, 0, 0, 0};
    CHECK_EQ_INT(nepdate_diff(a, b, &forward), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_diff(b, a, &backward), NEPDATE_OK);
    CHECK_EQ_INT(forward.sign, 1);
    CHECK_EQ_INT(backward.sign, -1);
    CHECK_EQ_INT(forward.years, backward.years);
    CHECK_EQ_INT(forward.months, backward.months);
    CHECK_EQ_INT(forward.days, backward.days);
}

TEST(diff_whole_supported_range) {
    const nepdate_date first = date_of(1901, 1, 1);
    const nepdate_date last = date_of(2199, 12, 30);
    nepdate_date_diff diff = {0, 0, 0, 0};
    int32_t total = 0;
    CHECK_EQ_INT(nepdate_diff(first, last, &diff), NEPDATE_OK);
    CHECK_EQ_INT(diff.sign, 1);
    CHECK_EQ_INT(diff.years, 298);
    CHECK_EQ_INT(diff.months, 11);
    CHECK_EQ_INT(diff.days, 29);
    CHECK_EQ_INT(nepdate_diff_days(first, last, &total), NEPDATE_OK);
    CHECK_EQ_INT(total, 109211);
}

TEST(null_and_invalid_dates) {
    const nepdate_date valid = date_of(2081, 4, 15);
    const nepdate_date invalid = {0};
    nepdate_date out = {-7};
    nepdate_ymd ymd = {-1, -2, -3};
    nepdate_date_diff diff = {-1, -2, -3, -4};
    int32_t days = -7;
    int number = -7;
    nepdate_quarter quarter = NEPDATE_Q4;
    nepdate_range range = {{-1}, {-1}};

    CHECK_EQ_INT(nepdate_add_days(valid, 1, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_add_months(valid, 1, NEPDATE_CLAMP, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_add_years(valid, 1, NEPDATE_CLAMP, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_diff_days(valid, valid, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_diff(valid, valid, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_fiscal_year_of(valid, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_fiscal_year_start(2081, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_fiscal_year_end(2081, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_quarter_of(valid, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_quarter_start(2081, NEPDATE_Q1, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_quarter_end(2081, NEPDATE_Q1, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_make(valid, valid, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_of_month(2081, 4, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_of_year(2081, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_of_fiscal_year(2081, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_length(range, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_contains(range, valid, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_range_begin(range, NULL), NEPDATE_E_NULL);
    CHECK(nepdate_range_next(NULL, &out) == false);

    CHECK_EQ_INT(nepdate_add_days(invalid, 1, &out), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(out.value, -7);
    CHECK_EQ_INT(nepdate_add_months(invalid, 1, NEPDATE_CLAMP, &out), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(out.value, -7);
    CHECK_EQ_INT(nepdate_add_years(invalid, 1, NEPDATE_CLAMP, &out), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(out.value, -7);
    CHECK_EQ_INT(nepdate_diff_days(invalid, valid, &days), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(days, -7);
    CHECK_EQ_INT(nepdate_diff_days(valid, invalid, &days), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(days, -7);
    CHECK_EQ_INT(nepdate_diff(invalid, valid, &diff), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(diff.sign, -1);
    CHECK_EQ_INT(nepdate_fiscal_year_of(invalid, &number), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(number, -7);
    CHECK_EQ_INT(nepdate_quarter_of(invalid, &quarter), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(quarter, NEPDATE_Q4);

    nepdate_ymd ad = {-1, -2, -3};
    CHECK_EQ_INT(nepdate_to_ad(invalid, &ad), NEPDATE_E_INVALID_DATE);
    CHECK(ad.year == -1 && ad.month == -2 && ad.day == -3);
    CHECK_EQ_INT(nepdate_to_bs(invalid, &ymd), NEPDATE_E_INVALID_DATE);
    CHECK(ymd.year == -1 && ymd.month == -2 && ymd.day == -3);
}

int main(void) {
    RUN_TEST(golden_add_days);
    RUN_TEST(golden_add_months);
    RUN_TEST(algorithm_8_examples);
    RUN_TEST(add_years_uses_months);
    RUN_TEST(arithmetic_extremes);
    RUN_TEST(invalid_overflow_mode);
    RUN_TEST(diff_days_is_signed);
    RUN_TEST(diff_days_antisymmetry_sample);
    RUN_TEST(diff_algorithm_8a_examples);
    RUN_TEST(diff_reversed_matches);
    RUN_TEST(diff_whole_supported_range);
    RUN_TEST(null_and_invalid_dates);
    return harness_finish();
}
