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
```

| Status | Meaning |
|---|---|
| `NEPDATE_E_OUT_OF_RANGE` | a BS year outside 1901 to 2199, or an AD date outside 1844-04-11 to 2143-04-15 |
| `NEPDATE_E_INVALID_MONTH` | a BS month outside 1 to 12 |
| `NEPDATE_E_INVALID_DAY` | a BS day outside 1 to the length of its month |
| `NEPDATE_E_INVALID_GREGORIAN` | an AD year, month and day that is not a proleptic Gregorian date |
| `NEPDATE_E_INVALID_CHARACTER` to `NEPDATE_E_UNRECOGNIZED` | parse errors (`shared/spec/PARSING.md` section 4), used by the parsing functions |
| `NEPDATE_E_INVALID_DATE` | a `nepdate_date` that no function of this library made |
| `NEPDATE_E_NULL` | a required pointer is `NULL` |

`nepdate_date` is a 4-byte value passed by value. Its `value` is private: 1 plus the number of days
since BS 1901-01-01 (ADR-0006), so a zero-initialised `nepdate_date` is not a date and every function
rejects it with `NEPDATE_E_INVALID_DATE`. Every function that takes a `nepdate_date` accepts any
bit pattern without undefined behaviour.

`nepdate_ymd` holds a year, a month (1 to 12) and a day, in either calendar.

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

## Linking

| Build | Define in the user's code |
|---|---|
| Shared library through CMake or pkg-config | nothing |
| Static library through CMake or pkg-config | nothing (the target and the `.pc` file add `NEPDATE_STATIC`) |
| Sources compiled into the user's project | `NEPDATE_STATIC` |
