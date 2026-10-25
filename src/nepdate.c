#include "nepdate.h"

#include "generated/calendar_tables.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

_Static_assert(NEPDATE_CAL_MIN_YEAR == NEPDATE_MIN_YEAR, "generated tables and header disagree");
_Static_assert(NEPDATE_CAL_MAX_YEAR == NEPDATE_MAX_YEAR, "generated tables and header disagree");
_Static_assert(sizeof(nepdate_date) == 4, "nepdate_date is a 4-byte value (ADR-0002)");

enum {
    YEARS = NEPDATE_CAL_MAX_YEAR - NEPDATE_CAL_MIN_YEAR + 1,
    MONTHS = YEARS * 12,
    /* nepdate_date.value of the last supported day; the first is 1. */
    LAST_VALUE = NEPDATE_CAL_DAYS,
    /* The last AD year with a supported day: the AD table ends at 2143-04. */
    AD_LAST_YEAR = NEPDATE_CAL_AD_FIRST_YEAR + (NEPDATE_CAL_AD_MONTHS - 1) / 12,
    /* Neri and Schneider shift and constants (ADR-0006): the shift moves every supported year to
     * a positive count of 400-year eras, so the arithmetic stays unsigned 32-bit. */
    CIVIL_SHIFT = 82,
    CIVIL_K = 719468 + 146097 * CIVIL_SHIFT,
    CIVIL_L = 400 * CIVIL_SHIFT
};

const char *nepdate_version(void) { return NEPDATE_VERSION_STRING; }

const char *nepdate_status_name(nepdate_status status) {
    switch (status) {
    case NEPDATE_OK:
        return "NEPDATE_OK";
    case NEPDATE_E_OUT_OF_RANGE:
        return "NEPDATE_E_OUT_OF_RANGE";
    case NEPDATE_E_INVALID_MONTH:
        return "NEPDATE_E_INVALID_MONTH";
    case NEPDATE_E_INVALID_DAY:
        return "NEPDATE_E_INVALID_DAY";
    case NEPDATE_E_INVALID_GREGORIAN:
        return "NEPDATE_E_INVALID_GREGORIAN";
    case NEPDATE_E_INVALID_CHARACTER:
        return "NEPDATE_E_INVALID_CHARACTER";
    case NEPDATE_E_WRONG_GROUP_COUNT:
        return "NEPDATE_E_WRONG_GROUP_COUNT";
    case NEPDATE_E_NUMBER_TOO_LONG:
        return "NEPDATE_E_NUMBER_TOO_LONG";
    case NEPDATE_E_AMBIGUOUS:
        return "NEPDATE_E_AMBIGUOUS";
    case NEPDATE_E_UNRECOGNIZED:
        return "NEPDATE_E_UNRECOGNIZED";
    case NEPDATE_E_INVALID_DATE:
        return "NEPDATE_E_INVALID_DATE";
    case NEPDATE_E_NULL:
        return "NEPDATE_E_NULL";
    default:
        return "unknown";
    }
}

/* The day count (days since BS 1901-01-01) of a date, or -1 for a value no function made. Every
 * caller checks the result before it indexes a table. */
static int32_t day_count(nepdate_date date) {
    /* Unsigned arithmetic, so INT32_MIN and INT32_MAX wrap instead of overflowing. */
    const uint32_t s = (uint32_t)date.value - 1U;
    return s < (uint32_t)LAST_VALUE ? (int32_t)s : -1;
}

/* Month index (year - 1901) * 12 + month - 1 of day count s: an estimate that the generator
 * proves is at most one month off, then one correction. Precondition: s is a valid day count. */
static uint32_t month_index_of(uint32_t s) {
    uint32_t i = (uint32_t)(((uint64_t)s * NEPDATE_CAL_ESTIMATE_MUL) >> NEPDATE_CAL_ESTIMATE_SHIFT);
    if (nepdate_cal_month_start[i] > s) {
        i--;
    } else if (nepdate_cal_month_start[i + 1] <= s) {
        i++;
    }
    return i;
}

nepdate_status nepdate_from_bs(int year, int month, int day, nepdate_date *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    /* Casts to unsigned before subtracting, so INT_MIN and INT_MAX can't overflow. */
    const unsigned y = (unsigned)year - (unsigned)NEPDATE_CAL_MIN_YEAR;
    if (y >= (unsigned)YEARS) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    const unsigned m = (unsigned)month - 1U;
    if (m >= 12U) {
        return NEPDATE_E_INVALID_MONTH;
    }
    const unsigned i = y * 12U + m;
    const unsigned d = (unsigned)day - 1U;
    if (d >= nepdate_cal_month_length[i]) {
        return NEPDATE_E_INVALID_DAY;
    }
    out->value = (int32_t)(nepdate_cal_month_start[i] + d + 1U);
    return NEPDATE_OK;
}

