/* AD conversion, the derived properties and comparison (ALGORITHM sections 4 to 7). */
#include "harness.h"

#include <nepdate.h>

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

static nepdate_date bs(int year, int month, int day) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_bs(year, month, day, &date), NEPDATE_OK);
    return date;
}

static nepdate_status ad_status(int year, int month, int day) {
    nepdate_date date = {0};
    return nepdate_from_ad(year, month, day, &date);
}

static nepdate_status bs_status(int year, int month, int day) {
    nepdate_date date = {0};
    return nepdate_from_bs(year, month, day, &date);
}

static void check_ad(nepdate_date date, int year, int month, int day) {
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_ad(date, &ymd), NEPDATE_OK);
    CHECK_EQ_INT(ymd.year, year);
    CHECK_EQ_INT(ymd.month, month);
    CHECK_EQ_INT(ymd.day, day);
    nepdate_date back = {0};
    CHECK_EQ_INT(nepdate_from_ad(year, month, day, &back), NEPDATE_OK);
    CHECK_EQ_INT(back.value, date.value);
}

static void check_weekday(nepdate_date date, nepdate_weekday weekday) {
    nepdate_weekday got = NEPDATE_SUNDAY;
    CHECK_EQ_INT(nepdate_weekday_of(date, &got), NEPDATE_OK);
    CHECK_EQ_INT(got, weekday);
}

TEST(ad_boundaries_convert_both_ways) {
    const nepdate_date first = bs(1901, 1, 1);
    const nepdate_date last = bs(2199, 12, 30);
    CHECK_EQ_INT(first.value, 1);
    CHECK_EQ_INT(last.value, 109212);
    check_ad(first, 1844, 4, 11);
    check_ad(last, 2143, 4, 15);
}

