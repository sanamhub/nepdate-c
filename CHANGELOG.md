# Changelog

Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioning follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- `NEPDATE_MIN` and `NEPDATE_MAX`, the first and last supported dates, with `nepdate_min` and
  `nepdate_max` behind them.
- `nepdate_year_length`, the length of a BS year, 364 to 367. It replaces the C# `IsLeapYear`,
  which follows the Gregorian year and changes in the middle of a BS year.
- `nepdate_today`, which takes the instant and the UTC offset, and `nepdate_is_today`,
  `nepdate_is_yesterday` and `nepdate_is_tomorrow` beside it. The library still never reads the
  clock itself.
- The `YYYYMMDD` integer form to and from a date: `nepdate_from_ymd_int` and `nepdate_to_ymd_int`.
  The integer is read whole, so a value that does not fit the supported years is refused instead of
  being truncated.
- `nepdate_format_iso`, the interchange text `YYYY-MM-DD`, named so a caller does not have to know
  that the pattern `"s"` writes it.
- Ordered batch conversion with caller buffers: `nepdate_bulk_to_bs`, `nepdate_bulk_parse_to_ad`
  and `nepdate_bulk_to_ad`, with the `nepdate_text` value for the text input. Each call is all or
  nothing, so a failing input writes no result.
- The rest of the range surface: `nepdate_range_from_day_count`, the three
  `nepdate_range_current_*` factories, `nepdate_range_contains_range`, `nepdate_range_overlaps`,
  `nepdate_range_is_adjacent_to`, `nepdate_range_intersect`, `nepdate_range_union`,
  `nepdate_range_except`, `nepdate_range_split_by_month`,
  `nepdate_range_split_by_fiscal_quarter`, `nepdate_range_equal`, `nepdate_range_format`,
  `nepdate_range_format_short`, and the day cursors `nepdate_range_dates_with_interval`,
  `nepdate_range_working_days`, `nepdate_range_weekend_days` and `nepdate_range_day_next` with the
  `nepdate_range_day_cursor` value.
- `NEPDATE_E_NO_OVERLAP`, the result of intersecting two ranges that share no day.
- Tests against the new shared vectors: the year lengths, the day difference in both directions,
  the packed integer, the interchange string, the ordered batch rows, the formatting matrix and the
  five range files. The bit-pattern property test now covers every new call that takes a date.

### Changed

- `nepdate_date.value` is documented as reserved and not part of the API. The struct stays a 4-byte
  value passed by value, and no layout changed.
- The API document is grouped like the canonical inventory and ends with a table that maps each C#
  member to its C functions, so a reader can find an equivalent without knowing the port's names.

### Breaking changes

- A pattern that ends in a single `\` now writes `\`. A backslash escapes the next character, and
  one at the end of the pattern has nothing to escape, so it is the character itself; before, the
  trailing backslash was dropped.
  **Migration:** remove the trailing `\` when the old output without it was intended.

## [0.1.0-rc.1] - 2026-11-03

### Added

- `verify-published.yml`: the release workflow now checks the GitHub release the way a user gets
  it, downloading the source archive and the amalgamation and building a consumer from each. The
  same workflow runs weekly with the newest release and daily against the release metadata, and
  reports a failure as one issue labelled `published-check`.

### Fixed

- The CMake targets are now named `nepdate-c` and `nepdate-c-patro`, so a project that adds
  nepdate-c with `FetchContent` or `add_subdirectory` can have its own targets named `nepdate` or
  `patro`, and can fetch nepdate-cpp, whose core target is `nepdate`, into the same build. Link
  `nepdate-c::nepdate` and `nepdate-c::patro` as before; installed packages and library file names
  don't change.

## [0.1.0-beta.1] - 2026-10-29

### Added

- `nepdate_date`, a 4-byte BS date, with `nepdate_from_bs` and `nepdate_to_bs`. Validation follows
  the spec's order (year, month, day) and accepts any `int`.
- `nepdate_status` with the error kinds of every port, plus `NEPDATE_E_INVALID_DATE` and
  `NEPDATE_E_NULL`, and `nepdate_status_name`.
- `nepdate_from_ad` and `nepdate_to_ad` for the proleptic Gregorian calendar, to the day.
- Weekday, month, day of year, month length, the first and last day of a month, and
  `nepdate_compare`, with the `nepdate_month` and `nepdate_weekday` enums.
- Day, month and year arithmetic with clamp or spill (`nepdate_add_days`, `nepdate_add_months`,
  `nepdate_add_years`), signed day difference and the calendar breakdown (`nepdate_diff_days`,
  `nepdate_diff`).
- Fiscal years and quarters (`nepdate_fiscal_year_of`, `nepdate_fiscal_year_start`,
  `nepdate_fiscal_year_end`, `nepdate_quarter_of`, `nepdate_quarter_start`, `nepdate_quarter_end`)
  and their labels in English and Nepali, with `nepdate_quarter` and `nepdate_lang`.
- Inclusive date ranges: `nepdate_range`, construction for a pair of dates, a month, a BS year or a
  fiscal year, length, containment, and a day cursor (`nepdate_range_begin`,
  `nepdate_range_next`).
- Formatting into caller buffers with `snprintf` semantics: `nepdate_format` for custom patterns,
  `nepdate_format_short` for the six field orders and separators, and `nepdate_format_long` for the
  long form, in English or Nepali (`nepdate_order`, `nepdate_separator`, `nepdate_long_options`).
- Strict and lenient parsing from a pointer and length (`nepdate_parse`, `nepdate_parse_lenient`),
  with the parse error kinds already in `nepdate_status`.
- `nepdate_from_unix` and `nepdate_today_nepal`, which take the instant from the caller and know
  Nepal's move from UTC+05:30 to UTC+05:45 in 1986. The optional `nepdate_from_tm`/`nepdate_to_tm`
  bridge for `struct tm` is built unless `NEPDATE_C_WITH_TM=OFF`.
- The separate `nepdate-c-patro` library with `<nepdate_patro.h>`: tithi, public holidays and
  events for BS 2077-01-01..2084-08-20 through `nepdate_patro_info`, `nepdate_patro_coverage`,
  `nepdate_patro_public_holidays` and `nepdate_patro_events`. The core has none of its data.