nepdate_status nepdate_to_bs(nepdate_date date, nepdate_ymd *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t s = day_count(date);
    if (s < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    const uint32_t i = month_index_of((uint32_t)s);
    out->year = NEPDATE_CAL_MIN_YEAR + (int)(i / 12U);
    out->month = (int)(i % 12U) + 1;
    out->day = (int)((uint32_t)s - nepdate_cal_month_start[i]) + 1;
    return NEPDATE_OK;
}

/* Days in an AD month. Precondition: month is 1..12. No arithmetic can overflow for any int year.
 */
static int gregorian_month_length(int year, unsigned month) {
    if (month == 2U) {
        const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
        return leap ? 29 : 28;
    }
    return month == 4U || month == 6U || month == 9U || month == 11U ? 30 : 31;
}

/* Proleptic Gregorian date of a Unix day (Neri and Schneider, ADR-0006). Precondition: days is
 * -45920..63291, the supported range. */
static void civil_from_days(int32_t days, nepdate_ymd *out) {
    const uint32_t n = (uint32_t)(days + CIVIL_K);
    const uint32_t n1 = 4U * n + 3U;
    const uint32_t c = n1 / 146097U;
    const uint32_t nc = n1 % 146097U / 4U;
    const uint32_t n2 = 4U * nc + 3U;
    const uint64_t p2 = (uint64_t)2939745U * n2;
    const uint32_t z = (uint32_t)(p2 >> 32U);
    const uint32_t ny = (uint32_t)p2 / 2939745U / 4U;
    const uint32_t y = 100U * c + z;
    const uint32_t n3 = 2141U * ny + 197913U;
    const uint32_t m = n3 >> 16U;
    const uint32_t d = (n3 & 0xFFFFU) / 2141U;
    /* January and February belong to the next year. */
    const uint32_t j = ny >= 306U ? 1U : 0U;
    out->year = (int)(y + j) - CIVIL_L;
    out->month = (int)(j != 0U ? m - 12U : m);
    out->day = (int)(d + 1U);
}

nepdate_status nepdate_from_ad(int year, int month, int day, nepdate_date *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    if (month < 1 || month > 12) {
        return NEPDATE_E_INVALID_GREGORIAN;
    }
    if (day < 1 || day > gregorian_month_length(year, (unsigned)month)) {
        return NEPDATE_E_INVALID_GREGORIAN;
    }
    /* A real AD date outside these years is out of range. Checked before any index, so INT_MIN
     * and INT_MAX never reach it. */
    if (year < NEPDATE_CAL_AD_FIRST_YEAR || year > AD_LAST_YEAR) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    /* The AD month-start table runs from 1844-04 to 2143-04 (ADR-0006). */
    if (year == NEPDATE_CAL_AD_FIRST_YEAR && month < NEPDATE_CAL_AD_FIRST_MONTH) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    if (year == AD_LAST_YEAR && month > NEPDATE_CAL_AD_FIRST_MONTH) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    const unsigned j = (unsigned)(year - NEPDATE_CAL_AD_FIRST_YEAR) * 12U +
                       (unsigned)(month - NEPDATE_CAL_AD_FIRST_MONTH);
    const int32_t serial = nepdate_cal_ad_month_start[j] + day - 1;
    if (serial < 0 || serial >= NEPDATE_CAL_DAYS) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    out->value = serial + 1;
    return NEPDATE_OK;
}

nepdate_status nepdate_to_ad(nepdate_date date, nepdate_ymd *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t s = day_count(date);
    if (s < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    civil_from_days(s + NEPDATE_CAL_EPOCH_UNIX_DAYS, out);
    return NEPDATE_OK;
}

nepdate_status nepdate_weekday_of(nepdate_date date, nepdate_weekday *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t s = day_count(date);
    if (s < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    *out = (nepdate_weekday)(((uint32_t)s + 4U) % 7U);
    return NEPDATE_OK;
}

nepdate_status nepdate_month_of(nepdate_date date, nepdate_month *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t s = day_count(date);
    if (s < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    const uint32_t i = month_index_of((uint32_t)s);
    *out = (nepdate_month)(i % 12U + 1U);
    return NEPDATE_OK;
}

nepdate_status nepdate_day_of_year(nepdate_date date, int *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t s = day_count(date);
    if (s < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    const uint32_t i = month_index_of((uint32_t)s);
    const uint32_t year_start = nepdate_cal_year_start[i / 12U];
    *out = (int)((uint32_t)s - year_start + 1U);
    return NEPDATE_OK;
}

nepdate_status nepdate_month_length(nepdate_date date, int *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t s = day_count(date);
    if (s < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    const uint32_t i = month_index_of((uint32_t)s);
    *out = (int)nepdate_cal_month_length[i];
    return NEPDATE_OK;
}

nepdate_status nepdate_first_day_of_month(nepdate_date date, nepdate_date *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t s = day_count(date);
    if (s < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    const uint32_t i = month_index_of((uint32_t)s);
    out->value = (int32_t)nepdate_cal_month_start[i] + 1;
    return NEPDATE_OK;
}

nepdate_status nepdate_last_day_of_month(nepdate_date date, nepdate_date *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t s = day_count(date);
    if (s < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    const uint32_t i = month_index_of((uint32_t)s);
    out->value = (int32_t)nepdate_cal_month_start[i + 1];
    return NEPDATE_OK;
}

nepdate_status nepdate_compare(nepdate_date a, nepdate_date b, int *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t sa = day_count(a);
    const int32_t sb = day_count(b);
    if (sa < 0 || sb < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    *out = (sa > sb) - (sa < sb);
    return NEPDATE_OK;
}
