/* Every bit pattern of nepdate_date is safe: each function returns either NEPDATE_OK with a date
 * the library could have made, or NEPDATE_E_INVALID_DATE and leaves the out object alone. */
#include "harness.h"

#include <nepdate.h>

#include <stdint.h>

#ifndef NEPDATE_C_BIT_PATTERN_BITS
#define NEPDATE_C_BIT_PATTERN_BITS 24
#endif

_Static_assert(NEPDATE_C_BIT_PATTERN_BITS >= 1 && NEPDATE_C_BIT_PATTERN_BITS <= 32,
               "NEPDATE_C_BIT_PATTERN_BITS must be 1..32");

/* A value the library made, for the comparison calls. */
static nepdate_date anchor_date;

static void check_invalid(uint32_t value) {
    const nepdate_date date = {(int32_t)value};
    nepdate_ymd ymd = {-1, -2, -3};
    nepdate_weekday weekday = NEPDATE_SATURDAY;
    nepdate_month month = NEPDATE_CHAITRA;
    nepdate_date out = {-7};
    int number = -7;
    int comparison = -7;
    const nepdate_date valid = anchor_date;

    CHECK_EQ_INT(nepdate_to_bs(date, &ymd), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_to_ad(date, &ymd), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_weekday_of(date, &weekday), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_month_of(date, &month), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_day_of_year(date, &number), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_month_length(date, &number), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_first_day_of_month(date, &out), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_last_day_of_month(date, &out), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_compare(date, valid, &comparison), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_compare(valid, date, &comparison), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(nepdate_compare(date, date, &comparison), NEPDATE_E_INVALID_DATE);

    CHECK(ymd.year == -1 && ymd.month == -2 && ymd.day == -3);
    CHECK_EQ_INT(weekday, NEPDATE_SATURDAY);
    CHECK_EQ_INT(month, NEPDATE_CHAITRA);
    CHECK_EQ_INT(out.value, -7);
    CHECK_EQ_INT(number, -7);
    CHECK_EQ_INT(comparison, -7);
}

static void check_valid(uint32_t value) {
    const nepdate_date date = {(int32_t)value};
    nepdate_ymd ymd = {0, 0, 0};
    nepdate_ymd ad = {0, 0, 0};
    nepdate_weekday weekday = NEPDATE_SUNDAY;
    nepdate_month month = NEPDATE_BAISHAKH;
    nepdate_date first = {0};
    nepdate_date last = {0};
    int day_of_year = 0;
    int length = 0;
    int comparison = 0;
    const nepdate_date valid = anchor_date;

    CHECK_EQ_INT(nepdate_to_bs(date, &ymd), NEPDATE_OK);
    CHECK(ymd.year >= 1901 && ymd.year <= 2199);
    CHECK(ymd.month >= 1 && ymd.month <= 12);
    CHECK(ymd.day >= 1 && ymd.day <= 32);
    nepdate_date back = {0};
    CHECK_EQ_INT(nepdate_from_bs(ymd.year, ymd.month, ymd.day, &back), NEPDATE_OK);
    CHECK_EQ_INT(back.value, date.value);

    CHECK_EQ_INT(nepdate_to_ad(date, &ad), NEPDATE_OK);
    nepdate_date from_ad = {0};
    CHECK_EQ_INT(nepdate_from_ad(ad.year, ad.month, ad.day, &from_ad), NEPDATE_OK);
    CHECK_EQ_INT(from_ad.value, date.value);

    CHECK_EQ_INT(nepdate_weekday_of(date, &weekday), NEPDATE_OK);
    CHECK(weekday >= NEPDATE_SUNDAY && weekday <= NEPDATE_SATURDAY);
    CHECK_EQ_INT(nepdate_month_of(date, &month), NEPDATE_OK);
    CHECK(month >= NEPDATE_BAISHAKH && month <= NEPDATE_CHAITRA);
    CHECK_EQ_INT(nepdate_day_of_year(date, &day_of_year), NEPDATE_OK);
    CHECK(day_of_year >= 1 && day_of_year <= 367);
    CHECK_EQ_INT(nepdate_month_length(date, &length), NEPDATE_OK);
    CHECK(length >= 29 && length <= 32);

    CHECK_EQ_INT(nepdate_first_day_of_month(date, &first), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_last_day_of_month(date, &last), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_compare(first, date, &comparison), NEPDATE_OK);
    CHECK(comparison <= 0);
    CHECK_EQ_INT(nepdate_compare(date, last, &comparison), NEPDATE_OK);
    CHECK(comparison <= 0);
    CHECK_EQ_INT(nepdate_compare(first, last, &comparison), NEPDATE_OK);
    CHECK(comparison < 0);
    nepdate_date first_back = {0};
    nepdate_date last_back = {0};
    CHECK_EQ_INT(nepdate_to_bs(first, &ymd), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_from_bs(ymd.year, ymd.month, ymd.day, &first_back), NEPDATE_OK);
    CHECK_EQ_INT(first_back.value, first.value);
    CHECK_EQ_INT(nepdate_to_bs(last, &ymd), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_from_bs(ymd.year, ymd.month, ymd.day, &last_back), NEPDATE_OK);
    CHECK_EQ_INT(last_back.value, last.value);

    CHECK_EQ_INT(nepdate_compare(date, date, &comparison), NEPDATE_OK);
    CHECK_EQ_INT(comparison, 0);
    CHECK_EQ_INT(nepdate_compare(date, valid, &comparison), NEPDATE_OK);
    CHECK(comparison >= -1 && comparison <= 1);
    CHECK_EQ_INT(nepdate_compare(valid, date, &comparison), NEPDATE_OK);
    CHECK(comparison >= -1 && comparison <= 1);
}

static void check_pattern(uint32_t value) {
    /* 1 is the first valid value and 109212 the last (ADR-0006). */
    if ((uint32_t)(value - 1U) < 109212U) {
        check_valid(value);
    } else {
        check_invalid(value);
    }
}

TEST(all_patterns) {
#ifdef NEPDATE_C_FULL_SWEEP
    for (uint64_t value = 0; value <= UINT32_MAX; value++) {
        check_pattern((uint32_t)value);
    }
#else
    const uint32_t count = 1U << NEPDATE_C_BIT_PATTERN_BITS;
    const uint32_t shift = 32U - NEPDATE_C_BIT_PATTERN_BITS;
    for (uint32_t high = 0; high < count; high++) {
        check_pattern(high << shift);
        check_pattern((high << shift) | 0xFFU);
    }
#endif
}

int main(void) {
    CHECK_EQ_INT(nepdate_from_bs(2081, 4, 15, &anchor_date), NEPDATE_OK);
    RUN_TEST(all_patterns);
    return harness_finish();
}
