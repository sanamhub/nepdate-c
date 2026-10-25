#include "nepdate.h"

#include "generated/calendar_tables.h"

#include <stddef.h>
#include <stdint.h>

_Static_assert(NEPDATE_CAL_MIN_YEAR == NEPDATE_MIN_YEAR, "generated tables and header disagree");
_Static_assert(NEPDATE_CAL_MAX_YEAR == NEPDATE_MAX_YEAR, "generated tables and header disagree");
_Static_assert(sizeof(nepdate_date) == 4, "nepdate_date is a 4-byte value (ADR-0002)");

enum {
    YEARS = NEPDATE_CAL_MAX_YEAR - NEPDATE_CAL_MIN_YEAR + 1,
    MONTHS = YEARS * 12,
    /* nepdate_date.value of the last supported day; the first is 1. */
    LAST_VALUE = NEPDATE_CAL_DAYS
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
