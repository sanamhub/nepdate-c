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
    /* The last AD month with a supported day: the AD table ends at 2143-04. */
    AD_LAST_YEAR = NEPDATE_CAL_AD_FIRST_YEAR +
                   (NEPDATE_CAL_AD_FIRST_MONTH - 1 + NEPDATE_CAL_AD_MONTHS - 1) / 12,
    AD_LAST_MONTH = (NEPDATE_CAL_AD_FIRST_MONTH - 1 + NEPDATE_CAL_AD_MONTHS - 1) % 12 + 1
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
    if (year == AD_LAST_YEAR && month > AD_LAST_MONTH) {
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
    /* The AD month table starts at AD 1844-04, ten days before the epoch. The generated estimate
     * is never a later month than s is in and at most one earlier, so one step settles it. */
    uint32_t j = (uint32_t)(((uint64_t)(uint32_t)s * NEPDATE_CAL_AD_ESTIMATE_MUL) >>
                            NEPDATE_CAL_AD_ESTIMATE_SHIFT);
    if (j + 1U < NEPDATE_CAL_AD_MONTHS && nepdate_cal_ad_month_start[j + 1U] <= s) {
        j++;
    }
    const uint32_t month_index = j + NEPDATE_CAL_AD_FIRST_MONTH - 1U;
    out->year = NEPDATE_CAL_AD_FIRST_YEAR + (int)(month_index / 12U);
    out->month = (int)(month_index % 12U) + 1;
    out->day = (int)(s - nepdate_cal_ad_month_start[j]) + 1;
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
    out->value = (int32_t)nepdate_cal_month_start[i + 1U];
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

static bool valid_overflow(nepdate_overflow overflow) {
    return overflow == NEPDATE_CLAMP || overflow == NEPDATE_OVERFLOW;
}

/* ALGORITHM section 8 add_months after the date and overflow checks, with the month count in 64
 * bits so that 12 * INT32_MIN and 12 * INT32_MAX can't overflow. */
static nepdate_status add_months_from_serial(int32_t serial, int64_t months,
                                             nepdate_overflow overflow, nepdate_date *out) {
    const uint32_t i = month_index_of((uint32_t)serial);
    const uint32_t day = (uint32_t)serial - nepdate_cal_month_start[i] + 1U;
    const int64_t total = (int64_t)i + months;
    if (total < 0 || total >= MONTHS) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    const uint32_t target = (uint32_t)total;
    const uint32_t length = nepdate_cal_month_length[target];
    if (day <= length) {
        out->value = (int32_t)(nepdate_cal_month_start[target] + day);
        return NEPDATE_OK;
    }
    if (overflow == NEPDATE_CLAMP) {
        out->value = (int32_t)(nepdate_cal_month_start[target] + length);
        return NEPDATE_OK;
    }
    if (target + 1U >= MONTHS) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    /* The day is at most 3 days past the target month (32 - 29), so the next month has room. */
    const uint32_t spill = day - length;
    out->value = (int32_t)(nepdate_cal_month_start[target + 1U] + spill);
    return NEPDATE_OK;
}

nepdate_status nepdate_add_days(nepdate_date date, int32_t days, nepdate_date *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t serial = day_count(date);
    if (serial < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    const int64_t result = (int64_t)serial + (int64_t)days;
    if (result < 0 || result >= NEPDATE_CAL_DAYS) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    out->value = (int32_t)result + 1;
    return NEPDATE_OK;
}

nepdate_status nepdate_add_months(nepdate_date date, int32_t months, nepdate_overflow overflow,
                                  nepdate_date *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t serial = day_count(date);
    if (serial < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    if (!valid_overflow(overflow)) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    return add_months_from_serial(serial, (int64_t)months, overflow, out);
}

nepdate_status nepdate_add_years(nepdate_date date, int32_t years, nepdate_overflow overflow,
                                 nepdate_date *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t serial = day_count(date);
    if (serial < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    if (!valid_overflow(overflow)) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    return add_months_from_serial(serial, 12 * (int64_t)years, overflow, out);
}

nepdate_status nepdate_diff_days(nepdate_date a, nepdate_date b, int32_t *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t sa = day_count(a);
    const int32_t sb = day_count(b);
    if (sa < 0 || sb < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    *out = sb - sa;
    return NEPDATE_OK;
}

nepdate_status nepdate_diff(nepdate_date a, nepdate_date b, nepdate_date_diff *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t sa = day_count(a);
    const int32_t sb = day_count(b);
    if (sa < 0 || sb < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    const int sign = sa > sb ? -1 : 1;
    const int32_t first = sa < sb ? sa : sb;
    const int32_t last = sa < sb ? sb : sa;
    const uint32_t first_month = month_index_of((uint32_t)first);
    uint32_t last_month = month_index_of((uint32_t)last);
    int32_t months = (int32_t)last_month - (int32_t)first_month;
    const uint32_t day = (uint32_t)first - nepdate_cal_month_start[first_month] + 1U;
    uint32_t length = nepdate_cal_month_length[last_month];
    uint32_t landed_day = day < length ? day : length;
    int32_t landed = (int32_t)nepdate_cal_month_start[last_month] + (int32_t)landed_day - 1;
    if (landed > last) {
        months--;
        last_month--;
        length = nepdate_cal_month_length[last_month];
        landed_day = day < length ? day : length;
        landed = (int32_t)nepdate_cal_month_start[last_month] + (int32_t)landed_day - 1;
    }
    out->sign = sign;
    out->years = months / 12;
    out->months = months % 12;
    out->days = last - landed;
    return NEPDATE_OK;
}

nepdate_status nepdate_fiscal_year_of(nepdate_date date, int *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t serial = day_count(date);
    if (serial < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    const uint32_t i = month_index_of((uint32_t)serial);
    const int year = NEPDATE_CAL_MIN_YEAR + (int)(i / 12U);
    *out = i % 12U >= 3U ? year : year - 1;
    return NEPDATE_OK;
}

nepdate_status nepdate_fiscal_year_start(int fiscal_year, nepdate_date *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    return nepdate_from_bs(fiscal_year, 4, 1, out);
}

nepdate_status nepdate_fiscal_year_end(int fiscal_year, nepdate_date *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    /* The end is in the next BS year, so a fiscal year at or past the last BS year has none. The
     * check also keeps fiscal_year + 1 from overflowing. */
    if (fiscal_year >= NEPDATE_MAX_YEAR) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    const int year = fiscal_year + 1;
    if (year < NEPDATE_MIN_YEAR) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    /* year is 1901..2199 here, so Ashad exists. */
    const uint32_t ashad = (uint32_t)(year - NEPDATE_CAL_MIN_YEAR) * 12U + 2U;
    out->value = (int32_t)nepdate_cal_month_start[ashad + 1U];
    return NEPDATE_OK;
}

nepdate_status nepdate_quarter_of(nepdate_date date, nepdate_quarter *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t serial = day_count(date);
    if (serial < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    const uint32_t month = month_index_of((uint32_t)serial) % 12U + 1U;
    *out = (nepdate_quarter)((month + 8U) % 12U / 3U + 1U);
    return NEPDATE_OK;
}

/* The first BS month of each quarter and the BS year it falls in. Precondition: quarter is
 * NEPDATE_Q1..NEPDATE_Q4 and fiscal_year is usable. */
static int quarter_first_month(nepdate_quarter quarter) {
    static const int months[4] = {4, 7, 10, 1};
    return months[(unsigned)quarter - 1U];
}

/* Returns the BS year of quarter: Q1 to Q3 are in fiscal_year, Q4 in the next year. Returns false
 * when that year would be outside the supported range or would overflow. */
static bool quarter_year(int fiscal_year, nepdate_quarter quarter, int *year) {
    if (quarter == NEPDATE_Q4) {
        if (fiscal_year >= NEPDATE_MAX_YEAR) {
            return false;
        }
        *year = fiscal_year + 1;
    } else {
        *year = fiscal_year;
    }
    return true;
}

nepdate_status nepdate_quarter_start(int fiscal_year, nepdate_quarter quarter, nepdate_date *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    if ((unsigned)quarter - 1U >= 4U) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    int year = 0;
    if (!quarter_year(fiscal_year, quarter, &year)) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    return nepdate_from_bs(year, quarter_first_month(quarter), 1, out);
}

nepdate_status nepdate_quarter_end(int fiscal_year, nepdate_quarter quarter, nepdate_date *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    if ((unsigned)quarter - 1U >= 4U) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    int year = 0;
    if (!quarter_year(fiscal_year, quarter, &year)) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    if (year < NEPDATE_MIN_YEAR || year > NEPDATE_MAX_YEAR) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    const int month = quarter_first_month(quarter) + 2;
    const uint32_t i = (uint32_t)(year - NEPDATE_CAL_MIN_YEAR) * 12U + (uint32_t)(month - 1);
    out->value = (int32_t)nepdate_cal_month_start[i + 1U];
    return NEPDATE_OK;
}

/* Writes one byte when it fits before the terminator. */
static void put_byte(char *buffer, size_t size, size_t position, char byte) {
    if (buffer != NULL && size > 0 && position < size - 1) {
        buffer[position] = byte;
    }
}

static void put_digit(char *buffer, size_t size, size_t *position, unsigned digit,
                      nepdate_lang lang) {
    if (lang == NEPDATE_NEPALI) {
        /* Devanagari zero to nine, U+0966 to U+096F. */
        static const char devanagari[10][4] = {
            "\340\245\246", "\340\245\247", "\340\245\250", "\340\245\251", "\340\245\252",
            "\340\245\253", "\340\245\254", "\340\245\255", "\340\245\256", "\340\245\257",
        };
        put_byte(buffer, size, *position, devanagari[digit][0]);
        put_byte(buffer, size, *position + 1U, devanagari[digit][1]);
        put_byte(buffer, size, *position + 2U, devanagari[digit][2]);
        *position += 3U;
    } else {
        put_byte(buffer, size, *position, (char)('0' + digit));
        *position += 1U;
    }
}

/* Writes value with at least min_width digits and returns the new position. */
static size_t put_number(char *buffer, size_t size, size_t position, uint32_t value,
                         unsigned min_width, nepdate_lang lang) {
    unsigned count = 1;
    uint32_t remaining = value;
    while (remaining >= 10U) {
        remaining /= 10U;
        count++;
    }
    if (count < min_width) {
        count = min_width;
    }
    uint32_t power = 1U;
    for (unsigned i = 1; i < count; i++) {
        power *= 10U;
    }
    for (unsigned i = 0; i < count; i++) {
        put_digit(buffer, size, &position, value / power, lang);
        value %= power;
        if (power > 1U) {
            power /= 10U;
        }
    }
    return position;
}

/* A label is at most '-', ten digits of three bytes, '/', two digits of three bytes: 38 bytes. */
size_t nepdate_fiscal_year_label(int fiscal_year, nepdate_lang lang, char *buffer, size_t size) {
    size_t length = 0;
    uint32_t magnitude = 0;
    if (fiscal_year < 0) {
        put_byte(buffer, size, length, '-');
        length++;
        magnitude = 0U - (uint32_t)fiscal_year;
    } else {
        magnitude = (uint32_t)fiscal_year;
    }
    length = put_number(buffer, size, length, magnitude, 1, lang);
    put_byte(buffer, size, length, '/');
    length++;
    const int64_t next = (int64_t)fiscal_year + 1;
    const uint64_t next_magnitude = next < 0 ? (uint64_t)(-next) : (uint64_t)next;
    length = put_number(buffer, size, length, (uint32_t)(next_magnitude % 100U), 2, lang);

    if (buffer == NULL || size == 0) {
        return length;
    }
    buffer[length < size ? length : size - 1] = '\0';
    return length;
}

/* True when range holds two dates this library made, with start not after end. */
static bool range_serials(nepdate_range range, int32_t *first, int32_t *last) {
    const int32_t start = day_count(range.start);
    const int32_t end = day_count(range.end);
    if (start < 0 || end < 0 || end < start) {
        return false;
    }
    *first = start;
    *last = end;
    return true;
}

nepdate_status nepdate_range_make(nepdate_date start, nepdate_date end, nepdate_range *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    const int32_t first = day_count(start);
    const int32_t last = day_count(end);
    if (first < 0 || last < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    if (last < first) {
        return NEPDATE_E_OUT_OF_RANGE;
    }
    out->start = start;
    out->end = end;
    return NEPDATE_OK;
}

nepdate_status nepdate_range_of_month(int year, int month, nepdate_range *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    nepdate_date first = {0};
    const nepdate_status status = nepdate_from_bs(year, month, 1, &first);
    if (status != NEPDATE_OK) {
        return status;
    }
    const uint32_t i = (uint32_t)(year - NEPDATE_CAL_MIN_YEAR) * 12U + (uint32_t)(month - 1);
    out->start = first;
    out->end.value = (int32_t)nepdate_cal_month_start[i + 1U];
    return NEPDATE_OK;
}

nepdate_status nepdate_range_of_year(int year, nepdate_range *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    nepdate_date first = {0};
    const nepdate_status status = nepdate_from_bs(year, 1, 1, &first);
    if (status != NEPDATE_OK) {
        return status;
    }
    const uint32_t y = (uint32_t)(year - NEPDATE_CAL_MIN_YEAR);
    out->start = first;
    out->end.value = (int32_t)nepdate_cal_year_start[y + 1U];
    return NEPDATE_OK;
}

nepdate_status nepdate_range_of_fiscal_year(int fiscal_year, nepdate_range *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    nepdate_date first = {0};
    nepdate_status status = nepdate_fiscal_year_start(fiscal_year, &first);
    if (status != NEPDATE_OK) {
        return status;
    }
    nepdate_date last = {0};
    status = nepdate_fiscal_year_end(fiscal_year, &last);
    if (status != NEPDATE_OK) {
        return status;
    }
    out->start = first;
    out->end = last;
    return NEPDATE_OK;
}

nepdate_status nepdate_range_length(nepdate_range range, int *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    int32_t first = 0;
    int32_t last = 0;
    if (!range_serials(range, &first, &last)) {
        return NEPDATE_E_INVALID_DATE;
    }
    *out = last - first + 1;
    return NEPDATE_OK;
}

nepdate_status nepdate_range_contains(nepdate_range range, nepdate_date date, bool *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    int32_t first = 0;
    int32_t last = 0;
    if (!range_serials(range, &first, &last)) {
        return NEPDATE_E_INVALID_DATE;
    }
    const int32_t serial = day_count(date);
    if (serial < 0) {
        return NEPDATE_E_INVALID_DATE;
    }
    *out = serial >= first && serial <= last;
    return NEPDATE_OK;
}

nepdate_status nepdate_range_begin(nepdate_range range, nepdate_range_iterator *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    int32_t first = 0;
    int32_t last = 0;
    if (!range_serials(range, &first, &last)) {
        return NEPDATE_E_INVALID_DATE;
    }
    out->current = first + 1;
    out->last = last + 1;
    return NEPDATE_OK;
}

bool nepdate_range_next(nepdate_range_iterator *iterator, nepdate_date *out) {
    if (iterator == NULL || out == NULL) {
        return false;
    }
    const uint32_t current = (uint32_t)iterator->current - 1U;
    const uint32_t last = (uint32_t)iterator->last - 1U;
    if (current > (uint32_t)LAST_VALUE - 1U || last > (uint32_t)LAST_VALUE - 1U || current > last) {
        return false;
    }
    out->value = iterator->current;
    iterator->current++;
    return true;
}
