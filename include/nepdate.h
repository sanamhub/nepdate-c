/* nepdate-c: Bikram Sambat (Nepali calendar) dates for C. The public surface is docs/API.md. */
#ifndef NEPDATE_H
#define NEPDATE_H

#define NEPDATE_VERSION_MAJOR 0
#define NEPDATE_VERSION_MINOR 1
#define NEPDATE_VERSION_PATCH 0
/* The part after "-" in a pre-release version such as 0.1.0-beta.1, empty for a stable one. */
#define NEPDATE_VERSION_PRERELEASE "beta.1"
#define NEPDATE_VERSION_STRING "0.1.0-beta.1"

/* 1 when the struct tm bridge (nepdate_from_tm and nepdate_to_tm) is declared. A build with
 * NEPDATE_C_WITH_TM=0 omits it so the core needs no <time.h> at all. */
#ifndef NEPDATE_C_WITH_TM
#define NEPDATE_C_WITH_TM 1
#endif

/* Exported functions carry NEPDATE_API (ADR-0002). A Windows DLL needs dllexport while it is built
 * and dllimport in its users; the CMake target defines NEPDATE_STATIC for a static library, and a
 * project that compiles the sources itself defines it too. */
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(NEPDATE_STATIC)
#define NEPDATE_API
#elif defined(NEPDATE_BUILDING)
#define NEPDATE_API __declspec(dllexport)
#else
#define NEPDATE_API __declspec(dllimport)
#endif
#elif defined(__GNUC__)
#define NEPDATE_API __attribute__((visibility("default")))
#else
#define NEPDATE_API
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The supported Bikram Sambat years. The last supported day is 2199-12-30 (AD 2143-04-15), the
 * first 1901-01-01 (AD 1844-04-11). */
#define NEPDATE_MIN_YEAR 1901
#define NEPDATE_MAX_YEAR 2199

/* What a function returns: NEPDATE_OK, or why it failed. The values are fixed; new kinds are only
 * ever added at the end. */
typedef enum nepdate_status {
    NEPDATE_OK = 0,
    /* A BS year outside NEPDATE_MIN_YEAR..NEPDATE_MAX_YEAR, an AD date outside 1844-04-11 to
     * 2143-04-15, a result outside the supported range, an overflow mode or quarter outside its
     * values, or a range whose end is before its start. */
    NEPDATE_E_OUT_OF_RANGE = 1,
    /* A month outside 1..12. */
    NEPDATE_E_INVALID_MONTH = 2,
    /* A day outside 1 to the length of its month. */
    NEPDATE_E_INVALID_DAY = 3,
    /* An AD year, month and day that is not a date of the proleptic Gregorian calendar. */
    NEPDATE_E_INVALID_GREGORIAN = 4,
    /* Parse errors (shared/spec/PARSING.md section 4). */
    NEPDATE_E_INVALID_CHARACTER = 5,
    NEPDATE_E_WRONG_GROUP_COUNT = 6,
    NEPDATE_E_NUMBER_TOO_LONG = 7,
    NEPDATE_E_AMBIGUOUS = 8,
    NEPDATE_E_UNRECOGNIZED = 9,
    /* A nepdate_date no function of this library made, such as a zero-initialised one. */
    NEPDATE_E_INVALID_DATE = 10,
    /* A required pointer argument is NULL, including the text of a parsing function. */
    NEPDATE_E_NULL = 11
} nepdate_status;

/* A Bikram Sambat date, 4 bytes, passed by value. Make one with nepdate_from_bs or
 * nepdate_from_ad and read it with nepdate_to_bs. `value` is private: it is 1 plus the number of
 * days since BS 1901-01-01 (ADR-0006), so a zero-initialised nepdate_date is not a date. Every
 * function that takes a nepdate_date accepts any value and returns NEPDATE_E_INVALID_DATE for one
 * it can't have made. */
typedef struct nepdate_date {
    int32_t value;
} nepdate_date;

/* A year, month (1 to 12) and day, in either calendar. */
typedef struct nepdate_ymd {
    int year;
    int month;
    int day;
} nepdate_ymd;

