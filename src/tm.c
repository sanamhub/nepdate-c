/* The optional struct tm bridge. This is the only core source that includes <time.h>, so a
 * freestanding build can leave it out with NEPDATE_C_WITH_TM=OFF. */
#include "nepdate.h"

#include "generated/calendar_tables.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

static bool leap_year(int year) { return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0; }

static int gregorian_month_length(int year, int month) {
    if (month == 2) {
        return leap_year(year) ? 29 : 28;
    }
    return month == 4 || month == 6 || month == 9 || month == 11 ? 30 : 31;
}

static int64_t floor_div(int64_t value, int64_t divisor) {
    int64_t quotient = value / divisor;
    if (value % divisor < 0) {
        quotient--;
    }
    return quotient;
}

/* Days since 1970-01-01 of a proleptic Gregorian date. */
static int64_t days_from_civil(int64_t year, int64_t month, int64_t day) {
    const int64_t y = month <= 2 ? year - 1 : year;
    const int64_t era = floor_div(y, 400);
    const int64_t year_of_era = y - era * 400;
    const int64_t month_prime = (month + 9) % 12;
    const int64_t day_of_year = (153 * month_prime + 2) / 5 + day - 1;
    const int64_t day_of_era =
        year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
    return era * 146097 + day_of_era - 719468;
}

nepdate_status nepdate_from_tm(const struct tm *tm, nepdate_date *out) {
    if (out == NULL || tm == NULL) {
        return NEPDATE_E_NULL;
    }
    const int64_t year = (int64_t)tm->tm_year + 1900;
    /* tm_year is an int, so the sum can only be too large, never below INT_MIN. */
    if (year > INT_MAX) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    if (tm->tm_mon < 0 || tm->tm_mon > 11) {
        return NEPDATE_E_INVALID_GREGORIAN;
    }
    const int month = tm->tm_mon + 1;
    if (tm->tm_mday < 1 || tm->tm_mday > gregorian_month_length((int)year, month)) {
        return NEPDATE_E_INVALID_GREGORIAN;
    }
    const int64_t value =
        days_from_civil(year, month, tm->tm_mday) - (int64_t)NEPDATE_CAL_EPOCH_UNIX_DAYS + 1;
    if (value < 1 || value > NEPDATE_CAL_DAYS) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    out->value = (int32_t)value;
    return NEPDATE_OK;
}

/* The 0-based day of the AD year of a valid Gregorian date. */
static int ad_day_of_year(int year, int month, int day) {
    static const int month_start[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int result = month_start[month - 1] + day - 1;
    if (month > 2 && leap_year(year)) {
        result++;
    }
    return result;
}

nepdate_status nepdate_to_tm(nepdate_date date, struct tm *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const uint32_t serial = (uint32_t)date.value - 1U;
    if (serial >= (uint32_t)NEPDATE_CAL_DAYS) {
        return NEPDATE_E_INVALID_DATE;
    }
    /* The same AD month estimate as nepdate_to_ad, done without taking the address of a local,
     * so a build with -fstack-protector-strong needs no __stack_chk_fail. */
    uint32_t index = (uint32_t)(((uint64_t)serial * NEPDATE_CAL_AD_ESTIMATE_MUL) >>
                                NEPDATE_CAL_AD_ESTIMATE_SHIFT);
    /* The generator proves the estimate is at most one month early. The estimate can be the last
     * month itself, so there may be no next start to read. */
    if (index + 1U < NEPDATE_CAL_AD_MONTHS &&
        nepdate_cal_ad_month_start[index + 1U] <= (int32_t)serial) {
        index++;
    }
    const int ad_year =
        NEPDATE_CAL_AD_FIRST_YEAR + (int)((index + NEPDATE_CAL_AD_FIRST_MONTH - 1U) / 12U);
    const int ad_month = (int)((index + NEPDATE_CAL_AD_FIRST_MONTH - 1U) % 12U) + 1;
    const int ad_day = (int)(serial - (uint32_t)nepdate_cal_ad_month_start[index]) + 1;
    out->tm_year = ad_year - 1900;
    out->tm_mon = ad_month - 1;
    out->tm_mday = ad_day;
    out->tm_hour = 0;
    out->tm_min = 0;
    out->tm_sec = 0;
    out->tm_wday = (int)((serial + 4U) % 7U);
    out->tm_yday = ad_day_of_year(ad_year, ad_month, ad_day);
    out->tm_isdst = -1;
    return NEPDATE_OK;
}
