# Changelog

Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioning follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