TEST(ad_error_precedence) {
    CHECK_EQ_INT(ad_status(2023, 2, 29), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(ad_status(2023, 2, 30), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(ad_status(2023, 13, 1), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(ad_status(2023, 0, 1), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(ad_status(2023, 4, 31), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(ad_status(2023, 1, 0), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(ad_status(1900, 2, 29), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(ad_status(1800, 2, 29), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(ad_status(2024, 2, 29), NEPDATE_OK);
    CHECK_EQ_INT(ad_status(2000, 2, 29), NEPDATE_OK);
    CHECK_EQ_INT(ad_status(1844, 3, 31), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(ad_status(1844, 4, 10), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(ad_status(1844, 4, 11), NEPDATE_OK);
    CHECK_EQ_INT(ad_status(2143, 4, 15), NEPDATE_OK);
    CHECK_EQ_INT(ad_status(2143, 4, 16), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(ad_status(2143, 5, 1), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(ad_status(1843, 12, 31), NEPDATE_E_OUT_OF_RANGE);
    /* A real Gregorian date outside the range is out of range even when the year is far away. */
    CHECK_EQ_INT(ad_status(1600, 2, 29), NEPDATE_E_OUT_OF_RANGE);
}

TEST(ad_extreme_arguments) {
    CHECK_EQ_INT(ad_status(INT_MAX, 1, 1), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(ad_status(INT_MIN, 1, 1), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(ad_status(INT_MAX, 2, 29), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(ad_status(INT_MIN, 2, 29), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(ad_status(2024, INT_MIN, 1), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(ad_status(2024, INT_MAX, 1), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(ad_status(2024, 1, INT_MIN), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(ad_status(2024, 1, INT_MAX), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(ad_status(INT_MAX, INT_MAX, INT_MAX), NEPDATE_E_INVALID_GREGORIAN);
}

TEST(leap_years) {
    CHECK_EQ_INT(ad_status(2024, 2, 29), NEPDATE_OK);
    CHECK_EQ_INT(ad_status(2028, 2, 29), NEPDATE_OK);
    CHECK_EQ_INT(ad_status(2000, 2, 29), NEPDATE_OK);
    CHECK_EQ_INT(ad_status(1900, 2, 29), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(ad_status(2100, 2, 29), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(ad_status(2023, 2, 29), NEPDATE_E_INVALID_GREGORIAN);
}

TEST(weekdays_match_algorithm_section_9) {
    check_weekday(bs(1901, 1, 1), NEPDATE_THURSDAY);
    check_weekday(bs(2081, 1, 1), NEPDATE_SATURDAY);
    check_weekday(bs(2082, 1, 1), NEPDATE_MONDAY);
    check_weekday(bs(2083, 1, 1), NEPDATE_TUESDAY);
    check_weekday(bs(2080, 5, 15), NEPDATE_FRIDAY);
    check_weekday(bs(2080, 1, 1), NEPDATE_FRIDAY);
    check_weekday(bs(2199, 12, 30), NEPDATE_MONDAY);
}

TEST(month_of_every_month) {
    for (int month = 1; month <= 12; month++) {
        nepdate_month got = NEPDATE_BAISHAKH;
        CHECK_EQ_INT(nepdate_month_of(bs(2081, month, 1), &got), NEPDATE_OK);
        CHECK_EQ_INT(got, (nepdate_month)month);
    }
    nepdate_month got = NEPDATE_BAISHAKH;
    CHECK_EQ_INT(nepdate_month_of(bs(2080, 5, 15), &got), NEPDATE_OK);
    CHECK_EQ_INT(got, NEPDATE_BHADRA);
}

TEST(day_of_year_and_month_length) {
    CHECK_EQ_INT(bs_status(2081, 4, 31), NEPDATE_OK);

    int day_of_year = 0;
    CHECK_EQ_INT(nepdate_day_of_year(bs(2080, 1, 1), &day_of_year), NEPDATE_OK);
    CHECK_EQ_INT(day_of_year, 1);
    CHECK_EQ_INT(nepdate_day_of_year(bs(2080, 4, 1), &day_of_year), NEPDATE_OK);
    CHECK_EQ_INT(day_of_year, 31 + 32 + 31 + 1);

    int length = 0;
    CHECK_EQ_INT(nepdate_month_length(bs(2081, 4, 1), &length), NEPDATE_OK);
    CHECK_EQ_INT(length, 32);
    CHECK_EQ_INT(nepdate_month_length(bs(2081, 3, 1), &length), NEPDATE_OK);
    CHECK_EQ_INT(length, 31);

    /* Every day's day of year is the sum of the earlier months plus the day. */
    for (int year = 1901; year <= 2199; year++) {
        int sum = 0;
        for (int month = 1; month <= 12; month++) {
            const nepdate_date first = bs(year, month, 1);
            CHECK_EQ_INT(nepdate_month_length(first, &length), NEPDATE_OK);
            for (int day = 1; day <= length; day++) {
                const nepdate_date date = bs(year, month, day);
                CHECK_EQ_INT(nepdate_day_of_year(date, &day_of_year), NEPDATE_OK);
                CHECK_EQ_INT(day_of_year, sum + day);
            }
            sum += length;
        }
        CHECK(sum >= 364 && sum <= 367);
    }
}

TEST(first_and_last_day_of_every_month) {
    for (int year = 1901; year <= 2199; year++) {
        for (int month = 1; month <= 12; month++) {
            const nepdate_date first = bs(year, month, 1);
            int length = 0;
            CHECK_EQ_INT(nepdate_month_length(first, &length), NEPDATE_OK);
            nepdate_date got_first = {0};
            nepdate_date got_last = {0};
            CHECK_EQ_INT(nepdate_first_day_of_month(first, &got_first), NEPDATE_OK);
            CHECK_EQ_INT(nepdate_last_day_of_month(first, &got_last), NEPDATE_OK);
            CHECK_EQ_INT(got_first.value, first.value);
            CHECK_EQ_INT(got_last.value, bs(year, month, length).value);

            nepdate_ymd ymd = {0, 0, 0};
            CHECK_EQ_INT(nepdate_to_bs(got_first, &ymd), NEPDATE_OK);
            CHECK_EQ_INT(ymd.year, year);
            CHECK_EQ_INT(ymd.month, month);
            CHECK_EQ_INT(ymd.day, 1);
            CHECK_EQ_INT(nepdate_to_bs(got_last, &ymd), NEPDATE_OK);
            CHECK_EQ_INT(ymd.year, year);
            CHECK_EQ_INT(ymd.month, month);
            CHECK_EQ_INT(ymd.day, length);

            /* The first and last days of a month reach the same result from any day in it. */
            const nepdate_date middle = bs(year, month, (length + 1) / 2);
            CHECK_EQ_INT(nepdate_first_day_of_month(middle, &got_first), NEPDATE_OK);
            CHECK_EQ_INT(got_first.value, first.value);
            CHECK_EQ_INT(nepdate_last_day_of_month(middle, &got_last), NEPDATE_OK);
            CHECK_EQ_INT(got_last.value, bs(year, month, length).value);
        }
    }
}

TEST(compare_dates) {
    const nepdate_date earlier = bs(2080, 5, 15);
    const nepdate_date same = bs(2080, 5, 15);
    const nepdate_date later = bs(2080, 5, 16);
    const nepdate_date next_year = bs(2081, 1, 1);
    int result = 99;
    CHECK_EQ_INT(nepdate_compare(earlier, later, &result), NEPDATE_OK);
    CHECK_EQ_INT(result, -1);
    CHECK_EQ_INT(nepdate_compare(same, earlier, &result), NEPDATE_OK);
    CHECK_EQ_INT(result, 0);
    CHECK_EQ_INT(nepdate_compare(later, earlier, &result), NEPDATE_OK);
    CHECK_EQ_INT(result, 1);
    CHECK_EQ_INT(nepdate_compare(next_year, later, &result), NEPDATE_OK);
    CHECK_EQ_INT(result, 1);
    CHECK_EQ_INT(nepdate_compare(earlier, next_year, &result), NEPDATE_OK);
    CHECK_EQ_INT(result, -1);
    CHECK_EQ_INT(nepdate_compare(bs(1901, 1, 1), bs(2199, 12, 30), &result), NEPDATE_OK);
    CHECK_EQ_INT(result, -1);
}

TEST(null_and_invalid_arguments) {
    const nepdate_date valid = bs(2081, 4, 15);
    const nepdate_date invalid = {0};
    nepdate_ymd ymd = {-1, -2, -3};
    nepdate_weekday weekday = NEPDATE_SUNDAY;
    nepdate_month month = NEPDATE_BAISHAKH;
    nepdate_date out = {0};
    int number = -7;
    int comparison = -7;

    CHECK_EQ_INT(nepdate_from_ad(2081, 4, 15, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_to_ad(valid, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_weekday_of(valid, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_month_of(valid, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_day_of_year(valid, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_month_length(valid, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_first_day_of_month(valid, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_last_day_of_month(valid, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_compare(valid, valid, NULL), NEPDATE_E_NULL);

    CHECK_EQ_INT(nepdate_to_ad(invalid, &ymd), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_weekday_of(invalid, &weekday), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_month_of(invalid, &month), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_day_of_year(invalid, &number), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_month_length(invalid, &number), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_first_day_of_month(invalid, &out), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_last_day_of_month(invalid, &out), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_compare(invalid, valid, &comparison), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_compare(valid, invalid, &comparison), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_compare(invalid, invalid, &comparison), NEPDATE_E_INVALID_DATE);

    /* Nothing was written on an error. */
    CHECK(ymd.year == -1 && ymd.month == -2 && ymd.day == -3);
    CHECK_EQ_INT(weekday, NEPDATE_SUNDAY);
    CHECK_EQ_INT(month, NEPDATE_BAISHAKH);
    CHECK_EQ_INT(number, -7);
    CHECK_EQ_INT(out.value, 0);
    CHECK_EQ_INT(comparison, -7);

    /* Every value outside 1..109212 is rejected, including the ends of the int range. */
    static const int32_t values[] = {INT32_MIN, -1, 0, 109213, INT32_MAX};
    for (size_t i = 0; i < sizeof values / sizeof values[0]; i++) {
        const nepdate_date bad = {values[i]};
        CHECK_EQ_INT(nepdate_to_ad(bad, &ymd), NEPDATE_E_INVALID_DATE);
        CHECK_EQ_INT(nepdate_weekday_of(bad, &weekday), NEPDATE_E_INVALID_DATE);
        CHECK_EQ_INT(nepdate_month_of(bad, &month), NEPDATE_E_INVALID_DATE);
        CHECK_EQ_INT(nepdate_day_of_year(bad, &number), NEPDATE_E_INVALID_DATE);
        CHECK_EQ_INT(nepdate_month_length(bad, &number), NEPDATE_E_INVALID_DATE);
        CHECK_EQ_INT(nepdate_first_day_of_month(bad, &out), NEPDATE_E_INVALID_DATE);
        CHECK_EQ_INT(nepdate_last_day_of_month(bad, &out), NEPDATE_E_INVALID_DATE);
        CHECK_EQ_INT(nepdate_compare(bad, valid, &comparison), NEPDATE_E_INVALID_DATE);
    }
}

int main(void) {
    RUN_TEST(ad_boundaries_convert_both_ways);
    RUN_TEST(ad_error_precedence);
    RUN_TEST(ad_extreme_arguments);
    RUN_TEST(leap_years);
    RUN_TEST(weekdays_match_algorithm_section_9);
    RUN_TEST(month_of_every_month);
    RUN_TEST(day_of_year_and_month_length);
    RUN_TEST(first_and_last_day_of_every_month);
    RUN_TEST(compare_dates);
    RUN_TEST(null_and_invalid_arguments);
    return harness_finish();
}
