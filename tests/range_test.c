/* Inclusive date ranges: construction, length, containment and the day cursor
 * (ALGORITHM section 8b for the fiscal-year range). */
#include "harness.h"

#include <nepdate.h>

#include <stddef.h>
#include <stdint.h>

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

TEST(make_and_measure) {
    nepdate_range range = {{0}, {0}};
    const nepdate_date start = date_of(2080, 1, 1);
    const nepdate_date end = date_of(2080, 1, 15);
    CHECK_EQ_INT(nepdate_range_make(start, end, &range), NEPDATE_OK);
    check_date(range.start, 2080, 1, 1);
    check_date(range.end, 2080, 1, 15);

    int length = 0;
    CHECK_EQ_INT(nepdate_range_length(range, &length), NEPDATE_OK);
    CHECK_EQ_INT(length, 15);
    CHECK_EQ_INT(nepdate_range_make(start, start, &range), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_range_length(range, &length), NEPDATE_OK);
    CHECK_EQ_INT(length, 1);

    bool contains = false;
    CHECK_EQ_INT(nepdate_range_contains(range, start, &contains), NEPDATE_OK);
    CHECK(contains);

    // NOLINTNEXTLINE(readability-suspicious-call-argument): testing a reversed range on purpose
    CHECK_EQ_INT(nepdate_range_make(end, start, &range), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_length(range, &length), NEPDATE_OK);
    CHECK_EQ_INT(length, 1);
}

TEST(contains_endpoints_and_outside) {
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_make(date_of(2080, 1, 5), date_of(2080, 1, 20), &range), NEPDATE_OK);
    const struct {
        int day;
        bool expected;
    } cases[] = {{4, false}, {5, true}, {10, true}, {20, true}, {21, false}};
    bool contains = false;
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        CHECK_EQ_INT(nepdate_range_contains(range, date_of(2080, 1, cases[i].day), &contains),
                     NEPDATE_OK);
        CHECK_EQ_INT(contains, cases[i].expected);
    }
}

TEST(month_ranges) {
    for (int year = NEPDATE_MIN_YEAR; year <= NEPDATE_MAX_YEAR; year++) {
        for (int month = 1; month <= 12; month++) {
            nepdate_range range = {{0}, {0}};
            CHECK_EQ_INT(nepdate_range_of_month(year, month, &range), NEPDATE_OK);
            nepdate_ymd first = {0, 0, 0};
            nepdate_ymd last = {0, 0, 0};
            CHECK_EQ_INT(nepdate_to_bs(range.start, &first), NEPDATE_OK);
            CHECK_EQ_INT(nepdate_to_bs(range.end, &last), NEPDATE_OK);
            CHECK_EQ_INT(first.year, year);
            CHECK_EQ_INT(first.month, month);
            CHECK_EQ_INT(first.day, 1);
            CHECK_EQ_INT(last.year, year);
            CHECK_EQ_INT(last.month, month);
            int month_days = 0;
            CHECK_EQ_INT(nepdate_month_length(range.start, &month_days), NEPDATE_OK);
            CHECK_EQ_INT(last.day, month_days);
            int length = 0;
            CHECK_EQ_INT(nepdate_range_length(range, &length), NEPDATE_OK);
            CHECK_EQ_INT(length, month_days);
        }
    }
}

/* The acceptance criterion: a year's range is as long as its twelve months together. */
TEST(year_ranges_match_month_lengths) {
    for (int year = NEPDATE_MIN_YEAR; year <= NEPDATE_MAX_YEAR; year++) {
        nepdate_range year_range = {{0}, {0}};
        CHECK_EQ_INT(nepdate_range_of_year(year, &year_range), NEPDATE_OK);
        int total = 0;
        for (int month = 1; month <= 12; month++) {
            nepdate_range month_range = {{0}, {0}};
            CHECK_EQ_INT(nepdate_range_of_month(year, month, &month_range), NEPDATE_OK);
            int length = 0;
            CHECK_EQ_INT(nepdate_range_length(month_range, &length), NEPDATE_OK);
            total += length;
        }
        int year_length = 0;
        CHECK_EQ_INT(nepdate_range_length(year_range, &year_length), NEPDATE_OK);
        CHECK_EQ_INT(year_length, total);
        CHECK(year_length >= 364 && year_length <= 367);
        check_date(year_range.start, year, 1, 1);
        nepdate_ymd end = {0, 0, 0};
        CHECK_EQ_INT(nepdate_to_bs(year_range.end, &end), NEPDATE_OK);
        CHECK_EQ_INT(end.year, year);
        CHECK_EQ_INT(end.month, 12);
        int chaitra = 0;
        CHECK_EQ_INT(nepdate_month_length(year_range.end, &chaitra), NEPDATE_OK);
        CHECK_EQ_INT(end.day, chaitra);
    }
}

