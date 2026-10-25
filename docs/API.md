# API

The public surface of `nepdate-c`. Everything listed here is declared in `<nepdate.h>`; nothing
else is exported (ADR-0002). The API-surface gate compares the shared library's export table with
the functions on this page.

## Version

```c
#define NEPDATE_VERSION_MAJOR 0
#define NEPDATE_VERSION_MINOR 0
#define NEPDATE_VERSION_PATCH 0
#define NEPDATE_VERSION_PRERELEASE ""
#define NEPDATE_VERSION_STRING "0.0.0"

const char *nepdate_version(void);
```

The macros give the version of the header. `NEPDATE_VERSION_PRERELEASE` is the part after `-` in
a pre-release such as `0.1.0-beta.1`, and empty for a stable version.

`nepdate_version` returns the version of the compiled library as a static string, equal to
`NEPDATE_VERSION_STRING` when the header and the library come from the same release. It can't
fail.

## Conventions

Functions return a `nepdate_status` and write their result through the last argument, a pointer
the caller owns. The pointer is checked first (`NEPDATE_E_NULL`), and on any error the object it
points to is left unchanged. No function allocates, keeps state or reads the clock, so every
function is thread-safe.

The supported range is BS 1901-01-01 to 2199-12-30, which is AD 1844-04-11 to 2143-04-15
(`shared/spec/ALGORITHM.md`).

## Types

```c
#define NEPDATE_MIN_YEAR 1901
#define NEPDATE_MAX_YEAR 2199

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
    NEPDATE_E_NULL = 11
} nepdate_status;

typedef struct nepdate_date { int32_t value; } nepdate_date;
typedef struct nepdate_ymd { int year; int month; int day; } nepdate_ymd;

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
```

| Status | Meaning |
|---|---|
| `NEPDATE_E_OUT_OF_RANGE` | a BS year outside 1901 to 2199, an AD date outside 1844-04-11 to 2143-04-15, an arithmetic result outside the supported range, or an overflow mode or quarter outside its values |
| `NEPDATE_E_INVALID_MONTH` | a BS month outside 1 to 12 |
| `NEPDATE_E_INVALID_DAY` | a BS day outside 1 to the length of its month |
| `NEPDATE_E_INVALID_GREGORIAN` | an AD year, month and day that is not a proleptic Gregorian date |
| `NEPDATE_E_INVALID_CHARACTER` to `NEPDATE_E_UNRECOGNIZED` | parse errors (`shared/spec/PARSING.md` section 4), used by the parsing functions |
| `NEPDATE_E_INVALID_DATE` | a `nepdate_date` or `nepdate_range` that no function of this library made |
| `NEPDATE_E_NULL` | a required pointer is `NULL` |

`nepdate_date` is a 4-byte value passed by value. Its `value` is private: 1 plus the number of days
since BS 1901-01-01 (ADR-0006), so a zero-initialised `nepdate_date` is not a date and every function
rejects it with `NEPDATE_E_INVALID_DATE`. Every function that takes a `nepdate_date` accepts any
bit pattern without undefined behaviour.

`nepdate_ymd` holds a year, a month (1 to 12) and a day, in either calendar.
`nepdate_month` numbers the BS months 1 to 12 with Baishakh first. `nepdate_weekday` numbers the
days 0 to 6 with Sunday first (`shared/spec/ALGORITHM.md` section 7). `nepdate_quarter` numbers the
fiscal quarters, `nepdate_overflow` chooses what month arithmetic does with a day the target month
does not have, and `nepdate_lang` selects English or Nepali text.

`nepdate_range` holds two valid dates with `start` not after `end`; a range is never empty. Make one
with `nepdate_range_make` or a `nepdate_range_of_*` function, not by filling the fields. A range
that no function made is rejected with `NEPDATE_E_INVALID_DATE`.

```c
const char *nepdate_status_name(nepdate_status status);
```

Returns the enumerator's name as a static string, such as `"NEPDATE_E_INVALID_DAY"`, or
`"unknown"` for a value that is not one.

## Building and reading a date

```c
nepdate_status nepdate_from_bs(int year, int month, int day, nepdate_date *out);
nepdate_status nepdate_to_bs(nepdate_date date, nepdate_ymd *out);
```

`nepdate_from_bs` makes the BS date `year-month-day`. It checks, in this order: `NEPDATE_E_NULL`,
`NEPDATE_E_OUT_OF_RANGE` for the year, `NEPDATE_E_INVALID_MONTH`, `NEPDATE_E_INVALID_DAY`. Any
`int` is accepted, `INT_MIN` and `INT_MAX` included.

`nepdate_to_bs` stores the BS year, month and day. It fails with `NEPDATE_E_NULL` or
`NEPDATE_E_INVALID_DATE`.