/* The twelve Bikram Sambat months, Baishakh = 1. */
typedef enum nepdate_month {
    NEPDATE_BAISHAKH = 1,
    NEPDATE_JESTHA = 2,
    NEPDATE_ASHAD = 3,
    NEPDATE_SHRAWAN = 4,
    NEPDATE_BHADRA = 5,
    NEPDATE_ASHOJ = 6,
    NEPDATE_KARTIK = 7,
    NEPDATE_MANGSIR = 8,
    NEPDATE_POUSH = 9,
    NEPDATE_MAGH = 10,
    NEPDATE_FALGUN = 11,
    NEPDATE_CHAITRA = 12
} nepdate_month;

/* The days of the week, Sunday = 0 (ALGORITHM section 7). */
typedef enum nepdate_weekday {
    NEPDATE_SUNDAY = 0,
    NEPDATE_MONDAY = 1,
    NEPDATE_TUESDAY = 2,
    NEPDATE_WEDNESDAY = 3,
    NEPDATE_THURSDAY = 4,
    NEPDATE_FRIDAY = 5,
    NEPDATE_SATURDAY = 6
} nepdate_weekday;

/* The quarters of Nepal's fiscal year, Shrawan to Ashad (ALGORITHM section 8b). */
typedef enum nepdate_quarter {
    NEPDATE_Q1 = 1,
    NEPDATE_Q2 = 2,
    NEPDATE_Q3 = 3,
    NEPDATE_Q4 = 4
} nepdate_quarter;

/* What nepdate_add_months and nepdate_add_years do when the target month is too short for the
 * day: NEPDATE_CLAMP uses its last day, NEPDATE_OVERFLOW carries the extra days into the next
 * month. NEPDATE_CLAMP is the C# AddMonths(n) default. */
typedef enum nepdate_overflow { NEPDATE_CLAMP = 0, NEPDATE_OVERFLOW = 1 } nepdate_overflow;

/* A language for text output: English or Nepali. */
typedef enum nepdate_lang { NEPDATE_ENGLISH = 0, NEPDATE_NEPALI = 1 } nepdate_lang;

/* The field order of a short format (FORMATTING section 3). A value outside the enumerators is
 * treated as NEPDATE_YMD. */
typedef enum nepdate_order {
    NEPDATE_YMD = 0,
    NEPDATE_YDM = 1,
    NEPDATE_MYD = 2,
    NEPDATE_MDY = 3,
    NEPDATE_DYM = 4,
    NEPDATE_DMY = 5
} nepdate_order;

/* The separator of a short format. A value outside the enumerators is treated as
 * NEPDATE_SEP_SLASH. */
typedef enum nepdate_separator {
    NEPDATE_SEP_SLASH = '/',
    NEPDATE_SEP_BACKSLASH = '\\',
    NEPDATE_SEP_DOT = '.',
    NEPDATE_SEP_UNDERSCORE = '_',
    NEPDATE_SEP_DASH = '-',
    NEPDATE_SEP_SPACE = ' '
} nepdate_separator;

/* Options of nepdate_format_long, matching the C# long format: a weekday and its comma when
 * `weekday` is true, the 2-digit day when `pad` is true, and the year and its comma when `year` is
 * true. C# defaults to no weekday, the year and the padded day. */
typedef struct nepdate_long_options {
    bool weekday;
    bool year;
    bool pad;
} nepdate_long_options;

/* The calendar distance from one date to another (ALGORITHM section 8a): `sign` is -1 when the
 * second date is earlier and 1 otherwise, and `years`, `months` and `days` are magnitudes. */
typedef struct nepdate_date_diff {
    int sign;
    int years;
    int months;
    int days;
} nepdate_date_diff;

/* Every day from start to end, both included. Make one with nepdate_range_make or one of the
 * range_of functions; a range holds two valid dates with start not after end. */
typedef struct nepdate_range {
    nepdate_date start;
    nepdate_date end;
} nepdate_range;

/* A cursor over a nepdate_range. Make one with nepdate_range_begin and step it with
 * nepdate_range_next. The fields are private. */
typedef struct nepdate_range_iterator {
    int32_t current;
    int32_t last;
} nepdate_range_iterator;

