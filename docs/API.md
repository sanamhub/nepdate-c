# API

The public surface of `nepdate-c`. The core is declared in `<nepdate.h>` and the patro library in
`<nepdate_patro.h>`; nothing else is exported (ADR-0002). The API-surface gate compares each shared
library's export table with the functions on this page.

The groups below follow the canonical inventory in the hub's `spec/API.md`, so a member of the C#
package can be found in the same place here. The [last table](#c-members-and-their-c-functions)
lists each C# member and the C function or functions that replace it.

## Version

```c
#define NEPDATE_VERSION_MAJOR 0
#define NEPDATE_VERSION_MINOR 1
#define NEPDATE_VERSION_PATCH 0
#define NEPDATE_VERSION_PRERELEASE "rc.1"
#define NEPDATE_VERSION_STRING "0.1.0-rc.1"

const char *nepdate_version(void);
```

The macros give the version of the header. `NEPDATE_VERSION_PRERELEASE` is the part after `-` in
a pre-release such as `0.1.0-rc.1`, and empty for a stable version.

`nepdate_version` returns the version of the compiled library as a static string, equal to
`NEPDATE_VERSION_STRING` when the header and the library come from the same release. It can't
fail.

```c
/* prints 0.1.0-rc.1 */
printf("%s\n", nepdate_version());
```

## Conventions

Functions return a `nepdate_status` and write their result through the last argument, a pointer
the caller owns. The pointer is checked first (`NEPDATE_E_NULL`), and on any error the object it
points to is left unchanged. The formatting functions are the exception to the status return: they
return the length of the text and use `snprintf` semantics, described in [Formatting](#formatting).
No function allocates, keeps state or reads the clock, so every function is thread-safe. A function
that needs the current day takes the instant and the UTC offset from the caller.

The supported range is BS 1901-01-01 to 2199-12-30, which is AD 1844-04-11 to 2143-04-15
(`shared/spec/ALGORITHM.md`).

## Types

```c
#define NEPDATE_MIN_YEAR 1901
#define NEPDATE_MAX_YEAR 2199
#define NEPDATE_MIN nepdate_min()
#define NEPDATE_MAX nepdate_max()

typedef enum nepdate_status {
    NEPDATE_OK = 0,
    NEPDATE_E_OUT_OF_RANGE = 1,
    NEPDATE_E_INVALID_MONTH = 2,
    NEPDATE_E_INVALID_DAY = 3,
    NEPDATE_E_INVALID_GREGORIAN = 4,
    NEPDATE_E_INVALID_CHARACTER = 5,
    NEPDATE_E_WRONG_GROUP_COUNT = 6,
    NEPDATE_E_NUMBER_TOO_LONG = 7,
    NEPDATE_E_AMBIGUOUS = 8,
    NEPDATE_E_UNRECOGNIZED = 9,
    NEPDATE_E_INVALID_DATE = 10,
    NEPDATE_E_NULL = 11,
    NEPDATE_E_NO_OVERLAP = 12
} nepdate_status;

typedef struct nepdate_date { int32_t value; } nepdate_date;
typedef struct nepdate_ymd { int year; int month; int day; } nepdate_ymd;
typedef struct nepdate_text { const char *text; size_t length; } nepdate_text;

typedef enum nepdate_month {
    NEPDATE_BAISHAKH = 1, NEPDATE_JESTHA = 2, NEPDATE_ASHAD = 3, NEPDATE_SHRAWAN = 4,
    NEPDATE_BHADRA = 5, NEPDATE_ASHOJ = 6, NEPDATE_KARTIK = 7, NEPDATE_MANGSIR = 8,
    NEPDATE_POUSH = 9, NEPDATE_MAGH = 10, NEPDATE_FALGUN = 11, NEPDATE_CHAITRA = 12
} nepdate_month;

typedef enum nepdate_weekday {
    NEPDATE_SUNDAY = 0, NEPDATE_MONDAY = 1, NEPDATE_TUESDAY = 2, NEPDATE_WEDNESDAY = 3,
    NEPDATE_THURSDAY = 4, NEPDATE_FRIDAY = 5, NEPDATE_SATURDAY = 6
} nepdate_weekday;

typedef enum nepdate_quarter {
    NEPDATE_Q1 = 1, NEPDATE_Q2 = 2, NEPDATE_Q3 = 3, NEPDATE_Q4 = 4
} nepdate_quarter;

typedef enum nepdate_overflow {
    NEPDATE_CLAMP = 0, NEPDATE_OVERFLOW = 1
} nepdate_overflow;

typedef enum nepdate_lang { NEPDATE_ENGLISH = 0, NEPDATE_NEPALI = 1 } nepdate_lang;

typedef enum nepdate_order {
    NEPDATE_YMD = 0, NEPDATE_YDM = 1, NEPDATE_MYD = 2,
    NEPDATE_MDY = 3, NEPDATE_DYM = 4, NEPDATE_DMY = 5
} nepdate_order;

typedef enum nepdate_separator {
    NEPDATE_SEP_SLASH = '/', NEPDATE_SEP_BACKSLASH = '\\', NEPDATE_SEP_DOT = '.',
    NEPDATE_SEP_UNDERSCORE = '_', NEPDATE_SEP_DASH = '-', NEPDATE_SEP_SPACE = ' '
} nepdate_separator;

typedef struct nepdate_long_options {
    bool weekday;
    bool year;
    bool pad;
} nepdate_long_options;

typedef struct nepdate_date_diff {
    int sign;
    int years;
    int months;
    int days;
} nepdate_date_diff;

typedef struct nepdate_range {
    nepdate_date start;
    nepdate_date end;
} nepdate_range;

typedef struct nepdate_range_iterator {
    int32_t current;
    int32_t last;
} nepdate_range_iterator;

typedef struct nepdate_range_day_cursor {
    int32_t current;
    int32_t last;
    int32_t step;
    uint32_t skip;
} nepdate_range_day_cursor;
```

`nepdate_date` is a 4-byte value passed by value. Its only field, `value`, is reserved: do not read
or write it, and do not build a date from it. It holds 1 plus the number of days since BS
1901-01-01 (ADR-0006), so a zero-initialised `nepdate_date` is not a date and every function
rejects it with `NEPDATE_E_INVALID_DATE`. Every function that takes a `nepdate_date` accepts any
bit pattern without undefined behaviour. There is therefore no separate "is default" test: the
zero value is simply invalid.

`NEPDATE_MIN` and `NEPDATE_MAX` are the first and last supported dates. Both expand to a call of
`nepdate_min` or `nepdate_max`, so they can be used wherever an expression is, not in a constant
initialiser.

`nepdate_ymd` holds a year, a month (1 to 12) and a day, in either calendar. `nepdate_text` holds a
piece of text and its length, so a caller can pass a set of bytes as one value.
`nepdate_month` numbers the BS months 1 to 12 with Baishakh first. `nepdate_weekday` numbers the
days 0 to 6 with Sunday first (`shared/spec/ALGORITHM.md` section 7). `nepdate_quarter` numbers the
fiscal quarters, `nepdate_overflow` chooses what month arithmetic does with a day the target month
does not have, and `nepdate_lang` selects English or Nepali text.

`nepdate_range` holds two valid dates with `start` not after `end`; a range is never empty. Make one
with `nepdate_range_make`, one of the `nepdate_range_of_*` functions or one of the
`nepdate_range_current_*` functions, not by filling the fields. A range that no function made is
rejected with `NEPDATE_E_INVALID_DATE`. `nepdate_range_iterator` and `nepdate_range_day_cursor` are
cursors whose fields are private; only the functions that step them touch those fields.

## Errors

```c
const char *nepdate_status_name(nepdate_status status);
```

Returns the enumerator's name as a static string, such as `"NEPDATE_E_INVALID_DAY"`, or
`"unknown"` for a value that is not one.

| Status | Meaning |
|---|---|
| `NEPDATE_OK` | the call succeeded and the out object holds the result |
| `NEPDATE_E_OUT_OF_RANGE` | a BS year outside 1901 to 2199, an AD date outside 1844-04-11 to 2143-04-15, an arithmetic result outside the supported range, an overflow mode, quarter or interval outside its values, or a range whose end is before its start |
| `NEPDATE_E_INVALID_MONTH` | a BS month outside 1 to 12 |
| `NEPDATE_E_INVALID_DAY` | a BS day outside 1 to the length of its month |
| `NEPDATE_E_INVALID_GREGORIAN` | an AD year, month and day that is not a proleptic Gregorian date |
| `NEPDATE_E_INVALID_CHARACTER` | parsing met a byte that is neither a digit nor a separator |
| `NEPDATE_E_WRONG_GROUP_COUNT` | strict parsing found fewer than three groups (empty input included) or a fourth |
| `NEPDATE_E_NUMBER_TOO_LONG` | a parsed number has more than four digits |
| `NEPDATE_E_AMBIGUOUS` | lenient parsing cannot tell the year, month and day apart |
| `NEPDATE_E_UNRECOGNIZED` | lenient parsing found no form it accepts |
| `NEPDATE_E_INVALID_DATE` | a `nepdate_date` or `nepdate_range` that no function of this library made |
| `NEPDATE_E_NULL` | a required pointer is `NULL`, including the text of a parsing function |
| `NEPDATE_E_NO_OVERLAP` | two ranges share no day, so their intersection has no value; the out object is unchanged |

```c
nepdate_date date;
const nepdate_status status = nepdate_from_bs(2081, 13, 1, &date);
if (status != NEPDATE_OK) {
    /* prints nepdate: NEPDATE_E_INVALID_MONTH */
    fprintf(stderr, "nepdate: %s\n", nepdate_status_name(status));
}
```

## Dates, identity and comparison

```c
nepdate_date nepdate_min(void);
nepdate_date nepdate_max(void);
nepdate_status nepdate_from_bs(int year, int month, int day, nepdate_date *out);
nepdate_status nepdate_to_bs(nepdate_date date, nepdate_ymd *out);
nepdate_status nepdate_from_ad(int year, int month, int day, nepdate_date *out);
nepdate_status nepdate_to_ad(nepdate_date date, nepdate_ymd *out);
nepdate_status nepdate_weekday_of(nepdate_date date, nepdate_weekday *out);
nepdate_status nepdate_month_of(nepdate_date date, nepdate_month *out);
nepdate_status nepdate_day_of_year(nepdate_date date, int *out);
nepdate_status nepdate_month_length(nepdate_date date, int *out);
nepdate_status nepdate_year_length(nepdate_date date, int *out);
nepdate_status nepdate_first_day_of_month(nepdate_date date, nepdate_date *out);
nepdate_status nepdate_last_day_of_month(nepdate_date date, nepdate_date *out);
nepdate_status nepdate_compare(nepdate_date a, nepdate_date b, int *out);
nepdate_status nepdate_today(int32_t utc_offset_minutes, int64_t unix_seconds,
                             nepdate_date *out);
nepdate_status nepdate_is_today(nepdate_date date, int32_t utc_offset_minutes,
                                int64_t unix_seconds, bool *out);
nepdate_status nepdate_is_yesterday(nepdate_date date, int32_t utc_offset_minutes,
                                    int64_t unix_seconds, bool *out);
nepdate_status nepdate_is_tomorrow(nepdate_date date, int32_t utc_offset_minutes,
                                   int64_t unix_seconds, bool *out);
```

`nepdate_from_bs` makes the BS date `year-month-day`. It checks, in this order: `NEPDATE_E_NULL`,
`NEPDATE_E_OUT_OF_RANGE` for the year, `NEPDATE_E_INVALID_MONTH`, `NEPDATE_E_INVALID_DAY`. Any
`int` is accepted, `INT_MIN` and `INT_MAX` included. A struct cannot carry an "unset" state, so the
only way to have no date is a zero value, and every function rejects that.

```c
nepdate_date date;
if (nepdate_from_bs(2081, 4, 32, &date) == NEPDATE_OK) {
    /* Shrawan 2081 has 32 days; 2081-04-33 is NEPDATE_E_INVALID_DAY */
}
```

`nepdate_to_bs` stores the BS year, month and day, the same three values a tuple deconstruction
gives in C#. It fails with `NEPDATE_E_NULL` or `NEPDATE_E_INVALID_DATE`.

```c
nepdate_ymd bs;
if (nepdate_to_bs(NEPDATE_MAX, &bs) == NEPDATE_OK) {
    /* bs is {2199, 12, 30} */
}
```

`nepdate_from_ad` makes the proleptic Gregorian date. It checks, in this order: `NEPDATE_E_NULL`,
`NEPDATE_E_INVALID_GREGORIAN` for a month or day the calendar doesn't have, then
`NEPDATE_E_OUT_OF_RANGE` for a real date outside 1844-04-11 to 2143-04-15. So 2023-02-29 is
`NEPDATE_E_INVALID_GREGORIAN` and 1844-04-10 is `NEPDATE_E_OUT_OF_RANGE`. Any `int` is accepted,
`INT_MIN` and `INT_MAX` included. `nepdate_to_ad` returns the AD year, month and day of a date.

```c
nepdate_date date;
nepdate_ymd ad;
if (nepdate_from_ad(2024, 7, 30, &date) == NEPDATE_OK &&
    nepdate_to_ad(date, &ad) == NEPDATE_OK) {
    /* ad is {2024, 7, 30} */
}
```

`nepdate_weekday_of` and `nepdate_month_of` return the enumerators above.
`nepdate_day_of_year` is the 1-based ordinal in the BS year, 1 to 367, not in the Gregorian year.
`nepdate_month_length` is 29 to 32. `nepdate_year_length` is 364 to 367, the sum of the twelve
month lengths of the BS year: it replaces the C# `IsLeapYear`, which follows the Gregorian year and
changes in the middle of a BS year (`shared/spec/PARITY.md`, D-12).
`nepdate_first_day_of_month` and `nepdate_last_day_of_month` return dates in the same BS month.

```c
nepdate_date date;
int length = 0;
if (nepdate_from_bs(2081, 4, 15, &date) == NEPDATE_OK) {
    nepdate_month_length(date, &length); /* 32 */
    nepdate_year_length(date, &length);  /* 365 or 366 */
    nepdate_day_of_year(date, &length);  /* 15 */
}
```

`nepdate_compare` stores -1, 0 or 1 for `a` before, equal to, or after `b`. Ordering and equality
are the same comparison in C, which has no operator overloading:

```c
nepdate_date first;
nepdate_date second;
int comparison = 0;
nepdate_from_bs(2081, 4, 15, &first);
nepdate_from_bs(2081, 4, 16, &second);
nepdate_compare(first, second, &comparison); /* -1: first is the earlier date */
/* equality is comparison == 0, and it is the packed day count compared */
```

`nepdate_today` stores today's BS date for the instant `unix_seconds` after 1970-01-01 00:00 UTC,
read at `utc_offset_minutes` east of UTC (Nepal is 345). The caller supplies both the clock reading
and the zone, so the call is deterministic and never reads the machine clock. `nepdate_is_today`,
`nepdate_is_yesterday` and `nepdate_is_tomorrow` compare a date with that day and take the same two
arguments. C# has a `Now` property beside `Today`; a date has no time of day, so both are the same
call here and there is no separate member.

```c
nepdate_date today;
if (nepdate_today(345, 1722364200, &today) == NEPDATE_OK) {
    /* 2024-07-30 18:30 UTC is 2081-04-16 in Nepal */
    bool same = false;
    nepdate_is_today(today, 345, 1722364200, &same); /* true */
}
```

All of these return `NEPDATE_E_NULL` when an out-pointer is `NULL` and `NEPDATE_E_INVALID_DATE`
when they are given a `nepdate_date` no function made, and leave the out object unchanged on error.
`nepdate_compare` returns `NEPDATE_E_INVALID_DATE` when either date is invalid. The clock functions
also return `NEPDATE_E_OUT_OF_RANGE` when the instant is outside the supported span.

## Conversion and serialization

```c
nepdate_status nepdate_from_ymd_int(int64_t ymd, nepdate_date *out);
nepdate_status nepdate_to_ymd_int(nepdate_date date, int64_t *out);
size_t nepdate_format_iso(nepdate_date date, char *buffer, size_t size);
nepdate_status nepdate_bulk_to_bs(const nepdate_ymd *ad, size_t count, nepdate_date *out,
                                  size_t capacity, size_t *written);
nepdate_status nepdate_bulk_parse_to_ad(const nepdate_text *texts, size_t count, nepdate_ymd *out,
                                        size_t capacity, size_t *written);
nepdate_status nepdate_bulk_to_ad(const nepdate_date *dates, size_t count, nepdate_ymd *out,
                                  size_t capacity, size_t *written);
```

The canonical text form is `YYYY-MM-DD` and the canonical integer form is `YYYYMMDD`.
`nepdate_format_iso` writes the text form with `snprintf` semantics: it returns 10 for a date, the
text is truncated and NUL-terminated when `size` is too small, `size` 0 or a `NULL` buffer writes
nothing, and a date no function made returns 0 and writes nothing. The digits are ASCII even when
the rest of the program formats Nepali text; the interchange form is one string in every language.
C# reaches the same text through a string format or a JSON converter; C uses this pair, with
`nepdate_parse` for the read direction (`shared/spec/PARITY.md`, D-11).

```c
nepdate_date date;
char text[16];
nepdate_from_bs(2081, 4, 15, &date);
nepdate_format_iso(date, text, sizeof text); /* "2081-04-15", returns 10 */
nepdate_parse(text, 10, &date);              /* the same date back */
```

`nepdate_from_ymd_int` reads the packed number: the year in the top decimal places, then the month,
then the day, split with truncating division as the C# type converter does. The three parts are
validated as `nepdate_from_bs` validates them, so the statuses are the same. The argument is
`int64_t` and no part is truncated, so a wider integer ends in `NEPDATE_E_OUT_OF_RANGE` instead of a
wrapped year. `nepdate_to_ymd_int` writes the packed number of a date, and the pair is the whole of
the C# `NepaliDateTypeConverter` surface that survives.

```c
nepdate_date date;
int64_t packed = 0;
nepdate_from_ymd_int(20810415, &date);  /* 2081-04-15 */
nepdate_to_ymd_int(date, &packed);      /* 20810415 */
nepdate_from_ymd_int(99999999, &date);  /* NEPDATE_E_OUT_OF_RANGE */
nepdate_from_ymd_int(INT64_MAX, &date); /* NEPDATE_E_OUT_OF_RANGE, not a wrapped year */
```

`nepdate_bulk_to_bs` converts `count` Gregorian dates into `out`, in input order.
`nepdate_bulk_parse_to_ad` parses `count` texts exactly as `nepdate_parse` does and stores their
Gregorian dates. `nepdate_bulk_to_ad` converts `count` BS dates the other way. Each call writes at
most `capacity` results, so a capacity below `count` truncates the output while `*written` still
reports `count`. Each call is all or nothing: it validates every input first, so on error nothing is
written, `*written` keeps its value, and the status is the one the single call would return. C# has
five batch entry points and does not promise an order above 500 items; this is one ordered function
each way.

```c
const nepdate_ymd ad[3] = {{2024, 7, 30}, {1844, 4, 11}, {2143, 4, 15}};
nepdate_date bs[3];
size_t written = 0;
if (nepdate_bulk_to_bs(ad, 3, bs, 3, &written) == NEPDATE_OK) {
    /* written is 3; bs[0] is 2081-04-15 and bs[2] is 2199-12-30, in input order */
}
```

### Time and the Gregorian type

The platform Gregorian date has three forms here: `nepdate_from_ad` takes a year, a month and a day,
`nepdate_from_unix` takes an instant, and `nepdate_from_tm` takes a broken-down `struct tm`.
`nepdate_to_tm` writes a date back into a `struct tm`.

```c
nepdate_status nepdate_from_unix(int64_t unix_seconds, int32_t utc_offset_minutes,
                                 nepdate_date *out);
nepdate_status nepdate_today_nepal(int64_t unix_seconds, nepdate_date *out);
nepdate_status nepdate_from_tm(const struct tm *tm, nepdate_date *out);
nepdate_status nepdate_to_tm(nepdate_date date, struct tm *out);
```

`nepdate_from_unix` takes an instant as seconds since 1970-01-01 00:00 UTC, adds
`utc_offset_minutes` (east of UTC is positive) and returns the BS date of the resulting local day.
Seconds before the epoch are floored, so an instant at 23:59:59 belongs to that day. It returns
`NEPDATE_E_OUT_OF_RANGE` when the local day is outside the supported range and `NEPDATE_E_NULL`
when `out` is `NULL`; any `int64_t` seconds and `int32_t` offset are accepted, including their
extreme values.

`nepdate_today_nepal` is `nepdate_from_unix` with Nepal's offset at that instant: UTC+05:45 from
1986-01-01 00:00 local (1985-12-31 18:30 UTC) and UTC+05:30 before. The +05:30 rule also covers
instants before 1920, where IANA's tzdata has local mean time (+05:41:16). Neither function reads
the clock; the caller passes the instant.

The `struct tm` pair is declared only when the core is built with `NEPDATE_C_WITH_TM=1`, which is
the default, so a build that leaves the bridge out needs no `<time.h>` at all. `nepdate_from_tm`
reads `tm_year + 1900`, `tm_mon + 1` and `tm_mday` as an AD date; the time of day and `tm_isdst` are
ignored. It returns `NEPDATE_E_NULL`, `NEPDATE_E_OUT_OF_RANGE` for a year outside `int` or the
supported range, and `NEPDATE_E_INVALID_GREGORIAN` for a month or day the Gregorian calendar doesn't
have. `nepdate_to_tm` writes the AD date, midnight in the time fields, `tm_wday` (0 = Sunday),
`tm_yday` (0 = 1 January) and `tm_isdst = -1`; it returns `NEPDATE_E_NULL` or
`NEPDATE_E_INVALID_DATE`, and leaves the object unchanged on error. Neither function reads or writes
a time zone.

```c
#include <time.h>

nepdate_date date;
nepdate_date back;
struct tm broken_down;
const int64_t instant = 1722364200; /* 2024-07-30 18:30 UTC */
nepdate_from_unix(instant, 0, &date); /* 2081-04-15 */
nepdate_today_nepal(instant, &date);  /* 2081-04-16 */
nepdate_to_tm(date, &broken_down);    /* AD 2024-07-31 in tm_year, tm_mon and tm_mday */
nepdate_from_tm(&broken_down, &back); /* the same date again */
```

## Arithmetic

```c
nepdate_status nepdate_add_days(nepdate_date date, int32_t days, nepdate_date *out);
nepdate_status nepdate_add_months(nepdate_date date, int32_t months,
                                  nepdate_overflow overflow, nepdate_date *out);
nepdate_status nepdate_add_years(nepdate_date date, int32_t years,
                                 nepdate_overflow overflow, nepdate_date *out);
nepdate_status nepdate_diff_days(nepdate_date a, nepdate_date b, int32_t *out);
nepdate_status nepdate_diff(nepdate_date a, nepdate_date b, nepdate_date_diff *out);
```

`nepdate_add_days` adds a signed day count. `nepdate_add_months` adds a signed month count, and
`nepdate_add_years` is `12 * years` months. Both use `overflow` when the target month is too short
for the day: `NEPDATE_CLAMP` uses its last day (the C# `AddMonths(n)` default) and
`NEPDATE_OVERFLOW` carries the extra days into the next month. Any `int32_t` amount is accepted.
C# takes a `double` for days and months; here both are integers, because a fraction of a day or a
month has no single meaning (`shared/spec/PARITY.md`, D-02 and D-03).

```c
nepdate_date date;
nepdate_date result;
if (nepdate_from_bs(2081, 4, 32, &date) == NEPDATE_OK &&
    nepdate_add_months(date, 2, NEPDATE_CLAMP, &result) == NEPDATE_OK) {
    /* result is 2081-06-30; NEPDATE_OVERFLOW gives 2081-07-02 */
}
```

`nepdate_diff_days` stores the signed number of days from `a` to `b`, so it is `b - a`: positive
when `b` is later. This is the direction of `days_between` in `shared/spec/ALGORITHM.md`, the
opposite of the C# `Subtract`, which returns `this - other`. The two directions are never
interchangeable (`shared/spec/PARITY.md`, D-07); C has no operator overloading, so this one
function is the whole difference surface.

`nepdate_diff` stores the calendar breakdown of `shared/spec/ALGORITHM.md` section 8a: `sign` is -1
when `b` is earlier and 1 otherwise, and `years`, `months` and `days` are magnitudes. Two equal dates
give sign 1 and zeroes.

```c
nepdate_date a;
nepdate_date b;
int32_t days = 0;
if (nepdate_from_bs(2080, 1, 1, &a) == NEPDATE_OK &&
    nepdate_from_bs(2081, 4, 15, &b) == NEPDATE_OK &&
    nepdate_diff_days(a, b, &days) == NEPDATE_OK) {
    /* days is 473 */
}
```

All of them return `NEPDATE_E_NULL` when the out-pointer is `NULL`, `NEPDATE_E_INVALID_DATE` when a
date is not one this library made, and `NEPDATE_E_OUT_OF_RANGE` when the result is outside the
supported range or `overflow` is neither value (or, for the day difference, when the count would
not fit in `int32_t`). They leave the out object unchanged on error.

## Formatting

```c
size_t nepdate_format(nepdate_date date, const char *pattern, nepdate_lang lang,
                      char *buffer, size_t size);
size_t nepdate_format_iso(nepdate_date date, char *buffer, size_t size);
size_t nepdate_format_short(nepdate_date date, nepdate_order order,
                            nepdate_separator separator, bool pad, nepdate_lang lang,
                            char *buffer, size_t size);
size_t nepdate_format_long(nepdate_date date, nepdate_long_options options,
                           nepdate_lang lang, char *buffer, size_t size);
```

All four write text with `snprintf` semantics: the return value is the number of bytes the full
text needs, the text is truncated and NUL-terminated when `size` is too small, and `size` 0 or a
`NULL` buffer writes nothing. This is the C form of the span formatter: a caller that has room for
the returned length can write the text without a second allocation.
`NEPDATE_NEPALI` selects Devanagari digits and the Nepali names; any other language is English. A
`nepdate_date` no function made returns 0 and writes nothing.

`nepdate_format` follows `shared/spec/FORMATTING.md` section 5. `NULL`, `""`, `"G"`, `"g"` and
`"d"` give the default display `2081/04/15`, `"D"` the long format, and `"s"` the interchange form
`2081-04-15` (which `nepdate_format_iso` writes without a pattern). Any other pattern is scanned for
`yyyy`, `yy`, `MMMM`, `MMM`, `MM`, `M`, `dd`, `d`, `'...'` and a backslash that escapes the next
character; a backslash at the very end of the pattern writes a backslash; other bytes are copied.
`MMM` uses the short English month name and the full Nepali one, and its Ashad and Ashoj forms are
`Asa` and `Aso` (`shared/spec/PARITY.md`, D-04).

```c
nepdate_date date;
char text[64];
nepdate_from_bs(2081, 4, 15, &date);
nepdate_format(date, NULL, NEPDATE_ENGLISH, text, sizeof text); /* "2081/04/15" */
nepdate_format(date, "d", NEPDATE_ENGLISH, text, sizeof text);  /* the default display */
nepdate_format(date, "D", NEPDATE_ENGLISH, text, sizeof text);  /* "Shrawan 15, 2081" */
nepdate_format(date, "s", NEPDATE_ENGLISH, text, sizeof text);  /* "2081-04-15" */
```

`nepdate_format_short` writes the three fields in the order `order`, separated by `separator`, with
a 4-digit year and 2-digit month and day when `pad` is true. An `order` or `separator` outside its
enumerators is treated as `NEPDATE_YMD` or `NEPDATE_SEP_SLASH`. This is `ToString(DateFormats,
Separators, leadingZeros)`.

```c
nepdate_date date;
char text[16];
nepdate_from_bs(2081, 4, 15, &date);
nepdate_format_short(date, NEPDATE_DMY, NEPDATE_SEP_DASH, false, NEPDATE_ENGLISH, text,
                     sizeof text); /* "15-4-2081" */
nepdate_format_short(date, NEPDATE_YMD, NEPDATE_SEP_SLASH, true, NEPDATE_NEPALI, text,
                     sizeof text); /* "२०८१/०४/१५" */
```

`nepdate_format_long` writes `[<weekday>, ]<month> <day>[, <year>]`; its `options` select the
weekday, the year and the padded day. C# defaults are no weekday, the year and the padded day.
`ToUnicodeString` and `ToLongDateUnicodeString` are not separate functions here: the same two
functions with `NEPDATE_NEPALI` give the Devanagari text (`shared/spec/PARITY.md`, D-15).

```c
nepdate_date date;
nepdate_from_bs(2081, 4, 15, &date);
const nepdate_long_options options = {true, true, true};
char text[64];
nepdate_format_long(date, options, NEPDATE_ENGLISH, text, sizeof text);
/* "Tuesday, Shrawan 15, 2081"; NEPDATE_NEPALI gives "मङ्गलवार, साउन १५, २०८१" */
```

## Parsing

```c
nepdate_status nepdate_parse(const char *text, size_t len, nepdate_date *out);
nepdate_status nepdate_parse_lenient(const char *text, size_t len, nepdate_date *out);
```

Both read at most `len` bytes and need no terminator, so a slice of a larger buffer works. `text`
is checked after `out`, and either may not be `NULL` (`NEPDATE_E_NULL`); empty input is a non-`NULL`
pointer with `len` 0. There is no separate `TryParse`: the status return is the non-throwing form,
and the out object is unchanged on failure.

`nepdate_parse` accepts exactly year, month and day in that order (`shared/spec/PARSING.md` section
1): groups of 1 to 4 ASCII or Devanagari digits, separated by `-`, `/`, `.`, `_`, backslash, space,
`|` or the danda `।`. Leading, trailing and repeated separators are ignored. It returns
`NEPDATE_E_INVALID_CHARACTER`, `NEPDATE_E_WRONG_GROUP_COUNT` or `NEPDATE_E_NUMBER_TOO_LONG`, then
validates the date and returns `NEPDATE_E_OUT_OF_RANGE`, `NEPDATE_E_INVALID_MONTH` or
`NEPDATE_E_INVALID_DAY`. The C# constructor with `autoAdjust` and the parser overload that takes it
are not ported; the heuristics have no single answer (`shared/spec/PARITY.md`, D-05 and D-06).

```c
nepdate_date date;
const char text[] = "२०८१।०४।१५";
if (nepdate_parse(text, sizeof text - 1, &date) == NEPDATE_OK) {
    /* date is 2081-04-15: Devanagari digits, separated by a danda */
}
```

`nepdate_parse_lenient` is for human input. It tries `nepdate_parse` first, then drops era markers
and the words `गते` and `मिति`, maps Devanagari digits, accepts a month name with two numbers in any
order, or three numbers as `Y M D` when the first has four digits and `D M Y` when the last does. It
returns `NEPDATE_E_AMBIGUOUS` or `NEPDATE_E_UNRECOGNIZED` for the rest, and a two-digit year is
never expanded (D-06).

```c
nepdate_date date;
const char text[] = "2081 Shrawan 15";
if (nepdate_parse_lenient(text, sizeof text - 1, &date) == NEPDATE_OK) {
    /* date is 2081-04-15 */
}
```

The C# package has one exception type for every failure, `InvalidNepaliDateFormatException`. Here
the failure kind is a `nepdate_status`, so a caller can branch on it without a mapping table.

## Ranges

```c
nepdate_status nepdate_range_make(nepdate_date start, nepdate_date end, nepdate_range *out);
nepdate_status nepdate_range_from_day_count(nepdate_date start, int32_t days,
                                            nepdate_range *out);
nepdate_status nepdate_range_of_month(int year, int month, nepdate_range *out);
nepdate_status nepdate_range_of_year(int year, nepdate_range *out);
nepdate_status nepdate_range_of_fiscal_year(int fiscal_year, nepdate_range *out);
nepdate_status nepdate_range_current_month(int32_t utc_offset_minutes, int64_t unix_seconds,
                                           nepdate_range *out);
nepdate_status nepdate_range_current_fiscal_year(int32_t utc_offset_minutes,
                                                 int64_t unix_seconds, nepdate_range *out);
nepdate_status nepdate_range_current_calendar_year(int32_t utc_offset_minutes,
                                                   int64_t unix_seconds, nepdate_range *out);
nepdate_status nepdate_range_length(nepdate_range range, int *out);
nepdate_status nepdate_range_contains(nepdate_range range, nepdate_date date, bool *out);
nepdate_status nepdate_range_contains_range(nepdate_range range, nepdate_range other, bool *out);
nepdate_status nepdate_range_overlaps(nepdate_range range, nepdate_range other, bool *out);
nepdate_status nepdate_range_is_adjacent_to(nepdate_range range, nepdate_range other, bool *out);
nepdate_status nepdate_range_intersect(nepdate_range range, nepdate_range other,
                                       nepdate_range *out);
nepdate_status nepdate_range_union(nepdate_range range, nepdate_range other, nepdate_range *out);
nepdate_status nepdate_range_except(nepdate_range range, nepdate_range other,
                                    nepdate_range *out, size_t *written);
nepdate_status nepdate_range_split_by_month(nepdate_range range, nepdate_range *out,
                                            size_t capacity, size_t *written);
nepdate_status nepdate_range_split_by_fiscal_quarter(nepdate_range range, nepdate_range *out,
                                                     size_t capacity, size_t *written);
nepdate_status nepdate_range_dates_with_interval(nepdate_range range, int32_t interval,
                                                 nepdate_range_day_cursor *out);
nepdate_status nepdate_range_working_days(nepdate_range range, bool exclude_sunday,
                                          nepdate_range_day_cursor *out);
nepdate_status nepdate_range_weekend_days(nepdate_range range, bool include_sunday,
                                          nepdate_range_day_cursor *out);
nepdate_status nepdate_range_begin(nepdate_range range, nepdate_range_iterator *out);
bool nepdate_range_next(nepdate_range_iterator *iterator, nepdate_date *out);
bool nepdate_range_day_next(nepdate_range_day_cursor *cursor, nepdate_date *out);
size_t nepdate_range_format(nepdate_range range, char *buffer, size_t size);
size_t nepdate_range_format_short(nepdate_range range, nepdate_order order,
                                  nepdate_separator separator, nepdate_lang lang,
                                  char *buffer, size_t size);
nepdate_status nepdate_range_equal(nepdate_range a, nepdate_range b, bool *out);
```

A range is the inclusive span from `start` to `end`. `nepdate_range_make` accepts two valid dates
with `end` not before `start`; a reversed pair returns `NEPDATE_E_OUT_OF_RANGE`, because a range is
never empty. C# can build an empty range because its struct cannot refuse the constructor; here an
invalid range is simply not representable (`shared/spec/PARITY.md`, D-01). A range of one day is
`nepdate_range_make(date, date)`.

```c
nepdate_range range;
if (nepdate_range_make(NEPDATE_MIN, NEPDATE_MAX, &range) == NEPDATE_OK) {
    /* the whole supported span */
}
```

`nepdate_range_from_day_count` is `start` to `start + days - 1`; `days` must be at least 1. The
three `of` functions build a whole BS month, a BS year (Baishakh 1 to the last day of Chaitra) or a
fiscal year. The three `current` functions build the month, fiscal year or BS year that contains
today at the instant and offset the caller passes, like `nepdate_today`.

```c
nepdate_range range;
nepdate_range_of_month(2081, 4, &range);              /* 2081-04-01 to 2081-04-32 */
nepdate_range_from_day_count(NEPDATE_MAX, 1, &range); /* one day */
nepdate_range_from_day_count(NEPDATE_MAX, 2, &range); /* NEPDATE_E_OUT_OF_RANGE */
nepdate_range_current_month(345, 1722364200, &range); /* 2081-04-01 to 2081-04-32 */
```

`nepdate_range_length` counts the days, at least 1. `nepdate_range_contains` tests one date and
`nepdate_range_contains_range` tests whether every day of another range is inside. `overlaps` is
true when the two share a day. `is_adjacent_to` is true when one starts the day after the other
ends; it is false when the day after the earlier range is past 2199-12-30, or the day before the
later one is before 1901-01-01, so a valid range never fails on that test
(`shared/spec/PARITY.md`, D-16).

`nepdate_range_intersect` stores the days in both ranges, and returns `NEPDATE_E_NO_OVERLAP` when
they share none: the port has no empty range to return. `nepdate_range_union` stores the smallest
range that holds every day of both, so a gap between them is inside the result. `except` stores the
days of `range` that are not in `other`: `out` must have room for two ranges, and `*written` is how
many were stored, 0, 1 or 2.

```c
nepdate_range work;
nepdate_range holiday;
nepdate_date first;
nepdate_date last;
nepdate_range_of_month(2081, 4, &work);
nepdate_from_bs(2081, 4, 13, &first);
nepdate_from_bs(2081, 4, 17, &last);
nepdate_range_make(first, last, &holiday);

nepdate_range parts[2];
nepdate_range intersect;
nepdate_range both;
size_t pieces = 0;
bool overlaps = false;
nepdate_range_overlaps(work, holiday, &overlaps);    /* true */
nepdate_range_intersect(work, holiday, &intersect);  /* 2081-04-13 to 2081-04-17 */
nepdate_range_union(work, holiday, &both);           /* 2081-04-01 to 2081-04-32 */
nepdate_range_except(work, holiday, parts, &pieces); /* pieces is 2 */
```

`nepdate_range_split_by_month` writes one range per BS month touched and
`nepdate_range_split_by_fiscal_quarter` one per fiscal quarter. The first and last piece are clipped
to the range ends, so the pieces are contiguous and together are the range. `out` holds up to
`capacity` pieces and `*written` is how many there are, so a capacity below that truncates the
output. Neither steps past 2199-12-30: the last piece ends at the range end (D-16).

```c
nepdate_range range;
nepdate_range pieces[12];
nepdate_date first_day;
nepdate_date last_day;
size_t count = 0;
nepdate_from_bs(2081, 1, 1, &first_day);
nepdate_from_bs(2081, 12, 31, &last_day);
nepdate_range_make(first_day, last_day, &range);
nepdate_range_split_by_month(range, pieces, 12, &count); /* one piece per month touched */
```

`nepdate_range_dates_with_interval` starts a cursor on every `interval` days from `start`;
`interval` must be at least 1, any `int32_t` above it is accepted, and a step at `INT32_MAX` yields
the start day only without overflowing. `nepdate_range_working_days` and `nepdate_range_weekend_days`
start a cursor on the working days (every day except Saturday, and except Sunday too when asked) or
the weekend days (every Saturday, and Sunday too when asked). `nepdate_range_begin` and
`nepdate_range_next` walk every day of a range.

```c
nepdate_range range;
nepdate_range_day_cursor cursor;
nepdate_date day;
nepdate_range_of_month(2081, 4, &range);
nepdate_range_working_days(range, false, &cursor);
while (nepdate_range_day_next(&cursor, &day)) {
    /* every day of Shrawan 2081 except Saturdays */
}
```

`nepdate_range_next` and `nepdate_range_day_next` return false at the end, or when either pointer is
`NULL`, and leave the out object unchanged. Neither can fail in the middle of a valid range.

`nepdate_range_format` writes `start - end` in the default display, for example
`2081/04/10 - 2081/04/20`, and `nepdate_range_format_short` writes the two dates in the requested
order, separator and language. Both use `snprintf` semantics like `nepdate_format`, and a range no
function made returns 0 and writes nothing. `nepdate_range_equal` compares the two bounds.

```c
nepdate_range range;
nepdate_date first;
nepdate_date last;
char text[64];
nepdate_from_bs(2081, 4, 1, &first);
nepdate_from_bs(2081, 4, 32, &last);
nepdate_range_make(first, last, &range);
nepdate_range_format(range, text, sizeof text); /* "2081/04/01 - 2081/04/32" */
nepdate_range_format_short(range, NEPDATE_DMY, NEPDATE_SEP_DASH, NEPDATE_NEPALI, text,
                           sizeof text); /* "०१-०४-२०८१ - ३२-०४-२०८१" */
```

The functions that read a range return `NEPDATE_E_INVALID_DATE` for one no function made and
`NEPDATE_E_NULL` when an out-pointer is `NULL`; the ones that build a range also return
`NEPDATE_E_OUT_OF_RANGE` when a bound is outside the supported span. The out object is unchanged on
error.

## Fiscal year

```c
nepdate_status nepdate_fiscal_year_of(nepdate_date date, int *out);
nepdate_status nepdate_fiscal_year_start(int fiscal_year, nepdate_date *out);
nepdate_status nepdate_fiscal_year_end(int fiscal_year, nepdate_date *out);
nepdate_status nepdate_quarter_of(nepdate_date date, nepdate_quarter *out);
nepdate_status nepdate_quarter_start(int fiscal_year, nepdate_quarter quarter,
                                     nepdate_date *out);
nepdate_status nepdate_quarter_end(int fiscal_year, nepdate_quarter quarter,
                                   nepdate_date *out);
size_t nepdate_fiscal_year_label(int fiscal_year, nepdate_lang lang, char *buffer, size_t size);
```

A fiscal year is named by the BS year it starts in: 2082 runs from 2082-04-01 (Shrawan 1) to the last
day of 2083-03 (Ashad). `nepdate_fiscal_year_of` stores that start year. For dates in Baishakh to
Ashad it is the previous year, so 1901-01-01 is in fiscal year 1900. `nepdate_fiscal_year_start`
and `nepdate_fiscal_year_end` store its first and last days; either can be `NEPDATE_E_OUT_OF_RANGE`
at the ends of the supported span (start of 1900, end of 2199).

Q1 is Shrawan to Ashoj, Q2 Kartik to Poush, Q3 Magh to Chaitra, and Q4 Baishakh to Ashad of the next
year (`shared/spec/ALGORITHM.md` section 8b). `nepdate_quarter_of` stores 1 to 4. The quarter
functions return `NEPDATE_E_OUT_OF_RANGE` for a quarter outside 1 to 4 or a first or last day outside
the supported range. C# also has instance methods with a `yearOffset` and static helpers that take
a calendar year and a month; this port uses the fiscal year as a value and two calls for a pair,
which is `shared/spec/PARITY.md` D-08.

```c
nepdate_date date;
nepdate_date start;
nepdate_date end;
int fiscal_year = 0;
if (nepdate_from_bs(2081, 1, 10, &date) == NEPDATE_OK &&
    nepdate_fiscal_year_of(date, &fiscal_year) == NEPDATE_OK &&
    nepdate_fiscal_year_start(fiscal_year, &start) == NEPDATE_OK &&
    nepdate_fiscal_year_end(fiscal_year, &end) == NEPDATE_OK) {
    /* fiscal_year is 2080; start is 2080-04-01 and end is 2081-03-32 */
}
```

`nepdate_fiscal_year_label` writes the label with `snprintf` semantics: it returns the length the
full label needs, truncates and NUL-terminates when `size` is too small, and writes nothing when
`size` is 0 or `buffer` is `NULL`. `NEPDATE_NEPALI` uses Devanagari digits; any other language uses
ASCII. A negative year gets a leading `-`, and a 39-byte buffer always fits.

```c
char label[40];
const size_t length = nepdate_fiscal_year_label(2082, NEPDATE_NEPALI, label, sizeof label);
printf("%s (%zu bytes)\n", label, length);
/* २०८२/८३ (19 bytes) */
```

`nepdate_fiscal_year_of` and `nepdate_quarter_of` return `NEPDATE_E_NULL` for a `NULL` out-pointer
and `NEPDATE_E_INVALID_DATE` for a date no function made. The four functions that build a date from
a fiscal year or a quarter return `NEPDATE_E_NULL` and `NEPDATE_E_OUT_OF_RANGE`. All of them leave
the out object unchanged on error.

## Patro

The patro library is a separate package and target, `nepdate-c::patro`, with header
`<nepdate_patro.h>` and pkg-config name `nepdate-c-patro`. It depends on the core; the core never
depends on it (hub ADR-0004). Its data covers BS 2077-01-01 to 2084-08-20.

```c
typedef struct nepdate_patro_name {
    const char *np;
    size_t np_length;
    const char *en;
    size_t en_length;
} nepdate_patro_name;

typedef struct nepdate_patro_day_info {
    bool has_tithi;
    nepdate_patro_name tithi;
    bool is_public_holiday;
    size_t event_count;
} nepdate_patro_day_info;

nepdate_status nepdate_patro_info(nepdate_date date, nepdate_patro_day_info *out);
nepdate_range nepdate_patro_coverage(void);
size_t nepdate_patro_public_holidays(nepdate_date *dates, size_t capacity);
size_t nepdate_patro_events(nepdate_date date, nepdate_patro_name *events, size_t capacity);
```

A `nepdate_patro_name` holds its Nepali and English UTF-8 bytes and their lengths; the bytes are
not NUL-terminated. A name with no value has both pointers `NULL` and both lengths 0.

`nepdate_patro_info` returns what the patro says about a day: its tithi (if it has one), whether it
is a gazetted public holiday, and how many events it has. A day outside the coverage has no tithi,
no holiday and no events. It returns `NEPDATE_E_NULL` or `NEPDATE_E_INVALID_DATE`, and leaves
`*out` unchanged on error.

`nepdate_patro_coverage` returns the range the generated tables cover, the `first_day` to `last_day`
range of `shared/spec/vectors/patro-summary.json`, and can't fail.

`nepdate_patro_public_holidays` writes the public holiday dates in ascending order and returns how
many there are, the summary's `holidays` count. It writes at most `capacity` dates; `dates` `NULL`
or `capacity` 0 writes nothing and still returns the count.

`nepdate_patro_events` writes a day's events in file (display) order and returns their number. It
writes at most `capacity` names; `events` `NULL` or `capacity` 0 writes nothing and still returns
the count. A date no function made, or a day outside the coverage, returns 0 and writes nothing.

```c
nepdate_date date;
nepdate_from_bs(2081, 4, 15, &date);
nepdate_patro_day_info info;
nepdate_patro_name events[4];
if (nepdate_patro_info(date, &info) == NEPDATE_OK && info.event_count <= 4) {
    (void)nepdate_patro_events(date, events, 4);
}
```

## C members and their C functions

The first column is the member of the C# package, as the hub's `spec/API.md` writes it.

| C# member | C functions |
|---|---|
| `NepaliDate(int yearBs, int monthBs, int dayBs)` | `nepdate_from_bs` |
| `NepaliDate(string rawNepaliDate)` | `nepdate_parse` |
| `NepaliDate(DateTime engDate)` | `nepdate_from_ad`, `nepdate_from_unix`, `nepdate_from_tm` |
| `bool IsDefault { get; }` | no member: a zero value is invalid (`NEPDATE_E_INVALID_DATE`) |
| `int Year`, `int Month`, `int Day` | `nepdate_to_bs` |
| `DateTime EnglishDate` | `nepdate_to_ad` |
| `DayOfWeek DayOfWeek` | `nepdate_weekday_of` |
| `int DayOfYear` | `nepdate_day_of_year` |
| `int MonthEndDay` | `nepdate_month_length` |
| `NepaliMonths MonthName` | `nepdate_month_of` |
| `static NepaliDate Today`, `static NepaliDate Now` | `nepdate_today`, `nepdate_today_nepal` |
| `static readonly NepaliDate MinValue`, `MaxValue` | `NEPDATE_MIN`, `NEPDATE_MAX` |
| `NepaliDate MonthEndDate()` | `nepdate_last_day_of_month` |
| `void Deconstruct(out int, out int, out int)` | `nepdate_to_bs` |
| `bool IsLeapYear()` | `nepdate_year_length` (D-12) |
| `bool IsToday()`, `IsYesterday()`, `IsTomorrow()` | `nepdate_is_today`, `nepdate_is_yesterday`, `nepdate_is_tomorrow` |
| `int CompareTo(NepaliDate)` | `nepdate_compare` |
| `bool Equals(NepaliDate)`, `operator ==`, `!=`, `<`, `<=`, `>`, `>=` | `nepdate_compare` |
| `override int GetHashCode()` | no member: C has no hash protocol |
| `static NepaliDate DateTimeExtensions.ToNepaliDate(this DateTime)` | `nepdate_from_ad` |
| `static NepaliDate SmartParsingExtensions.ToNepaliDate(this string)` | `nepdate_parse_lenient` |
| `static bool ...TryToNepaliDate(this string, out NepaliDate)` | `nepdate_parse_lenient` status |
| `static IEnumerable<NepaliDate> BulkConvert.ToNepaliDates(...)` | `nepdate_bulk_to_bs` |
| `static IEnumerable<DateTime> ...ToEnglishDates(...)` | `nepdate_bulk_parse_to_ad`, `nepdate_bulk_to_ad` |
| `static IEnumerable<...> ...BatchProcessTo...(...)` | the same two functions |
| `sealed class NepaliDateTypeConverter` | `nepdate_from_ymd_int`, `nepdate_to_ymd_int` |
| `class SystemTextJsonConverters.NepaliDateJsonConverter` | `nepdate_format_iso`, `nepdate_parse` |
| `NepaliDate AddMonths(double, bool)`, `AddYears(int, bool)` | `nepdate_add_months`, `nepdate_add_years` |
| `NepaliDate AddDays(double)` | `nepdate_add_days` |
| `TimeSpan Subtract(NepaliDate)`, `operator -` | `nepdate_diff_days` |
| `override string ToString()` | `nepdate_format` with `NULL` |
| `string ToString(DateFormats, Separators, bool)` | `nepdate_format_short` |
| `string ToLongDateString(bool, bool, bool)` | `nepdate_format_long` |
| `string ToUnicodeString(...)`, `ToLongDateUnicodeString(...)` | the same two with `NEPDATE_NEPALI` |
| `string ToString(string format, IFormatProvider)` | `nepdate_format` |
| `bool ISpanFormattable.TryFormat(...)` | every formatter, with `snprintf` semantics |
| `enum DateFormats` | `nepdate_order` |
| `enum Separators` | `nepdate_separator` |
| `enum NepaliMonths` | `nepdate_month` |
| `static NepaliDate Parse(string)` | `nepdate_parse` |
| `static bool TryParse(string, out NepaliDate)` | `nepdate_parse` status |
| `static NepaliDate SmartDateParser.Parse(string)` | `nepdate_parse_lenient` |
| `static bool SmartDateParser.TryParse(string, out NepaliDate)` | `nepdate_parse_lenient` status |
| `sealed class InvalidNepaliDateFormatException` | `nepdate_status` |
| `readonly struct NepaliDateRange` | `nepdate_range` |
| `NepaliDateRange(NepaliDate, NepaliDate)` | `nepdate_range_make` |
| `NepaliDate Start`, `NepaliDate End` | `range.start`, `range.end` |
| `bool IsEmpty` | no member: a range is never empty (D-01) |
| `int Length` | `nepdate_range_length` |
| `static NepaliDateRange SingleDay(NepaliDate)` | `nepdate_range_make(date, date)` |
| `static NepaliDateRange FromDayCount(NepaliDate, int)` | `nepdate_range_from_day_count` |
| `static NepaliDateRange ForMonth(int, int)` | `nepdate_range_of_month` |
| `static NepaliDateRange ForFiscalYear(int)` | `nepdate_range_of_fiscal_year` |
| `static NepaliDateRange ForCalendarYear(int)` | `nepdate_range_of_year` |
| `static NepaliDateRange CurrentMonth()`, `CurrentFiscalYear()`, `CurrentCalendarYear()` | the three `nepdate_range_current_*` functions |
| `bool Contains(NepaliDate)` | `nepdate_range_contains` |
| `bool Contains(NepaliDateRange)` | `nepdate_range_contains_range` |
| `bool Overlaps(...)`, `IsAdjacentTo(...)` | `nepdate_range_overlaps`, `nepdate_range_is_adjacent_to` |
| `NepaliDateRange Intersect(...)`, `Union(...)` | `nepdate_range_intersect`, `nepdate_range_union` |
| `NepaliDateRange[] Except(...)` | `nepdate_range_except` |
| `NepaliDateRange[] SplitByMonth()`, `SplitByFiscalQuarter()` | `nepdate_range_split_by_month`, `nepdate_range_split_by_fiscal_quarter` |
| `IEnumerable<NepaliDate> DatesWithInterval(int)` | `nepdate_range_dates_with_interval`, `nepdate_range_day_next` |
| `IEnumerable<NepaliDate> WorkingDays(bool)`, `WeekendDays(bool)` | `nepdate_range_working_days`, `nepdate_range_weekend_days` |
| `IEnumerator<NepaliDate> GetEnumerator()` | `nepdate_range_begin`, `nepdate_range_next` |
| `override string ToString()`, `ToString(DateFormats, Separators)` | `nepdate_range_format`, `nepdate_range_format_short` |
| `Equals`, `GetHashCode`, `operator ==`, `!=` | `nepdate_range_equal` |
| `enum FiscalYearQuarters` | `nepdate_quarter` |
| `FiscalYearStartDate(int)`, `FiscalYearEndDate(int)` | `nepdate_fiscal_year_start`, `nepdate_fiscal_year_end` |
| `FiscalYearStartAndEndDate(int)` | the two calls above |
| `FiscalYearQuarterStartDate(...)`, `FiscalYearQuarterEndDate(...)` | `nepdate_quarter_start`, `nepdate_quarter_end` |
| `static NepaliDate GetFiscalYearStartDate(int)`, `GetFiscalYearEndDate(int)` | the same two functions |
| `readonly struct CalendarInfo` | `nepdate_patro_day_info` |
| `string TithiNp`, `TithiEn` | `info.tithi.np`, `info.tithi.en` |
| `bool IsPublicHoliday` | `info.is_public_holiday` |
| `string[] EventsNp`, `EventsEn` | `nepdate_patro_events` |
| `CalendarInfo GetCalendarInfo()` | `nepdate_patro_info` |

## Linking

| Build | Define in the user's code |
|---|---|
| Shared library through CMake or pkg-config | nothing |
| Static library through CMake or pkg-config | nothing (the target and the `.pc` file add `NEPDATE_STATIC`) |
| Sources compiled into the user's project | `NEPDATE_STATIC` |
| Patro shared library | link `nepdate-c::patro` or use `nepdate-c-patro` from pkg-config; it depends on the core |
| Patro static library | nothing extra; the target and `nepdate-c-patro.pc` add `NEPDATE_PATRO_STATIC` |
| Patro installed on its own (vcpkg, Conan) | `find_package(nepdate-c-patro CONFIG REQUIRED)` gives the `nepdate-c::patro` target |
| Patro sources compiled into the user's project | `NEPDATE_PATRO_STATIC` |