```c
nepdate_date date;
nepdate_ymd bs;
if (nepdate_from_bs(2081, 4, 32, &date) == NEPDATE_OK &&
    nepdate_to_bs(date, &bs) == NEPDATE_OK) {
    /* bs is {2081, 4, 32}; nepdate_from_bs(2081, 4, 33, &date) is NEPDATE_E_INVALID_DAY */
}
```

## AD conversion and derived properties

```c
nepdate_status nepdate_from_ad(int year, int month, int day, nepdate_date *out);
nepdate_status nepdate_to_ad(nepdate_date date, nepdate_ymd *out);
nepdate_status nepdate_weekday_of(nepdate_date date, nepdate_weekday *out);
nepdate_status nepdate_month_of(nepdate_date date, nepdate_month *out);
nepdate_status nepdate_day_of_year(nepdate_date date, int *out);
nepdate_status nepdate_month_length(nepdate_date date, int *out);
nepdate_status nepdate_first_day_of_month(nepdate_date date, nepdate_date *out);
nepdate_status nepdate_last_day_of_month(nepdate_date date, nepdate_date *out);
nepdate_status nepdate_compare(nepdate_date a, nepdate_date b, int *out);
```

`nepdate_from_ad` makes the proleptic Gregorian date. It checks, in this order: `NEPDATE_E_NULL`,
`NEPDATE_E_INVALID_GREGORIAN` for a month or day the calendar doesn't have, then
`NEPDATE_E_OUT_OF_RANGE` for a real date outside 1844-04-11 to 2143-04-15. So 2023-02-29 is
`NEPDATE_E_INVALID_GREGORIAN` and 1844-04-10 is `NEPDATE_E_OUT_OF_RANGE`. Any `int` is accepted,
`INT_MIN` and `INT_MAX` included. `nepdate_to_ad` returns the AD year, month and day of a date.

`nepdate_weekday_of` and `nepdate_month_of` return the enumerators above. `nepdate_day_of_year` is 1
to 367, `nepdate_month_length` 29 to 32, and `nepdate_first_day_of_month` and
`nepdate_last_day_of_month` return dates in the same BS month. `nepdate_compare` stores -1, 0 or 1
for `a` before, equal to, or after `b`.

All of them return `NEPDATE_E_NULL` when an out-pointer is `NULL` and `NEPDATE_E_INVALID_DATE` when
they are given a `nepdate_date` no function made, and leave the out object unchanged on error.
`nepdate_compare` returns `NEPDATE_E_INVALID_DATE` when either date is invalid.

```c
nepdate_date date;
nepdate_ymd ad;
nepdate_weekday weekday;
if (nepdate_from_ad(2024, 7, 30, &date) == NEPDATE_OK &&
    nepdate_to_ad(date, &ad) == NEPDATE_OK &&
    nepdate_weekday_of(date, &weekday) == NEPDATE_OK) {
    /* ad is {2024, 7, 30}; weekday is NEPDATE_TUESDAY */
}
```

## Arithmetic

```c
nepdate_status nepdate_add_days(nepdate_date date, int32_t days, nepdate_date *out);
nepdate_status nepdate_add_months(nepdate_date date, int32_t months,
                                  nepdate_overflow overflow, nepdate_date *out);
nepdate_status nepdate_add_years(nepdate_date date, int32_t years,
                                 nepdate_overflow overflow, nepdate_date *out);
```