TEST(fiscal_year_ranges) {
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_fiscal_year(2080, &range), NEPDATE_OK);
    check_date(range.start, 2080, 4, 1);
    check_date(range.end, 2081, 3, 31);
    int length = 0;
    CHECK_EQ_INT(nepdate_range_length(range, &length), NEPDATE_OK);
    int total = 0;
    for (int month = 4; month <= 12; month++) {
        nepdate_range part = {{0}, {0}};
        CHECK_EQ_INT(nepdate_range_of_month(2080, month, &part), NEPDATE_OK);
        int part_length = 0;
        CHECK_EQ_INT(nepdate_range_length(part, &part_length), NEPDATE_OK);
        total += part_length;
    }
    for (int month = 1; month <= 3; month++) {
        nepdate_range part = {{0}, {0}};
        CHECK_EQ_INT(nepdate_range_of_month(2081, month, &part), NEPDATE_OK);
        int part_length = 0;
        CHECK_EQ_INT(nepdate_range_length(part, &part_length), NEPDATE_OK);
        total += part_length;
    }
    CHECK_EQ_INT(length, total);

    CHECK_EQ_INT(nepdate_range_of_fiscal_year(1900, &range), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_of_fiscal_year(2199, &range), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_of_fiscal_year(INT32_MIN, &range), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_of_fiscal_year(INT32_MAX, &range), NEPDATE_E_OUT_OF_RANGE);
}

TEST(range_errors) {
    nepdate_range range = {{0}, {0}};
    const nepdate_date valid = date_of(2080, 1, 1);
    const nepdate_date invalid = {0};

    CHECK_EQ_INT(nepdate_range_make(invalid, valid, &range), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_make(valid, invalid, &range), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_make(invalid, invalid, &range), NEPDATE_E_INVALID_DATE);

    CHECK_EQ_INT(nepdate_range_of_month(1900, 1, &range), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_of_month(2200, 1, &range), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_of_month(2080, 0, &range), NEPDATE_E_INVALID_MONTH);
    CHECK_EQ_INT(nepdate_range_of_month(2080, 13, &range), NEPDATE_E_INVALID_MONTH);
    CHECK_EQ_INT(nepdate_range_of_year(1900, &range), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_of_year(2200, &range), NEPDATE_E_OUT_OF_RANGE);

    /* A range no function made: reversed, invalid start, invalid end. */
    const nepdate_range reversed = {{20}, {10}};
    const nepdate_range bad_start = {{0}, {10}};
    const nepdate_range bad_end = {{10}, {0}};
    int length = 0;
    bool contains = true;
    nepdate_range_iterator iterator = {0, 0};
    CHECK_EQ_INT(nepdate_range_length(reversed, &length), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_length(bad_start, &length), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_length(bad_end, &length), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_contains(reversed, valid, &contains), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_contains(bad_start, valid, &contains), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_begin(reversed, &iterator), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_begin(bad_start, &iterator), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_range_make(valid, valid, &range), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_range_contains(range, invalid, &contains), NEPDATE_E_INVALID_DATE);
    CHECK(contains);
}

TEST(iterator_walks_the_range) {
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_make(date_of(2080, 5, 13), date_of(2080, 5, 15), &range),
                 NEPDATE_OK);
    nepdate_range_iterator iterator = {0, 0};
    CHECK_EQ_INT(nepdate_range_begin(range, &iterator), NEPDATE_OK);
    nepdate_date date = {0};
    int count = 0;
    while (nepdate_range_next(&iterator, &date)) {
        check_date(date, 2080, 5, 13 + count);
        count++;
    }
    CHECK_EQ_INT(count, 3);
    CHECK(nepdate_range_next(&iterator, &date) == false);

    CHECK_EQ_INT(nepdate_range_make(date_of(2080, 5, 15), date_of(2080, 5, 15), &range),
                 NEPDATE_OK);
    CHECK_EQ_INT(nepdate_range_begin(range, &iterator), NEPDATE_OK);
    count = 0;
    while (nepdate_range_next(&iterator, &date)) {
        check_date(date, 2080, 5, 15);
        count++;
    }
    CHECK_EQ_INT(count, 1);
}

TEST(iterator_rejects_a_forged_cursor) {
    nepdate_date date = {0};
    nepdate_date unchanged = {-7};
    nepdate_range_iterator cursor = {109212, 109212};
    CHECK(nepdate_range_next(&cursor, &date));
    CHECK_EQ_INT(date.value, 109212);
    CHECK(nepdate_range_next(&cursor, &date) == false);

    CHECK(nepdate_range_next(NULL, &date) == false);
    CHECK(nepdate_range_next(&cursor, NULL) == false);

    const nepdate_range_iterator zero = {0, 5};
    const nepdate_range_iterator negative = {INT32_MIN, 5};
    const nepdate_range_iterator too_large = {5, INT32_MAX};
    const nepdate_range_iterator beyond_last = {109213, 109212};
    const nepdate_range_iterator reversed = {10, 5};
    nepdate_range_iterator it = zero;
    CHECK(nepdate_range_next(&it, &unchanged) == false);
    CHECK_EQ_INT(unchanged.value, -7);
    it = negative;
    CHECK(nepdate_range_next(&it, &unchanged) == false);
    it = too_large;
    CHECK(nepdate_range_next(&it, &unchanged) == false);
    it = beyond_last;
    CHECK(nepdate_range_next(&it, &unchanged) == false);
    it = reversed;
    CHECK(nepdate_range_next(&it, &unchanged) == false);
}

int main(void) {
    RUN_TEST(make_and_measure);
    RUN_TEST(contains_endpoints_and_outside);
    RUN_TEST(month_ranges);
    RUN_TEST(year_ranges_match_month_lengths);
    RUN_TEST(fiscal_year_ranges);
    RUN_TEST(range_errors);
    RUN_TEST(iterator_walks_the_range);
    RUN_TEST(iterator_rejects_a_forged_cursor);
    return harness_finish();
}
