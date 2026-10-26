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

/* A text and its length in bytes, so writing it needs no scan for the terminator, which a compiler
 * could turn into a strlen call. */
typedef struct text_span {
    const char *text;
    size_t length;
} text_span;

/* clang-format off: clang-format 18.1.3 and 18.1.8 disagree on a braced macro body. */
#define TEXT(literal) {(literal), sizeof(literal) - 1U}
/* clang-format on */

/* Month and weekday names in the order of their enums (FORMATTING section 1). Devanagari strings
 * are octal escapes of their UTF-8 bytes, with the readable text in the comment. The Nepali short
 * month name is the full name: Nepali has no customary month abbreviation. */
static const text_span month_names[12][2] = {
    {TEXT("Baishakh"),
     TEXT("\340\244\254\340\245\210\340\244\266\340\244\276\340\244\226")},      /* Baishakh */
    {TEXT("Jestha"), TEXT("\340\244\234\340\245\207\340\244\240")},              /* Jestha */
    {TEXT("Ashad"), TEXT("\340\244\205\340\244\270\340\244\276\340\244\260")},   /* Ashad */
    {TEXT("Shrawan"), TEXT("\340\244\270\340\244\276\340\244\211\340\244\250")}, /* Shrawan */
    {TEXT("Bhadra"), TEXT("\340\244\255\340\244\246\340\245\214")},              /* Bhadra */
    {TEXT("Ashoj"), TEXT("\340\244\205\340\244\270\340\245\213\340\244\234")},   /* Ashoj */
    {TEXT("Kartik"),
     TEXT("\340\244\225\340\244\276\340\244\260\340\245\215\340\244\244\340\244\277\340\244\225")},
    {TEXT("Mangsir"), TEXT("\340\244\256\340\244\202\340\244\270\340\244\277\340\244\260")},
    {TEXT("Poush"), TEXT("\340\244\252\340\245\201\340\244\267")}, /* Poush */
    {TEXT("Magh"), TEXT("\340\244\256\340\244\276\340\244\230")},  /* Magh */
    {TEXT("Falgun"), TEXT("\340\244\253\340\244\276\340\244\227\340\245\201\340\244\250")},
    {TEXT("Chaitra"), TEXT("\340\244\232\340\245\210\340\244\244")}, /* Chaitra */
};

static const text_span month_short_names[12][2] = {
    {TEXT("Bai"), TEXT("\340\244\254\340\245\210\340\244\266\340\244\276\340\244\226")}, /* Bai */
    {TEXT("Jes"), TEXT("\340\244\234\340\245\207\340\244\240")},                         /* Jes */
    {TEXT("Asa"), TEXT("\340\244\205\340\244\270\340\244\276\340\244\260")},             /* Asa */
    {TEXT("Shr"), TEXT("\340\244\270\340\244\276\340\244\211\340\244\250")},             /* Shr */
    {TEXT("Bha"), TEXT("\340\244\255\340\244\246\340\245\214")},                         /* Bha */
    {TEXT("Aso"), TEXT("\340\244\205\340\244\270\340\245\213\340\244\234")},             /* Aso */
    {TEXT("Kar"),
     TEXT("\340\244\225\340\244\276\340\244\260\340\245\215\340\244\244\340\244\277\340\244\225")},
    {TEXT("Man"), TEXT("\340\244\256\340\244\202\340\244\270\340\244\277\340\244\260")},
    {TEXT("Pou"), TEXT("\340\244\252\340\245\201\340\244\267")}, /* Pou */
    {TEXT("Mag"), TEXT("\340\244\256\340\244\276\340\244\230")}, /* Mag */
    {TEXT("Fal"), TEXT("\340\244\253\340\244\276\340\244\227\340\245\201\340\244\250")},
    {TEXT("Cha"), TEXT("\340\244\232\340\245\210\340\244\244")}, /* Cha */
};