`nepdate_add_days` adds a signed day count. `nepdate_add_months` adds a signed month count, and
`nepdate_add_years` is `12 * years` months. Both use `overflow` when the target month is too short
for the day: `NEPDATE_CLAMP` uses its last day (the C# `AddMonths(n)` default) and
`NEPDATE_OVERFLOW` carries the extra days into the next month. Any `int32_t` amount is accepted.

They return `NEPDATE_E_NULL`, `NEPDATE_E_INVALID_DATE`, and `NEPDATE_E_OUT_OF_RANGE` when the
result is outside the supported range, or `overflow` is neither value. `NEPDATE_E_OUT_OF_RANGE` can
also mean that the spill from the last supported month would fall past the table.

```c
nepdate_date date;
nepdate_date result;
if (nepdate_from_bs(2081, 4, 32, &date) == NEPDATE_OK &&
    nepdate_add_months(date, 2, NEPDATE_CLAMP, &result) == NEPDATE_OK) {
    /* result is 2081-06-30; NEPDATE_OVERFLOW gives 2081-07-02 */
}
```

## Difference

```c
nepdate_status nepdate_diff_days(nepdate_date a, nepdate_date b, int32_t *out);
nepdate_status nepdate_diff(nepdate_date a, nepdate_date b, nepdate_date_diff *out);
```

`nepdate_diff_days` stores `b - a` in days: positive when `b` is later, negative when it is earlier.
`nepdate_diff` stores the calendar breakdown of `shared/spec/ALGORITHM.md` section 8a: `sign` is -1
when `b` is earlier and 1 otherwise, and `years`, `months` and `days` are magnitudes. Two equal dates
give sign 1 and zeroes.

Both return `NEPDATE_E_NULL` when the out-pointer is `NULL` and `NEPDATE_E_INVALID_DATE` when either
date is not one this library made. The out object is unchanged on error.

```c
nepdate_date a;
nepdate_date b;
nepdate_date_diff diff;
int32_t days;
if (nepdate_from_bs(2080, 1, 1, &a) == NEPDATE_OK &&
    nepdate_from_bs(2081, 4, 15, &b) == NEPDATE_OK &&
    nepdate_diff(a, b, &diff) == NEPDATE_OK &&
    nepdate_diff_days(a, b, &days) == NEPDATE_OK) {
    /* diff is {1, 1, 3, 14}; days is 473 */
}
```

## Fiscal year and quarters

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
the supported range.

`nepdate_fiscal_year_label` writes the label with `snprintf` semantics: it returns the length the
full label needs, truncates and NUL-terminates when `size` is too small, and writes nothing when
`size` is 0. `NEPDATE_NEPALI` uses Devanagari digits; any other language uses ASCII. A negative year
gets a leading `-`, and a 39-byte buffer always fits.

```c
char label[40];
const size_t length = nepdate_fiscal_year_label(2082, NEPDATE_NEPALI, label, sizeof label);
/* label is "२०८२/८३" and length is 19 */
```

`nepdate_fiscal_year_of` and `nepdate_quarter_of` return `NEPDATE_E_NULL` for a `NULL` out-pointer
and `NEPDATE_E_INVALID_DATE` for a date no function made. The four functions that build a date from
a fiscal year or a quarter return `NEPDATE_E_NULL` and `NEPDATE_E_OUT_OF_RANGE`. All of them leave
the out object unchanged on error.

## Ranges

```c
nepdate_status nepdate_range_make(nepdate_date start, nepdate_date end, nepdate_range *out);
nepdate_status nepdate_range_of_month(int year, int month, nepdate_range *out);
nepdate_status nepdate_range_of_year(int year, nepdate_range *out);
nepdate_status nepdate_range_of_fiscal_year(int fiscal_year, nepdate_range *out);
nepdate_status nepdate_range_length(nepdate_range range, int *out);
nepdate_status nepdate_range_contains(nepdate_range range, nepdate_date date, bool *out);
nepdate_status nepdate_range_begin(nepdate_range range, nepdate_range_iterator *out);
bool nepdate_range_next(nepdate_range_iterator *iterator, nepdate_date *out);
```

A range is the inclusive span from `start` to `end`. `nepdate_range_make` accepts two valid dates
with `end` not before `start`; a reversed pair returns `NEPDATE_E_OUT_OF_RANGE`, because a range is
never empty. The three `of` functions build a whole month, a BS year (Baishakh 1 to the last day of
Chaitra) or a fiscal year. `nepdate_range_length` counts the days, at least 1, and
`nepdate_range_contains` tests a date.

`nepdate_range_begin` makes a cursor on the first day and `nepdate_range_next` stores the next day and
advances it. `nepdate_range_next` returns false at the end, or when either pointer is `NULL`; it
leaves the out object unchanged when it returns false. The cursor is a plain loop over day values,
so it needs no allocation and cannot fail in the middle of a valid range.

```c
nepdate_range range;
nepdate_range_iterator it;
nepdate_date day;
if (nepdate_range_of_month(2081, 4, &range) == NEPDATE_OK &&
    nepdate_range_begin(range, &it) == NEPDATE_OK) {
    while (nepdate_range_next(&it, &day)) {
        /* every day of Shrawan 2081, 32 of them */
    }
}
```

`nepdate_range_length`, `nepdate_range_contains` and `nepdate_range_begin` return
`NEPDATE_E_NULL` for a `NULL` out-pointer and `NEPDATE_E_INVALID_DATE` for a range no function made;
`nepdate_range_contains` also rejects a date no function made. The out object is unchanged on error.

Ranges have no set operations, splitting or working-day count (PARITY D-09): iterate and filter.

## Linking

| Build | Define in the user's code |
|---|---|
| Shared library through CMake or pkg-config | nothing |
| Static library through CMake or pkg-config | nothing (the target and the `.pc` file add `NEPDATE_STATIC`) |
| Sources compiled into the user's project | `NEPDATE_STATIC` |
