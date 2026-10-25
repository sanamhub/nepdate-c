/* nepdate-c: Bikram Sambat (Nepali calendar) dates for C. The public surface is docs/API.md. */
#ifndef NEPDATE_H
#define NEPDATE_H

#define NEPDATE_VERSION_MAJOR 0
#define NEPDATE_VERSION_MINOR 0
#define NEPDATE_VERSION_PATCH 0
/* The part after "-" in a pre-release version such as 0.1.0-beta.1, empty for a stable one. */
#define NEPDATE_VERSION_PRERELEASE ""
#define NEPDATE_VERSION_STRING "0.0.0"

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
     * 2143-04-15, or a result outside the supported range. */
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
    /* A required pointer argument is NULL. */
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

#ifdef __cplusplus
}
#endif

#endif /* NEPDATE_H */
