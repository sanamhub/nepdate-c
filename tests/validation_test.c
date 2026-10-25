/* Status codes and BS validation (ALGORITHM section 3): error precedence, every int argument
 * including INT_MIN and INT_MAX, NULL out-pointers and dates no constructor made. */
#include "harness.h"

#include <nepdate.h>

#include <limits.h>
#include <stddef.h>

static const int extremes[] = {INT_MIN, INT_MIN + 1, -1,   0,    1,    12,          13,     32,
                               33,      1900,        1901, 2199, 2200, INT_MAX - 1, INT_MAX};
enum { EXTREMES = sizeof extremes / sizeof extremes[0] };

/* The status ALGORITHM section 3 gives for year, month and day, with month lengths from
 * nepdate_from_bs itself only where the year and month are valid. */
static nepdate_status expected_status(int year, int month, int day) {
    if (year < 1901 || year > 2199) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    if (month < 1 || month > 12) {
        return NEPDATE_E_INVALID_MONTH;
    }
    if (day < 1 || day > 32) {
        return NEPDATE_E_INVALID_DAY;
    }
    return NEPDATE_OK;
}

TEST(status_names) {
    CHECK_EQ_STR(nepdate_status_name(NEPDATE_OK), "NEPDATE_OK");
    CHECK_EQ_STR(nepdate_status_name(NEPDATE_E_OUT_OF_RANGE), "NEPDATE_E_OUT_OF_RANGE");
    CHECK_EQ_STR(nepdate_status_name(NEPDATE_E_INVALID_MONTH), "NEPDATE_E_INVALID_MONTH");
    CHECK_EQ_STR(nepdate_status_name(NEPDATE_E_INVALID_DAY), "NEPDATE_E_INVALID_DAY");
    CHECK_EQ_STR(nepdate_status_name(NEPDATE_E_INVALID_GREGORIAN), "NEPDATE_E_INVALID_GREGORIAN");
    CHECK_EQ_STR(nepdate_status_name(NEPDATE_E_INVALID_CHARACTER), "NEPDATE_E_INVALID_CHARACTER");
    CHECK_EQ_STR(nepdate_status_name(NEPDATE_E_WRONG_GROUP_COUNT), "NEPDATE_E_WRONG_GROUP_COUNT");
    CHECK_EQ_STR(nepdate_status_name(NEPDATE_E_NUMBER_TOO_LONG), "NEPDATE_E_NUMBER_TOO_LONG");
    CHECK_EQ_STR(nepdate_status_name(NEPDATE_E_AMBIGUOUS), "NEPDATE_E_AMBIGUOUS");
    CHECK_EQ_STR(nepdate_status_name(NEPDATE_E_UNRECOGNIZED), "NEPDATE_E_UNRECOGNIZED");
    CHECK_EQ_STR(nepdate_status_name(NEPDATE_E_INVALID_DATE), "NEPDATE_E_INVALID_DATE");
    CHECK_EQ_STR(nepdate_status_name(NEPDATE_E_NULL), "NEPDATE_E_NULL");
    CHECK_EQ_STR(nepdate_status_name((nepdate_status)12), "unknown");
    CHECK_EQ_STR(nepdate_status_name((nepdate_status)255), "unknown");
}

TEST(status_values_are_fixed) {
    CHECK_EQ_INT(NEPDATE_OK, 0);
    CHECK_EQ_INT(NEPDATE_E_OUT_OF_RANGE, 1);
    CHECK_EQ_INT(NEPDATE_E_INVALID_GREGORIAN, 4);
    CHECK_EQ_INT(NEPDATE_E_UNRECOGNIZED, 9);
    CHECK_EQ_INT(NEPDATE_E_INVALID_DATE, 10);
    CHECK_EQ_INT(NEPDATE_E_NULL, 11);
    CHECK_EQ_INT(sizeof(nepdate_date), 4);
}

TEST(shrawan_2081_has_32_days) {
    nepdate_date date;
    CHECK_EQ_INT(nepdate_from_bs(2081, 4, 32, &date), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_from_bs(2081, 4, 33, &date), NEPDATE_E_INVALID_DAY);
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_from_bs(2081, 4, 32, &date), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_bs(date, &ymd), NEPDATE_OK);
    CHECK_EQ_INT(ymd.year, 2081);
    CHECK_EQ_INT(ymd.month, 4);
    CHECK_EQ_INT(ymd.day, 32);
}