/* Returns the version of the compiled library as "MAJOR.MINOR.PATCH" or
 * "MAJOR.MINOR.PATCH-PRERELEASE", a static string. It equals NEPDATE_VERSION_STRING when the header
 * and the library come from the same release. */
NEPDATE_API const char *nepdate_version(void);

/* Returns the name of status as a static string, for example "NEPDATE_E_INVALID_DAY", or
 * "unknown" for a value that is not a nepdate_status. */
NEPDATE_API const char *nepdate_status_name(nepdate_status status);

/* Makes the BS date year-month-day and stores it in *out. The checks run in this order:
 * NEPDATE_E_NULL when out is NULL, NEPDATE_E_OUT_OF_RANGE for a year outside
 * NEPDATE_MIN_YEAR..NEPDATE_MAX_YEAR, NEPDATE_E_INVALID_MONTH, then NEPDATE_E_INVALID_DAY. Any int
 * is accepted. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_from_bs(int year, int month, int day, nepdate_date *out);

/* Stores the BS year, month and day of date in *out. Returns NEPDATE_E_NULL when out is NULL and
 * NEPDATE_E_INVALID_DATE for a date no function made; *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_to_bs(nepdate_date date, nepdate_ymd *out);

/* Makes the AD (proleptic Gregorian) date year-month-day and stores the BS date in *out. The
 * checks run in this order: NEPDATE_E_NULL when out is NULL, NEPDATE_E_INVALID_GREGORIAN for a
 * month or day the Gregorian calendar doesn't have, then NEPDATE_E_OUT_OF_RANGE for a real date
 * outside 1844-04-11..2143-04-15. Any int is accepted, INT_MIN and INT_MAX included. *out is
 * unchanged on error. */
NEPDATE_API nepdate_status nepdate_from_ad(int year, int month, int day, nepdate_date *out);

/* Stores the AD year, month and day of date in *out. Returns NEPDATE_E_NULL when out is NULL and
 * NEPDATE_E_INVALID_DATE for a date no function made; *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_to_ad(nepdate_date date, nepdate_ymd *out);

/* Stores the weekday of date in *out (ALGORITHM section 7). Returns NEPDATE_E_NULL when out is
 * NULL and NEPDATE_E_INVALID_DATE for a date no function made; *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_weekday_of(nepdate_date date, nepdate_weekday *out);

/* Stores the month of date in *out. Returns NEPDATE_E_NULL when out is NULL and
 * NEPDATE_E_INVALID_DATE for a date no function made; *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_month_of(nepdate_date date, nepdate_month *out);

/* Stores the day of the BS year of date in *out, 1 to 367. Returns NEPDATE_E_NULL when out is NULL
 * and NEPDATE_E_INVALID_DATE for a date no function made; *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_day_of_year(nepdate_date date, int *out);

/* Stores the length in days of the BS month of date in *out, 29 to 32. Returns NEPDATE_E_NULL when
 * out is NULL and NEPDATE_E_INVALID_DATE for a date no function made; *out is unchanged on error.
 */
NEPDATE_API nepdate_status nepdate_month_length(nepdate_date date, int *out);