static const text_span weekday_names[7][2] = {
    {TEXT("Sunday"),
     TEXT("\340\244\206\340\244\207\340\244\244\340\244\265\340\244\276\340\244\260")},
    {TEXT("Monday"),
     TEXT("\340\244\270\340\245\213\340\244\256\340\244\265\340\244\276\340\244\260")},
    {TEXT("Tuesday"),
     TEXT("\340\244\256\340\244\231\340\245\215\340\244\227\340\244\262\340\244\265\340\244"
          "\276\340\244\260")},
    {TEXT("Wednesday"),
     TEXT("\340\244\254\340\245\201\340\244\247\340\244\265\340\244\276\340\244\260")},
    {TEXT("Thursday"),
     TEXT("\340\244\254\340\244\277\340\244\271\340\244\277\340\244\265\340\244\276\340\244\260")},
    {TEXT("Friday"),
     TEXT("\340\244\266\340\245\201\340\244\225\340\245\215\340\244\260\340\244\265\340\244"
          "\276\340\244\260")},
    {TEXT("Saturday"),
     TEXT("\340\244\266\340\244\250\340\244\277\340\244\265\340\244\276\340\244\260")},
};

/* ", " between the parts of the long format. */
static const text_span comma_space = TEXT(", ");

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
/* NOLINTBEGIN(clang-analyzer-core.UndefinedBinaryOperatorResult,
 * clang-analyzer-core.uninitialized.Assign): the generator proves the estimate is in range, and
 * month 0 starts at day count 0, so i never steps below 0. */
static uint32_t month_index_of(uint32_t s) {
    uint32_t i = (uint32_t)(((uint64_t)s * NEPDATE_CAL_ESTIMATE_MUL) >> NEPDATE_CAL_ESTIMATE_SHIFT);
    if (nepdate_cal_month_start[i] > s) {
        i--;
    } else if (nepdate_cal_month_start[i + 1] <= s) {
        i++;
    }
    return i;
}
/* NOLINTEND(clang-analyzer-core.UndefinedBinaryOperatorResult,
 * clang-analyzer-core.uninitialized.Assign) */

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
    /* NOLINTBEGIN(clang-analyzer-core.UndefinedBinaryOperatorResult,
     * clang-analyzer-core.uninitialized.Assign): landed is never past a month that is also the
     * first month, so last_month is never 0 here. */
    if (landed > last) {
        months--;
        last_month--;
        length = nepdate_cal_month_length[last_month];
        landed_day = day < length ? day : length;
        landed = (int32_t)nepdate_cal_month_start[last_month] + (int32_t)landed_day - 1;
    }
    /* NOLINTEND(clang-analyzer-core.UndefinedBinaryOperatorResult,
     * clang-analyzer-core.uninitialized.Assign) */
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

/* Writes text and returns the new position. */
static size_t write_text(char *buffer, size_t size, size_t position, text_span text) {
    for (size_t i = 0; i < text.length; i++) {
        put_byte(buffer, size, position, text.text[i]);
        position++;
    }
    return position;
}

/* True when text is exactly literal. */
static bool text_is(const char *text, const char *literal) {
    while (*text != '\0' && *text == *literal) {
        text++;
        literal++;
    }
    return *text == *literal;
}

static char separator_char(nepdate_separator separator) {
    switch (separator) {
    case NEPDATE_SEP_BACKSLASH:
        return '\\';
    case NEPDATE_SEP_DOT:
        return '.';
    case NEPDATE_SEP_UNDERSCORE:
        return '_';
    case NEPDATE_SEP_DASH:
        return '-';
    case NEPDATE_SEP_SPACE:
        return ' ';
    case NEPDATE_SEP_SLASH:
        break;
    }
    return '/';
}

/* The three short-format fields in the order of order (FORMATTING section 3). Precondition: date is
 * a date this library made. */
