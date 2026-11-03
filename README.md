<p align="center">
  <img src="https://raw.githubusercontent.com/sanamhub/nepdate-c/main/docs/assets/nepdate-c-128.png" width="128" alt="NepDate for C logo">
</p>

<h1 align="center">nepdate-c</h1>

<p align="center">
  <a href="https://github.com/sanamhub/nepdate-c/actions/workflows/ci.yml"><img src="https://github.com/sanamhub/nepdate-c/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <img src="https://img.shields.io/badge/C-11-283593?logo=c" alt="C11">
  <a href="https://github.com/sanamhub/nepdate-c/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue" alt="License: MIT"></a>
</p>

Bikram Sambat (Nepali calendar) for C. Part of the NepDate family: shared data and spec live in the
hub [`sanamhub/nepdate`](https://github.com/sanamhub/nepdate); the C# original is
[NepDate](https://github.com/RajuPrasai/NepDate).

Plain ISO C11 with no allocation, no global state and no dependencies, so it also builds into Zig
projects and into extensions for languages that call C.

> **Status:** `0.2.0-rc.1`, a release candidate. The API can still change before `0.2.0`; the other
> [released languages](https://github.com/sanamhub/nepdate#packages) follow the same versions.

## Build

CMake 3.25 or newer, Ninja and a C11 compiler (GCC 13, Clang 18, MSVC 2022, `zig cc`, or newer):

```sh
cmake --workflow --preset dev
```

`cmake --install` gives a CMake package and a pkg-config file, both named `nepdate-c`. The patro
(tithi, holidays and events) is a second target in the same package:

```cmake
find_package(nepdate-c CONFIG REQUIRED)
target_link_libraries(app PRIVATE nepdate-c::nepdate)
# optional: target_link_libraries(app PRIVATE nepdate-c::patro)
```

## Quick start

```c
#include <nepdate.h>
#include <stdio.h>

int main(void) {
    nepdate_date date;
    nepdate_ymd bs;
    if (nepdate_from_ad(2024, 7, 30, &date) != NEPDATE_OK ||
        nepdate_to_bs(date, &bs) != NEPDATE_OK) {
        return 1;
    }
    printf("BS %04d-%02d-%02d\n", bs.year, bs.month, bs.day);
    return 0;
}
```

Every function that can fail returns a `nepdate_status` and writes its result through a pointer,
which it leaves unchanged on error. Nothing allocates and nothing reads the clock, so every call is
thread-safe and every clock-dependent call takes the instant from you. `nepdate_version` gives the
version of the library, for a log or a bug report. The full list of names, with signatures and an
example each, is [docs/API.md](https://github.com/sanamhub/nepdate-c/blob/main/docs/API.md).

## Dates

`nepdate_from_bs` makes a date from a BS year, month and day, `nepdate_from_ad` from a Gregorian
one, and `nepdate_to_bs` and `nepdate_to_ad` read either calendar back. The supported span is
1901-01-01 to 2199-12-30 BS, which is 1844-04-11 to 2143-04-15 AD. `NEPDATE_MIN` and `NEPDATE_MAX`
are its two ends, and `nepdate_today` takes the instant and the UTC offset.

```c
nepdate_date date;
nepdate_ymd bs;
nepdate_ymd ad;
int day_of_year;
nepdate_from_bs(2081, 4, 32, &date);     /* Shrawan 2081 has 32 days */
nepdate_to_ad(date, &ad);                /* {2024, 8, 16} */
nepdate_to_bs(date, &bs);                /* {2081, 4, 32} */
nepdate_day_of_year(date, &day_of_year); /* 32 */

nepdate_date today;
bool is_today = false;
nepdate_today(345, 1722364200, &today);              /* 2081-04-16, Nepal, UTC+05:45 */
nepdate_is_today(today, 345, 1722364200, &is_today); /* true */
```

Derived values are separate functions, because C has no properties: `nepdate_weekday_of`,
`nepdate_month_of`, `nepdate_day_of_year`, `nepdate_month_length`, `nepdate_year_length`,
`nepdate_first_day_of_month` and `nepdate_last_day_of_month`. `nepdate_year_length` is 364 to 367 and
replaces the C# `IsLeapYear`, which follows the Gregorian year and changes in the middle of a BS
year.

## Conversion and serialization

The canonical text is `YYYY-MM-DD`. `nepdate_format_iso` writes it into your buffer with `snprintf`
rules and `nepdate_parse` reads it back. The canonical integer is `YYYYMMDD`:
`nepdate_to_ymd_int` writes it and `nepdate_from_ymd_int` reads it, with no part truncated. Batch
conversion is ordered and takes caller buffers.

```c
char text[16];
int64_t packed;
nepdate_date date;
nepdate_format_iso(NEPDATE_MAX, text, sizeof text); /* "2199-12-30", returns 10 */
nepdate_to_ymd_int(NEPDATE_MAX, &packed);           /* 21991230 */
nepdate_from_ymd_int(20810415, &date);              /* 2081-04-15 */

const nepdate_ymd ad[2] = {{2024, 7, 30}, {1844, 4, 11}};
nepdate_date bs[2];
size_t written = 0;
nepdate_bulk_to_bs(ad, 2, bs, 2, &written); /* 2081-04-15 and 1901-01-01, in input order */
```

A JSON, database or text hook is this pair: format to a string, parse from a string. There is no
object form and no converter type, which is the native-hook rule the family follows. The other
conversion forms take the platform date: `nepdate_from_unix` takes a Unix instant and a UTC offset,
`nepdate_today_nepal` knows Nepal's own offset at that instant, and `nepdate_from_tm` and
`nepdate_to_tm` bridge to `struct tm` when the build keeps that part.

## Arithmetic

`nepdate_add_days`, `nepdate_add_months` and `nepdate_add_years` take a signed count and write the
new date. Month arithmetic takes `NEPDATE_CLAMP`, which uses the last day of a shorter target month
(the C# default), or `NEPDATE_OVERFLOW`, which carries the extra days into the next month.

```c
nepdate_date date;
nepdate_date moved;
nepdate_from_bs(2081, 4, 32, &date);
nepdate_add_months(date, 2, NEPDATE_CLAMP, &moved);    /* 2081-06-30 */
nepdate_add_months(date, 2, NEPDATE_OVERFLOW, &moved); /* 2081-07-02 */
nepdate_add_days(date, -1, &moved);                    /* 2081-04-31 */
```

`nepdate_diff_days` gives the signed day count from the first date to the second, which is
`second - first`, and `nepdate_diff` gives the calendar years, months and days with a sign.

```c
int32_t days;
nepdate_date a;
nepdate_date b;
nepdate_from_bs(2080, 1, 1, &a);
nepdate_from_bs(2081, 4, 15, &b);
nepdate_diff_days(a, b, &days); /* 473 */
```

## Formatting

`nepdate_format` takes the pattern language, `nepdate_format_short` the six field orders and six
separators, and `nepdate_format_long` the long form with optional weekday and year. All three write
into your buffer, return the length the full text needs, and truncate safely, so one call with a
small buffer tells you the size you need. `NEPDATE_NEPALI` gives Devanagari digits and Nepali names.

```c
char text[64];
nepdate_date date;
const nepdate_long_options long_options = {true, true, true};
nepdate_from_bs(2081, 4, 15, &date);
nepdate_format(date, NULL, NEPDATE_ENGLISH, text, sizeof text);          /* 2081/04/15 */
nepdate_format(date, "MMMM d, yyyy", NEPDATE_ENGLISH, text, sizeof text); /* Shrawan 15, 2081 */
nepdate_format_short(date, NEPDATE_DMY, NEPDATE_SEP_DASH, true, NEPDATE_NEPALI, text,
                     sizeof text); /* १५-०४-२०८१ */
nepdate_format_long(date, long_options, NEPDATE_ENGLISH, text, sizeof text);
/* Tuesday, Shrawan 15, 2081 */
```

## Parsing

`nepdate_parse` is strict: three number groups in year, month and day order, separated by `-`, `/`,
`.`, `_`, backslash, space, `|` or the danda, with ASCII or Devanagari digits.
`nepdate_parse_lenient` also takes month names in any position, era markers and the words `गते` and
`मिति`. Both read at most the length you pass and need no terminator, and both return a
`nepdate_status` instead of throwing.

```c
nepdate_date date;
const char text[] = "2081 Shrawan 15";
if (nepdate_parse_lenient(text, sizeof text - 1, &date) == NEPDATE_OK) {
    /* 2081-04-15 */
}
if (nepdate_parse(text, sizeof text - 1, &date) != NEPDATE_OK) {
    /* NEPDATE_E_INVALID_CHARACTER: the strict parser has no month names */
}
```

## Ranges

A range is the inclusive span between two dates. You can measure it, test dates and other ranges
against it, and combine two ranges with intersection, union and difference. Two splits cut it at
month or fiscal-quarter boundaries, and three cursors walk every day, every nth day, or the working
or weekend days. A cursor is a small struct and a `next` function, so nothing allocates.

```c
nepdate_range april;
nepdate_range rest[2];
nepdate_date first;
nepdate_date last;
size_t rest_count = 0;
bool overlaps = false;
nepdate_range_of_month(2081, 4, &april); /* 2081-04-01 to 2081-04-32 */

nepdate_range holiday;
nepdate_from_bs(2081, 4, 13, &first);
nepdate_from_bs(2081, 4, 17, &last);
nepdate_range_make(first, last, &holiday);
nepdate_range_overlaps(april, holiday, &overlaps);       /* true */
nepdate_range_except(april, holiday, rest, &rest_count); /* 2 pieces, before and after */

nepdate_range_day_cursor cursor;
nepdate_date day;
nepdate_range_working_days(april, false, &cursor);
while (nepdate_range_day_next(&cursor, &day)) {
    /* every day of Shrawan 2081 except Saturdays */
}
```

`nepdate_range_format` writes `2081/04/01 - 2081/04/32` and `nepdate_range_format_short` the same
two ends in another order, separator or language. A range is never empty, so a reversed pair and a
disjoint intersection are errors, not empty values: the intersection reports
`NEPDATE_E_NO_OVERLAP`.

## Fiscal year

A fiscal year starts on 1 Shrawan and ends on the last day of Ashad of the next BS year, and it is
named by the BS year it starts in. `nepdate_fiscal_year_of` gives the year that contains a date,
`nepdate_fiscal_year_start` and `nepdate_fiscal_year_end` its two ends, and the quarter functions
the four quarters, where Q1 is Shrawan to Ashoj. `nepdate_fiscal_year_label` writes `2082/83`.

```c
nepdate_date date;
nepdate_date start;
nepdate_date end;
nepdate_date q2;
int fiscal_year;
char label[40];
nepdate_from_bs(2081, 1, 10, &date);
nepdate_fiscal_year_of(date, &fiscal_year);          /* 2080 */
nepdate_fiscal_year_start(fiscal_year, &start);      /* 2080-04-01 */
nepdate_fiscal_year_end(fiscal_year, &end);          /* 2081-03-32 */
nepdate_quarter_start(fiscal_year, NEPDATE_Q2, &q2); /* 2080-07-01 */
nepdate_fiscal_year_label(2082, NEPDATE_ENGLISH, label, sizeof label); /* "2082/83" */
```

## Errors

Every fallible call returns a `nepdate_status`: `NEPDATE_OK`, or the first thing that was wrong.
`nepdate_status_name` turns it into the enumerator's name for a log. On error the out object is left
unchanged, so a caller can keep using its previous value. A `nepdate_date` you built yourself is
rejected with `NEPDATE_E_INVALID_DATE`, so a forged or zero value cannot index a table.

```c
nepdate_date date;
const nepdate_status status = nepdate_from_bs(2081, 13, 1, &date);
if (status != NEPDATE_OK) {
    fprintf(stderr, "nepdate: %s\n", nepdate_status_name(status)); /* NEPDATE_E_INVALID_MONTH */
}
```

## Tithi, holidays and events

`nepdate-c-patro` is the second library and target, `nepdate-c::patro`. `nepdate_patro_info` gives
the tithi, the public-holiday flag and the event count of a day, `nepdate_patro_events` its event
names, and `nepdate_patro_coverage` the range the data covers, BS 2077-01-01 to 2084-08-20.
`nepdate_patro_public_holidays` lists every gazetted holiday. The core has none of this data.

## Packages

The CMake package installs the core and the patro target together. The vcpkg ports and the Conan
recipes install the core and the patro separately; the patro package is found with
`find_package(nepdate-c-patro CONFIG REQUIRED)`. The amalgamation is in `dist/`: `nepdate.c` with
`nepdate.h`, and `nepdate_patro.c` with `nepdate_patro.h`.

## Zig

`build.zig` and `build.zig.zon` are tested with Zig **0.17.0**, the stable release at the time. A
Zig project can add this repository as a path or fetch dependency and link the `nepdate-c` and
`nepdate-c-patro` static libraries; `tests/zig_consumer` is a working consumer. The consumer prints
`BS 2081-04-15` for AD 2024-07-30.

## More

| Topic | Where |
|---|---|
| Every public name, with signatures | [docs/API.md](https://github.com/sanamhub/nepdate-c/blob/main/docs/API.md) |
| Architecture decision records | [docs/adr](https://github.com/sanamhub/nepdate-c/blob/main/docs/adr/README.md) |
| What changed in each version | [CHANGELOG.md](https://github.com/sanamhub/nepdate-c/blob/main/CHANGELOG.md) |
| Measured B1 to B8, with Rust and C++ | [docs/benchmarks.md](https://github.com/sanamhub/nepdate-c/blob/main/docs/benchmarks.md) |
| The other C code for Bikram Sambat | [docs/competitors.md](https://github.com/sanamhub/nepdate-c/blob/main/docs/competitors.md) |
| Building, gates and pull requests | [CONTRIBUTING.md](https://github.com/sanamhub/nepdate-c/blob/main/CONTRIBUTING.md) |
| Reporting a vulnerability | [SECURITY.md](https://github.com/sanamhub/nepdate-c/blob/main/SECURITY.md) |
| Guides and examples | [nepdate.com/docs/c](https://nepdate.com/docs/c/) |
| A wrong date, holiday or tithi | the hub's [data error form](https://github.com/sanamhub/nepdate/issues/new/choose) |
| The other languages | [sanamhub/nepdate](https://github.com/sanamhub/nepdate#packages) |

## License

MIT, see [LICENSE](https://github.com/sanamhub/nepdate-c/blob/main/LICENSE). Calendar and patro
data from NepDate by Raju Prasai and contributors, MIT, see
[shared/data/NOTICE](https://github.com/sanamhub/nepdate-c/blob/main/shared/data/NOTICE). Not
affiliated with the Government of Nepal.