TEST(first_and_last_supported_days) {
    nepdate_date date;
    CHECK_EQ_INT(nepdate_from_bs(1901, 1, 1, &date), NEPDATE_OK);
    CHECK_EQ_INT(date.value, 1);
    CHECK_EQ_INT(nepdate_from_bs(2199, 12, 30, &date), NEPDATE_OK);
    CHECK_EQ_INT(date.value, 109212);
    CHECK_EQ_INT(nepdate_from_bs(2199, 12, 31, &date), NEPDATE_E_INVALID_DAY);
    CHECK_EQ_INT(nepdate_from_bs(1900, 12, 30, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_from_bs(2200, 1, 1, &date), NEPDATE_E_OUT_OF_RANGE);
}

TEST(error_precedence_is_year_month_day) {
    nepdate_date date;
    CHECK_EQ_INT(nepdate_from_bs(1900, 13, 40, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_from_bs(2081, 13, 40, &date), NEPDATE_E_INVALID_MONTH);
    CHECK_EQ_INT(nepdate_from_bs(2081, 0, 0, &date), NEPDATE_E_INVALID_MONTH);
    CHECK_EQ_INT(nepdate_from_bs(2081, 1, 0, &date), NEPDATE_E_INVALID_DAY);
}

/* Every combination of the extreme values: no undefined behaviour (the sanitizer jobs run this)
 * and the status ALGORITHM section 3 names. */
TEST(extreme_arguments_return_an_error) {
    for (int i = 0; i < EXTREMES; i++) {
        for (int j = 0; j < EXTREMES; j++) {
            for (int k = 0; k < EXTREMES; k++) {
                const int year = extremes[i];
                const int month = extremes[j];
                const int day = extremes[k];
                nepdate_date date = {-7};
                const nepdate_status status = nepdate_from_bs(year, month, day, &date);
                nepdate_status expected = expected_status(year, month, day);
                /* Only day 32 depends on the month: 2199-12 has 30 days, 2199-01 31. */
                if (expected == NEPDATE_OK && day == 32) {
                    expected = status == NEPDATE_OK ? NEPDATE_OK : NEPDATE_E_INVALID_DAY;
                }
                CHECK_EQ_INT(status, expected);
                if (status != NEPDATE_OK) {
                    CHECK_EQ_INT(date.value, -7);
                }
            }
        }
    }
}

/* Every valid day builds and reads back; the day after each month's last does not. */
TEST(every_day_builds_and_reads_back) {
    int32_t previous = 0;
    for (int year = 1901; year <= 2199; year++) {
        for (int month = 1; month <= 12; month++) {
            int day = 1;
            nepdate_date date;
            while (nepdate_from_bs(year, month, day, &date) == NEPDATE_OK) {
                CHECK_EQ_INT(date.value, previous + 1);
                previous = date.value;
                nepdate_ymd ymd = {0, 0, 0};
                CHECK_EQ_INT(nepdate_to_bs(date, &ymd), NEPDATE_OK);
                CHECK(ymd.year == year && ymd.month == month && ymd.day == day);
                day++;
            }
            CHECK(day >= 30 && day <= 33);
            CHECK_EQ_INT(nepdate_from_bs(year, month, day, &date), NEPDATE_E_INVALID_DAY);
        }
    }
    CHECK_EQ_INT(previous, 109212);
}

TEST(null_out_pointer) {
    nepdate_date date;
    CHECK_EQ_INT(nepdate_from_bs(2081, 4, 15, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_from_bs(1900, 0, 0, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_from_bs(2081, 4, 15, &date), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_bs(date, NULL), NEPDATE_E_NULL);
}

TEST(dates_no_constructor_made_are_rejected) {
    static const int32_t values[] = {INT32_MIN, -1, 0, 109213, INT32_MAX};
    for (size_t i = 0; i < sizeof values / sizeof values[0]; i++) {
        const nepdate_date date = {values[i]};
        nepdate_ymd ymd = {-1, -2, -3};
        CHECK_EQ_INT(nepdate_to_bs(date, &ymd), NEPDATE_E_INVALID_DATE);
        CHECK(ymd.year == -1 && ymd.month == -2 && ymd.day == -3);
    }
    const nepdate_date zero = {0};
    nepdate_ymd ymd;
    CHECK_EQ_INT(nepdate_to_bs(zero, &ymd), NEPDATE_E_INVALID_DATE);
}

int main(void) {
    RUN_TEST(status_names);
    RUN_TEST(status_values_are_fixed);
    RUN_TEST(shrawan_2081_has_32_days);
    RUN_TEST(first_and_last_supported_days);
    RUN_TEST(error_precedence_is_year_month_day);
    RUN_TEST(extreme_arguments_return_an_error);
    RUN_TEST(every_day_builds_and_reads_back);
    RUN_TEST(null_out_pointer);
    RUN_TEST(dates_no_constructor_made_are_rejected);
    return harness_finish();
}