static size_t write_short(char *buffer, size_t size, size_t position, nepdate_date date,
                          nepdate_order order, nepdate_separator separator, bool pad,
                          nepdate_lang lang) {
    const uint32_t index = month_index_of((uint32_t)day_count(date));
    const uint32_t year = (uint32_t)(NEPDATE_CAL_MIN_YEAR + (int)(index / 12U));
    const uint32_t month = index % 12U + 1U;
    const uint32_t day = (uint32_t)day_count(date) - nepdate_cal_month_start[index] + 1U;
    uint32_t first = year;
    uint32_t second = month;
    uint32_t third = day;
    const unsigned year_width = pad ? 4U : 1U;
    const unsigned field_width = pad ? 2U : 1U;
    unsigned first_width = year_width;
    unsigned second_width = field_width;
    unsigned third_width = field_width;
    switch (order) {
    case NEPDATE_YDM:
        second = day;
        third = month;
        break;
    case NEPDATE_MYD:
        first = month;
        second = year;
        first_width = field_width;
        second_width = year_width;
        break;
    case NEPDATE_MDY:
        first = month;
        second = day;
        third = year;
        first_width = field_width;
        third_width = year_width;
        break;
    case NEPDATE_DYM:
        first = day;
        second = year;
        third = month;
        first_width = field_width;
        second_width = year_width;
        break;
    case NEPDATE_DMY:
        first = day;
        third = year;
        first_width = field_width;
        third_width = year_width;
        break;
    case NEPDATE_YMD:
        break;
    }
    const char separator_byte = separator_char(separator);
    position = put_number(buffer, size, position, first, first_width, lang);
    put_byte(buffer, size, position, separator_byte);
    position++;
    position = put_number(buffer, size, position, second, second_width, lang);
    put_byte(buffer, size, position, separator_byte);
    position++;
    return put_number(buffer, size, position, third, third_width, lang);
}

/* FORMATTING section 4. Precondition: date is a date this library made. */
static size_t write_long(char *buffer, size_t size, size_t position, nepdate_date date,
                         nepdate_long_options options, nepdate_lang lang) {
    const uint32_t serial = (uint32_t)day_count(date);
    const uint32_t index = month_index_of(serial);
    const unsigned language = lang == NEPDATE_NEPALI ? 1U : 0U;
    if (options.weekday) {
        position = write_text(buffer, size, position, weekday_names[(serial + 4U) % 7U][language]);
        position = write_text(buffer, size, position, comma_space);
    }
    position = write_text(buffer, size, position, month_names[index % 12U][language]);
    put_byte(buffer, size, position, ' ');
    position++;
    position = put_number(buffer, size, position, serial - nepdate_cal_month_start[index] + 1U,
                          options.pad ? 2U : 1U, lang);
    if (options.year) {
        position = write_text(buffer, size, position, comma_space);
        position = put_number(buffer, size, position,
                              (uint32_t)(NEPDATE_CAL_MIN_YEAR + (int)(index / 12U)), 4U, lang);
    }
    return position;
}

/* Writes one run of y, M or d (FORMATTING section 5). */
static size_t write_pattern_token(char *buffer, size_t size, size_t position, char token,
                                  size_t run, uint32_t year, uint32_t month, uint32_t day,
                                  unsigned language, nepdate_lang lang) {
    if (token == 'y') {
        return put_number(buffer, size, position, run >= 4U ? year : year % 100U,
                          run >= 4U ? 4U : 2U, lang);
    }
    if (token == 'M') {
        if (run >= 4U) {
            return write_text(buffer, size, position, month_names[month - 1U][language]);
        }
        if (run == 3U) {
            return write_text(buffer, size, position, month_short_names[month - 1U][language]);
        }
        return put_number(buffer, size, position, month, (unsigned)run, lang);
    }
    return put_number(buffer, size, position, day, run == 2U ? 2U : 1U, lang);
}

/* FORMATTING section 5, a left-to-right scan after the standard patterns. Precondition: date is a
 * date this library made. */
static size_t write_pattern(char *buffer, size_t size, size_t position, nepdate_date date,
                            const char *pattern, nepdate_lang lang) {
    const uint32_t index = month_index_of((uint32_t)day_count(date));
    const uint32_t year = (uint32_t)(NEPDATE_CAL_MIN_YEAR + (int)(index / 12U));
    const uint32_t month = index % 12U + 1U;
    const uint32_t day = (uint32_t)day_count(date) - nepdate_cal_month_start[index] + 1U;
    const unsigned language = lang == NEPDATE_NEPALI ? 1U : 0U;
    size_t i = 0;
    while (pattern[i] != '\0') {
        const char c = pattern[i];
        if (c == '\\') {
            if (pattern[i + 1] == '\0') {
                break;
            }
            put_byte(buffer, size, position, pattern[i + 1]);
            position++;
            i += 2;
        } else if (c == '\'') {
            size_t close = i + 1U;
            while (pattern[close] != '\0' && pattern[close] != '\'') {
                close++;
            }
            size_t j = i + 1U;
            while (j < close) {
                put_byte(buffer, size, position, pattern[j]);
                position++;
                j++;
            }
            i = pattern[close] == '\'' ? close + 1U : close;
        } else if (c == 'y' || c == 'M' || c == 'd') {
            size_t end = i + 1U;
            while (pattern[end] == c) {
                end++;
            }
            const size_t run = end - i;
            i = end;
            position = write_pattern_token(buffer, size, position, c, run, year, month, day,
                                           language, lang);
        } else {
            put_byte(buffer, size, position, c);
            position++;
            i++;
        }
    }
    return position;
}