/* Stores the first day of the BS month of date in *out. Returns NEPDATE_E_NULL when out is NULL and
 * NEPDATE_E_INVALID_DATE for a date no function made; *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_first_day_of_month(nepdate_date date, nepdate_date *out);

/* Stores the last day of the BS month of date in *out. Returns NEPDATE_E_NULL when out is NULL and
 * NEPDATE_E_INVALID_DATE for a date no function made; *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_last_day_of_month(nepdate_date date, nepdate_date *out);

/* Compares a and b and stores -1, 0 or 1 in *out for a before, equal to, or after b. Returns
 * NEPDATE_E_NULL when out is NULL and NEPDATE_E_INVALID_DATE when either date is not one this
 * library made; *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_compare(nepdate_date a, nepdate_date b, int *out);

/* Stores in *out the date `days` days after date, or before it for a negative count. Returns
 * NEPDATE_E_NULL when out is NULL, NEPDATE_E_INVALID_DATE when date is not one this library made,
 * and NEPDATE_E_OUT_OF_RANGE when the result would be outside the supported range. Any int32_t is
 * accepted. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_add_days(nepdate_date date, int32_t days, nepdate_date *out);

/* Stores in *out the date `months` months after date, or before it for a negative count. When the
 * day does not exist in the target month, overflow chooses the last day (NEPDATE_CLAMP) or the
 * next month with the extra days (NEPDATE_OVERFLOW). Returns NEPDATE_E_NULL when out is NULL,
 * NEPDATE_E_INVALID_DATE for a date no function made, and NEPDATE_E_OUT_OF_RANGE when the result
 * would be outside the supported range or overflow is neither value. Any int32_t is accepted.
 * *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_add_months(nepdate_date date, int32_t months,
                                              nepdate_overflow overflow, nepdate_date *out);

/* Stores in *out the date `years` years after date, or before it for a negative count: 12 * years
 * months with the same overflow rule as nepdate_add_months. Returns NEPDATE_E_NULL when out is
 * NULL, NEPDATE_E_INVALID_DATE for a date no function made, and NEPDATE_E_OUT_OF_RANGE when the
 * result would be outside the supported range or overflow is neither value. Any int32_t is
 * accepted. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_add_years(nepdate_date date, int32_t years,
                                             nepdate_overflow overflow, nepdate_date *out);

/* Stores in *out the signed number of days from a to b: positive when b is later, negative when it
 * is earlier. Returns NEPDATE_E_NULL when out is NULL and NEPDATE_E_INVALID_DATE when either date
 * is not one this library made. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_diff_days(nepdate_date a, nepdate_date b, int32_t *out);

/* Stores in *out the calendar distance from a to b (ALGORITHM section 8a): sign, then whole years,
 * whole months and the remaining days. Returns NEPDATE_E_NULL when out is NULL and
 * NEPDATE_E_INVALID_DATE when either date is not one this library made. *out is unchanged on
 * error. */
NEPDATE_API nepdate_status nepdate_diff(nepdate_date a, nepdate_date b, nepdate_date_diff *out);

/* Stores in *out the fiscal year that contains date, named by the BS year it starts in: the date's
 * own year from Shrawan on, the year before from Baishakh to Ashad. Returns NEPDATE_E_NULL when
 * out is NULL and NEPDATE_E_INVALID_DATE for a date no function made. *out is unchanged on error.
 */
NEPDATE_API nepdate_status nepdate_fiscal_year_of(nepdate_date date, int *out);

/* Stores in *out the first day of fiscal_year, 1 Shrawan, or NEPDATE_E_OUT_OF_RANGE when that day
 * is not supported. Returns NEPDATE_E_NULL when out is NULL. Any int is accepted. *out is
 * unchanged on error. */
NEPDATE_API nepdate_status nepdate_fiscal_year_start(int fiscal_year, nepdate_date *out);

/* Stores in *out the last day of fiscal_year, the last day of Ashad of the following year, or
 * NEPDATE_E_OUT_OF_RANGE when that day is not supported. Returns NEPDATE_E_NULL when out is NULL.
 * Any int is accepted. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_fiscal_year_end(int fiscal_year, nepdate_date *out);

/* Stores in *out the quarter of date: Q1 is Shrawan to Ashoj, Q2 Kartik to Poush, Q3 Magh to
 * Chaitra, and Q4 Baishakh to Ashad (ALGORITHM section 8b). Returns NEPDATE_E_NULL when out is
 * NULL and NEPDATE_E_INVALID_DATE for a date no function made. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_quarter_of(nepdate_date date, nepdate_quarter *out);

/* Stores in *out the first day of quarter quarter of fiscal_year. Returns NEPDATE_E_NULL when out
 * is NULL, and NEPDATE_E_OUT_OF_RANGE when quarter is not NEPDATE_Q1..NEPDATE_Q4 or the day is
 * not supported. Any int is accepted for fiscal_year. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_quarter_start(int fiscal_year, nepdate_quarter quarter,
                                                 nepdate_date *out);

/* Stores in *out the last day of quarter quarter of fiscal_year. Returns NEPDATE_E_NULL when out
 * is NULL, and NEPDATE_E_OUT_OF_RANGE when quarter is not NEPDATE_Q1..NEPDATE_Q4 or the day is
 * not supported. Any int is accepted for fiscal_year. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_quarter_end(int fiscal_year, nepdate_quarter quarter,
                                               nepdate_date *out);

/* Writes the label of fiscal_year, such as "2082/83" or the same in Devanagari digits, to buffer
 * with snprintf semantics: the return value is the length the full label needs, the text is
 * truncated and NUL-terminated when size is too small, and size 0 or a NULL buffer writes
 * nothing. A negative
 * fiscal_year gets a leading '-'. lang NEPDATE_NEPALI selects Devanagari digits and any other value
 * English. A buffer of 39 bytes always fits the label and its terminator. */
