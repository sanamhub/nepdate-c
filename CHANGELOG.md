# Changelog

Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioning follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- `nepdate_date`, a 4-byte BS date, with `nepdate_from_bs` and `nepdate_to_bs`. Validation follows
  the spec's order (year, month, day) and accepts any `int`.
- `nepdate_status` with the error kinds of every port, plus `NEPDATE_E_INVALID_DATE` and
  `NEPDATE_E_NULL`, and `nepdate_status_name`.