/* Ends a formatted text with NUL when there is room. */
static size_t finish_text(char *buffer, size_t size, size_t length) {
    if (buffer != NULL && size > 0) {
        buffer[length < size ? length : size - 1U] = '\0';
    }
    return length;
}

size_t nepdate_format(nepdate_date date, const char *pattern, nepdate_lang lang, char *buffer,
                      size_t size) {
    if (day_count(date) < 0) {
        return 0;
    }
    if (pattern == NULL || pattern[0] == '\0' || text_is(pattern, "G") || text_is(pattern, "g") ||
        text_is(pattern, "d")) {
        return finish_text(
            buffer, size,
            write_short(buffer, size, 0, date, NEPDATE_YMD, NEPDATE_SEP_SLASH, true, lang));
    }
    if (text_is(pattern, "D")) {
        const nepdate_long_options options = {false, true, true};
        return finish_text(buffer, size, write_long(buffer, size, 0, date, options, lang));
    }
    if (text_is(pattern, "s")) {
        return finish_text(
            buffer, size,
            write_short(buffer, size, 0, date, NEPDATE_YMD, NEPDATE_SEP_DASH, true, lang));
    }
    return finish_text(buffer, size, write_pattern(buffer, size, 0, date, pattern, lang));
}

size_t nepdate_format_short(nepdate_date date, nepdate_order order, nepdate_separator separator,
                            bool pad, nepdate_lang lang, char *buffer, size_t size) {
    if (day_count(date) < 0) {
        return 0;
    }
    return finish_text(buffer, size,
                       write_short(buffer, size, 0, date, order, separator, pad, lang));
}