NEPDATE_API size_t nepdate_fiscal_year_label(int fiscal_year, nepdate_lang lang, char *buffer,
                                             size_t size);

/* Writes date in the format described by pattern (FORMATTING section 5) and returns the length the
 * full text needs. A NULL pattern means the default display, "YYYY/MM/DD". The text is truncated
 * and NUL-terminated when size is too small, size 0 or a NULL buffer writes nothing, and the
 * returned length is still the full one. NEPDATE_NEPALI selects Devanagari digits and the Nepali
 * month names; any other language is English. A date no function made returns 0 and writes nothing.
 */
NEPDATE_API size_t nepdate_format(nepdate_date date, const char *pattern, nepdate_lang lang,
                                  char *buffer, size_t size);

/* Writes the short format of date (FORMATTING section 3): the three fields in order, separated by
 * separator, with a 4-digit year and 2-digit month and day when pad is true. The truncation,
 * NUL-termination and invalid-date rules are those of nepdate_format. */
NEPDATE_API size_t nepdate_format_short(nepdate_date date, nepdate_order order,
                                        nepdate_separator separator, bool pad, nepdate_lang lang,
                                        char *buffer, size_t size);

/* Writes the long format of date (FORMATTING section 4), for example "Tuesday, Shrawan 15, 2081".
 * The truncation, NUL-termination and invalid-date rules are those of nepdate_format. */
NEPDATE_API size_t nepdate_format_long(nepdate_date date, nepdate_long_options options,
                                       nepdate_lang lang, char *buffer, size_t size);

/* Stores in *out the BS date of the instant unix_seconds after 1970-01-01 00:00 UTC, after
 * shifting it by utc_offset_minutes (east of UTC is positive) and taking the local day. Seconds
 * before the epoch are floored, so the result is the day that contains the instant. Returns
 * NEPDATE_E_NULL when out is NULL and NEPDATE_E_OUT_OF_RANGE when that day is outside the
 * supported range. Any int64_t seconds and int32_t offset are accepted, INT64_MIN, INT64_MAX,
 * INT32_MIN and INT32_MAX included. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_from_unix(int64_t unix_seconds, int32_t utc_offset_minutes,
                                             nepdate_date *out);

/* Stores in *out today's BS date for the instant unix_seconds after 1970-01-01 00:00 UTC, read in
 * Nepal: UTC+05:45 from 1986-01-01 00:00 local (1985-12-31 18:30 UTC), UTC+05:30 before that.
 * The +05:30 rule also covers instants before 1920, where IANA's tzdata has local mean time
 * (+05:41:16) instead.
 * Returns NEPDATE_E_NULL when out is NULL and NEPDATE_E_OUT_OF_RANGE when the date is outside the
 * supported range. Any int64_t is accepted; *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_today_nepal(int64_t unix_seconds, nepdate_date *out);

#if NEPDATE_C_WITH_TM
/* The C library's broken-down time. Only the year, month and day fields are read or written by
 * this pair, so no time zone or daylight-saving state is involved. */
struct tm;

/* Stores in *out the BS date of the proleptic Gregorian calendar date in tm: tm_year + 1900,
 * tm_mon + 1 and tm_mday. The time of day and tm_isdst are ignored. Returns NEPDATE_E_NULL when
 * tm or out is NULL, NEPDATE_E_OUT_OF_RANGE when the year does not fit in int or is outside the
 * supported range, and NEPDATE_E_INVALID_GREGORIAN for a month or day the Gregorian calendar
 * doesn't have. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_from_tm(const struct tm *tm, nepdate_date *out);

/* Stores the AD date of date in out's tm_year, tm_mon and tm_mday, sets tm_hour, tm_min and tm_sec
 * to 0 and tm_isdst to -1, and fills tm_wday (0 = Sunday) and tm_yday (0 = 1 January). Returns
 * NEPDATE_E_NULL when out is NULL and NEPDATE_E_INVALID_DATE for a date no function made; *out is
 * unchanged on error. */
NEPDATE_API nepdate_status nepdate_to_tm(nepdate_date date, struct tm *out);
#endif

/* Parses exactly year, month and day from text, in that order (PARSING section 1). Reads at most
 * len bytes and never needs a terminator. Returns NEPDATE_OK and stores the date in *out, or one of
 * NEPDATE_E_NULL for a NULL argument, NEPDATE_E_INVALID_CHARACTER, NEPDATE_E_WRONG_GROUP_COUNT,
 * NEPDATE_E_NUMBER_TOO_LONG, NEPDATE_E_OUT_OF_RANGE, NEPDATE_E_INVALID_MONTH or
 * NEPDATE_E_INVALID_DAY. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_parse(const char *text, size_t len, nepdate_date *out);

/* Parses human input from text (PARSING section 2): strict parsing first, then month names, era
 * markers, the dropped words and the numeric fallback. Reads at most len bytes and never needs a
 * terminator. Besides the strict errors it returns NEPDATE_E_AMBIGUOUS or NEPDATE_E_UNRECOGNIZED,
 * and NEPDATE_E_NULL for a NULL argument. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_parse_lenient(const char *text, size_t len, nepdate_date *out);

/* Stores in *out the range from start to end, both included. Returns NEPDATE_E_NULL when out is
 * NULL, NEPDATE_E_INVALID_DATE when either date is not one this library made, and
 * NEPDATE_E_OUT_OF_RANGE when end is before start. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_range_make(nepdate_date start, nepdate_date end,
                                              nepdate_range *out);

/* Stores in *out every day of one BS month. Returns NEPDATE_E_NULL when out is NULL and the
 * errors of nepdate_from_bs for day 1 of the month. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_range_of_month(int year, int month, nepdate_range *out);

/* Stores in *out every day of one BS year, Baishakh 1 to the last day of Chaitra. Returns
 * NEPDATE_E_NULL when out is NULL and NEPDATE_E_OUT_OF_RANGE for a year outside the supported
 * range. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_range_of_year(int year, nepdate_range *out);

/* Stores in *out every day of a fiscal year, 1 Shrawan to the last day of Ashad. Returns
 * NEPDATE_E_NULL when out is NULL and NEPDATE_E_OUT_OF_RANGE when either end is not supported.
 * *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_range_of_fiscal_year(int fiscal_year, nepdate_range *out);

/* Stores in *out the number of days in range, at least 1. Returns NEPDATE_E_NULL when out is NULL
 * and NEPDATE_E_INVALID_DATE for a range no function made. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_range_length(nepdate_range range, int *out);

/* Stores in *out true when date is inside range, endpoints included. Returns NEPDATE_E_NULL when
 * out is NULL, and NEPDATE_E_INVALID_DATE for a range or date no function made. *out is unchanged
 * on error. */
NEPDATE_API nepdate_status nepdate_range_contains(nepdate_range range, nepdate_date date,
                                                  bool *out);

/* Stores in *out a cursor on the first day of range. Returns NEPDATE_E_NULL when out is NULL and
 * NEPDATE_E_INVALID_DATE for a range no function made. *out is unchanged on error. */
NEPDATE_API nepdate_status nepdate_range_begin(nepdate_range range, nepdate_range_iterator *out);

/* Stores the next day in *out and advances the cursor. Returns false at the end of the range, or
 * when iterator or out is NULL; *out is unchanged when it returns false. */
NEPDATE_API bool nepdate_range_next(nepdate_range_iterator *iterator, nepdate_date *out);

#ifdef __cplusplus
}
#endif

#endif /* NEPDATE_H */