size_t nepdate_format_long(nepdate_date date, nepdate_long_options options, nepdate_lang lang,
                           char *buffer, size_t size) {
    if (day_count(date) < 0) {
        return 0;
    }
    return finish_text(buffer, size, write_long(buffer, size, 0, date, options, lang));
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

/* The month spellings of PARSING section 3, folded as ASCII letters only. The Devanagari entries
 * are the UTF-8 bytes of the readable form. */
typedef struct month_spelling {
    const char *text;
    unsigned char month;
} month_spelling;

static const month_spelling month_spellings[] = {
    {"baishakh", 1},
    {"baisakh", 1},
    {"baishak", 1},
    {"vaishakh", 1},
    {"vaisakh", 1},
    {"bai", 1},
    {"\340\244\254\340\245\210\340\244\266\340\244\276\340\244\226", 1}, /* Baishakh */
    {"\340\244\265\340\245\210\340\244\266\340\244\276\340\244\226", 1}, /* Vaishakh */
    {"\340\244\254\340\245\210\340\244\270\340\244\276\340\244\226", 1}, /* Baisakh */
    {"jestha", 2},
    {"jeth", 2},
    {"jyestha", 2},
    {"jes", 2},
    {"\340\244\234\340\245\207\340\244\240", 2},                         /* Jeth */
    {"\340\244\234\340\245\207\340\244\267\340\245\215\340\244\240", 2}, /* Jyeshtha */
    {"\340\244\234\340\245\215\340\244\257\340\245\207\340\244\267\340\245\215\340\244\240",
     2}, /* Jyestha */
    {"ashad", 3},
    {"ashadh", 3},
    {"asar", 3},
    {"asadh", 3},
    {"asa", 3},
    {"\340\244\205\340\244\270\340\244\276\340\244\260", 3}, /* Asar */
    {"\340\244\206\340\244\267\340\244\276\340\244\242", 3}, /* Ashadh */
    {"\340\244\205\340\244\270\340\244\276\340\244\242", 3}, /* Asadh */
    {"shrawan", 4},
    {"shravan", 4},
    {"srawan", 4},
    {"sawan", 4},
    {"saun", 4},
    {"shr", 4},
    {"\340\244\270\340\244\276\340\244\211\340\244\250", 4},                         /* Saun */
    {"\340\244\266\340\245\215\340\244\260\340\244\276\340\244\265\340\244\243", 4}, /* Shrawan */
    {"\340\244\270\340\244\276\340\244\265\340\244\250", 4},                         /* Sawan */
    {"bhadra", 5},
    {"bhadau", 5},
    {"bha", 5},
    {"\340\244\255\340\244\246\340\245\214", 5},                         /* Bhadau */
    {"\340\244\255\340\244\276\340\244\246\340\245\215\340\244\260", 5}, /* Bhadra */
    {"\340\244\255\340\244\276\340\244\246\340\245\215\340\244\260\340\244\252\340\244\246",
     5}, /* Bhadrapad */
    {"ashoj", 6},
    {"asoj", 6},
    {"ashwin", 6},
    {"aso", 6},
    {"\340\244\205\340\244\270\340\245\213\340\244\234", 6},                         /* Asoj */
    {"\340\244\206\340\244\266\340\245\215\340\244\265\340\244\277\340\244\250", 6}, /* Ashwin */
    {"\340\244\205\340\244\270\340\245\214\340\244\234", 6},                         /* Asauj */
    {"kartik", 7},
    {"kattik", 7},
    {"kar", 7},
    {"\340\244\225\340\244\276\340\244\260\340\245\215\340\244\244\340\244\277\340\244\225",
     7}, /* Kartik */
    {"\340\244\225\340\244\276\340\244\244\340\245\215\340\244\244\340\244\277\340\244\225",
     7}, /* Kattik */
    {"mangsir", 8},
    {"mansir", 8},
    {"margashirsha", 8},
    {"man", 8},
    {"\340\244\256\340\244\202\340\244\270\340\244\277\340\244\260", 8}, /* Mangsir */
    {"\340\244\256\340\244\231\340\245\215\340\244\270\340\244\277\340\244\260",
     8}, /* Mangsir with nga */
    {"\340\244\256\340\244\276\340\244\260\340\245\215\340\244\227\340\244\266\340\245\200\340\244"
     "\260\340\245\215\340\244\267",
     8}, /* Margashirsha */
    {"poush", 9},
    {"paush", 9},
    {"push", 9},
    {"pus", 9},
    {"pou", 9},
    {"\340\244\252\340\245\201\340\244\267", 9}, /* Push */
    {"\340\244\252\340\245\214\340\244\267", 9}, /* Paush */
    {"\340\244\252\340\245\201\340\244\270", 9}, /* Pus */
    {"magh", 10},
    {"mag", 10},
    {"\340\244\256\340\244\276\340\244\230", 10}, /* Magh */
    {"falgun", 11},
    {"phalgun", 11},
    {"fagun", 11},
    {"fal", 11},
    {"\340\244\253\340\244\276\340\244\227\340\245\201\340\244\250", 11}, /* Fagun */
    {"\340\244\253\340\244\276\340\244\262\340\245\215\340\244\227\340\245\201\340\244\250",
     11}, /* Phalgun */
    {"chaitra", 12},
    {"chait", 12},
    {"cha", 12},
    {"\340\244\232\340\245\210\340\244\244", 12},                         /* Chait */
    {"\340\244\232\340\245\210\340\244\244\340\245\215\340\244\260", 12}, /* Chaitra */
};

/* The era markers lenient parsing drops (PARSING section 2), compared folded. */
static const char *const era_markers[] = {
    "bs",
    "b.s.",
    "b.s",
    "vs",
    "v.s.",
    "v.s",
    "\340\244\254\340\244\277.\340\244\270\340\244\202.", /* B.S. marker */
    "\340\244\265\340\244\277.\340\244\270\340\244\202.", /* V.S. marker */
    "\340\244\254\340\244\277.\340\244\270\340\244\202",  /* B.S. marker without dot */
    "\340\244\265\340\244\277.\340\244\270\340\244\202",  /* V.S. marker without dot */
};

/* The two words lenient parsing drops, compared exactly. */
static const text_span dropped_words[] = {
    TEXT("\340\244\227\340\244\244\340\245\207"),             /* gate */
    TEXT("\340\244\256\340\244\277\340\244\244\340\244\277"), /* miti */
};

#undef TEXT

/* True when text[0..length) is exactly entry. */
static bool exact_equal(const char *text, size_t length, text_span entry) {
    if (length != entry.length) {
        return false;
    }
    for (size_t i = 0; i < length; i++) {
        if (text[i] != entry.text[i]) {
            return false;
        }
    }
    return true;
}

/* PARSING section 2 case folding: ASCII A-Z and the Kelvin sign U+212A, nothing else. */
static bool folded_equal(const char *text, size_t length, const char *entry) {
    size_t i = 0;
    size_t j = 0;
    while (i < length) {
        const unsigned char c = (unsigned char)text[i];
        char folded = (char)c;
        size_t size = 1;
        if (c >= 'A' && c <= 'Z') {
            folded = (char)(c - 'A' + 'a');
        } else if (c == 0xE2U && length - i >= 3U && (unsigned char)text[i + 1] == 0x84U &&
                   (unsigned char)text[i + 2] == 0xAAU) {
            folded = 'k';
            size = 3;
        }
        if (entry[j] == '\0' || entry[j] != folded) {
            return false;
        }
        i += size;
        j++;
    }
    return entry[j] == '\0';
}

/* True for a Devanagari digit U+0966..U+096F at text[i], with i < length. */
static bool devanagari_digit(const char *text, size_t length, size_t i, unsigned *digit) {
    if ((unsigned char)text[i] != 0xE0U || length - i < 3U || (unsigned char)text[i + 1] != 0xA5U) {
        return false;
    }
    const unsigned char third = (unsigned char)text[i + 2];
    if (third < 0xA6U || third > 0xAFU) {
        return false;
    }
    *digit = (unsigned)(third - 0xA6U);
    return true;
}

static bool ascii_digit(char c, unsigned *digit) {
    if (c < '0' || c > '9') {
        return false;
    }
    *digit = (unsigned)(c - '0');
    return true;
}

/* The size in bytes of a lenient first-pass separator at text[i], or 0. */
static size_t lenient_separator_size(const char *text, size_t length, size_t i) {
    const unsigned char c = (unsigned char)text[i];
    if (c == 0xE0U && length - i >= 3U && (unsigned char)text[i + 1] == 0xA5U &&
        (unsigned char)text[i + 2] == 0xA4U) {
        return 3;
    }
    switch (c) {
    case '-':
    case '/':
    case '_':
    case '\\':
    case ' ':
    case '|':
    case ',':
        return 1;
    default:
        return 0;
    }
}

/* True when text[0..length) is only ASCII digits, Devanagari digits and dots. */
static bool digits_and_dots(const char *text, size_t length) {
    size_t i = 0;
    while (i < length) {
        const unsigned char c = (unsigned char)text[i];
        if (c == '.' || (c >= '0' && c <= '9')) {
            i++;
            continue;
        }
        unsigned digit = 0;
        if (!devanagari_digit(text, length, i, &digit)) {
            return false;
        }
        (void)digit;
        i += 3;
    }
    return true;
}

/* The digit count of a piece made only of digits, and the value of its first four digits, so a long
 * piece can't overflow. */
typedef struct digit_run {
    size_t count;
    unsigned value;
} digit_run;

static digit_run read_digits(const char *text, size_t length) {
    size_t count = 0;
    unsigned number = 0;
    size_t i = 0;
    while (i < length) {
        unsigned digit = 0;
        size_t size = 1;
        if (!ascii_digit(text[i], &digit)) {
            (void)devanagari_digit(text, length, i, &digit);
            size = 3;
        }
        i += size;
        count++;
        if (count <= 4U) {
            number = number * 10U + digit;
        }
    }
    const digit_run run = {count, number};
    return run;
}

typedef struct lenient_state {
    unsigned number0;
    unsigned number1;
    unsigned number2;
    size_t digits0;
    size_t digits1;
    size_t digits2;
    size_t number_count;
    unsigned month;
    size_t month_count;
    size_t unknown_count;
    bool too_long;
} lenient_state;

/* The lenient state passes by value: a local whose address reached another function would make
 * -fstack-protector-strong add a canary, and the core links no __stack_chk_fail. */

/* Appends the numbers of a digits-and-dots token, re-split on dots. */
static lenient_state append_digit_token(const char *text, size_t length, lenient_state state) {
    size_t start = 0;
    for (;;) {
        size_t end = start;
        while (end < length && text[end] != '.') {
            end++;
        }
        if (end > start) {
            const digit_run run = read_digits(text + start, end - start);
            if (run.count > 4U) {
                state.too_long = true;
                return state;
            }
            if (state.number_count == 0U) {
                state.number0 = run.value;
                state.digits0 = run.count;
            } else if (state.number_count == 1U) {
                state.number1 = run.value;
                state.digits1 = run.count;
            } else if (state.number_count == 2U) {
                state.number2 = run.value;
                state.digits2 = run.count;
            }
            state.number_count++;
        }
        if (end == length) {
            break;
        }
        start = end + 1U;
    }
    return state;
}

/* Classifies one token: a dropped word, an era marker, a month name, or an unknown word. */
static lenient_state classify_word(const char *text, size_t length, lenient_state state) {
    const size_t dropped_count = sizeof dropped_words / sizeof dropped_words[0];
    for (size_t i = 0; i < dropped_count; i++) {
        if (exact_equal(text, length, dropped_words[i])) {
            return state;
        }
    }
    const size_t era_count = sizeof era_markers / sizeof era_markers[0];
    for (size_t i = 0; i < era_count; i++) {
        if (folded_equal(text, length, era_markers[i])) {
            return state;
        }
    }
    const size_t spelling_count = sizeof month_spellings / sizeof month_spellings[0];
    for (size_t i = 0; i < spelling_count; i++) {
        if (folded_equal(text, length, month_spellings[i].text)) {
            if (state.month_count == 0) {
                state.month = month_spellings[i].month;
            }
            state.month_count++;
            return state;
        }
    }
    state.unknown_count++;
    return state;
}

/* Splits text into tokens; stops early with too_long set when a number has more than four digits.
 */
static lenient_state tokenize(const char *text, size_t length) {
    lenient_state state = {0};
    size_t i = 0;
    while (i < length) {
        const size_t separator = lenient_separator_size(text, length, i);
        if (separator != 0) {
            i += separator;
            continue;
        }
        const size_t start = i;
        while (i < length && lenient_separator_size(text, length, i) == 0) {
            i++;
        }
        if (digits_and_dots(text + start, i - start)) {
            state = append_digit_token(text + start, i - start, state);
            if (state.too_long) {
                return state;
            }
        } else {
            state = classify_word(text + start, i - start, state);
        }
    }
    return state;
}

/* The month-name form of PARSING section 2. */
static nepdate_status assemble_month_name(lenient_state state, nepdate_date *out) {
    unsigned year = 0;
    unsigned day = 0;
    size_t day_digits = 0;
    if (state.digits0 == 4U && state.digits1 != 4U) {
        year = state.number0;
        day = state.number1;
        day_digits = state.digits1;
    } else if (state.digits1 == 4U && state.digits0 != 4U) {
        year = state.number1;
        day = state.number0;
        day_digits = state.digits0;
    } else {
        return NEPDATE_E_AMBIGUOUS;
    }
    if (day_digits > 2U) {
        return NEPDATE_E_AMBIGUOUS;
    }
    return nepdate_from_bs((int)year, (int)state.month, (int)day, out);
}

/* The numeric form of PARSING section 2. */
static nepdate_status assemble_numeric(lenient_state state, nepdate_date *out) {
    if (state.digits0 == 4U) {
        return nepdate_from_bs((int)state.number0, (int)state.number1, (int)state.number2, out);
    }
    if (state.digits2 == 4U) {
        return nepdate_from_bs((int)state.number2, (int)state.number1, (int)state.number0, out);
    }
    return NEPDATE_E_AMBIGUOUS;
}

static nepdate_status assemble(lenient_state state, nepdate_date *out) {
    if (state.month_count == 1U && state.unknown_count == 0U && state.number_count == 2U) {
        return assemble_month_name(state, out);
    }
    if (state.month_count == 0U && state.unknown_count == 0U && state.number_count == 3U) {
        return assemble_numeric(state, out);
    }
    return NEPDATE_E_UNRECOGNIZED;
}

/* One scanned character of strict parsing: a digit, a separator, or anything else. A Devanagari
 * sequence cut off by the end of the input is "other". */
enum scanned_kind { SCANNED_DIGIT, SCANNED_SEPARATOR, SCANNED_OTHER };

typedef struct scanned_char {
    enum scanned_kind kind;
    unsigned digit;
    size_t size;
} scanned_char;

static scanned_char scan_strict(const char *text, size_t length, size_t i) {
    scanned_char result = {SCANNED_OTHER, 0, 1};
    const unsigned char c = (unsigned char)text[i];
    if (ascii_digit((char)c, &result.digit)) {
        result.kind = SCANNED_DIGIT;
        return result;
    }
    if (c == 0xE0U && length - i >= 3U && (unsigned char)text[i + 1] == 0xA5U) {
        const unsigned char third = (unsigned char)text[i + 2];
        if (third >= 0xA6U && third <= 0xAFU) {
            result.kind = SCANNED_DIGIT;
            result.digit = (unsigned)(third - 0xA6U);
            result.size = 3;
            return result;
        }
        if (third == 0xA4U) {
            result.kind = SCANNED_SEPARATOR;
            result.size = 3;
            return result;
        }
    }
    switch (c) {
    case '-':
    case '/':
    case '.':
    case '_':
    case '\\':
    case ' ':
    case '|':
        result.kind = SCANNED_SEPARATOR;
        break;
    default:
        break;
    }
    return result;
}

static nepdate_status parse_strict(const char *text, size_t length, nepdate_date *out) {
    unsigned group0 = 0;
    unsigned group1 = 0;
    unsigned group2 = 0;
    size_t group = 0;
    size_t digits = 0;
    size_t i = 0;
    while (i < length) {
        const scanned_char c = scan_strict(text, length, i);
        i += c.size;
        if (c.kind == SCANNED_DIGIT) {
            if (group == 3U) {
                return NEPDATE_E_WRONG_GROUP_COUNT;
            }
            if (digits == 4U) {
                return NEPDATE_E_NUMBER_TOO_LONG;
            }
            if (group == 0U) {
                group0 = group0 * 10U + c.digit;
            } else if (group == 1U) {
                group1 = group1 * 10U + c.digit;
            } else {
                group2 = group2 * 10U + c.digit;
            }
            digits++;
        } else if (c.kind == SCANNED_SEPARATOR) {
            if (digits > 0U) {
                group++;
                digits = 0;
            }
        } else {
            return NEPDATE_E_INVALID_CHARACTER;
        }
    }
    if (digits > 0U) {
        group++;
    }
    if (group < 3U) {
        return NEPDATE_E_WRONG_GROUP_COUNT;
    }
    return nepdate_from_bs((int)group0, (int)group1, (int)group2, out);
}

nepdate_status nepdate_parse(const char *text, size_t len, nepdate_date *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    if (text == NULL) {
        return NEPDATE_E_NULL;
    }
    return parse_strict(text, len, out);
}

nepdate_status nepdate_parse_lenient(const char *text, size_t len, nepdate_date *out) {
    if (out == NULL) {
        return NEPDATE_E_NULL;
    }
    if (text == NULL) {
        return NEPDATE_E_NULL;
    }
    /* parse_strict leaves *out unchanged on error, so it can write there directly. */
    if (parse_strict(text, len, out) == NEPDATE_OK) {
        return NEPDATE_OK;
    }
    const lenient_state state = tokenize(text, len);
    if (state.too_long) {
        return NEPDATE_E_NUMBER_TOO_LONG;
    }
    return assemble(state, out);
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
