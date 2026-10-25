# Port plan: phases and acceptance criteria every port meets

Language-neutral. A new port works through these phases in order and adds only what is
language-specific. Names below are neutral; each port uses the names in its own `docs/API.md`.

## Phase 0: repository and CI

- Package manifest, licence (MIT), changelog with `[Unreleased]`, community files (SECURITY,
  CONTRIBUTING, issue and PR templates), Dependabot for CI actions.
- One CI command that runs every gate in order and ends with a one-line summary.
- Table generator skeleton reading `shared/data/calendar/bs-calendar.json`; output deterministic;
  `--check` mode fails on drift.
- CI calls the hub's reusable `shared-check`; weekly `shared-sync` with the port's regenerate command.
- AC: CI green on the first PR; the CI command fails on an unformatted file.

## C# test catalogue (every phase from 1 to 4)

`vectors/csharp-tests/catalog.tsv` lists every test case of C# NepDate's own test
suite (395 `[Fact]` and 29 `[Theory]` methods with 147 `[InlineData]` rows at `cb05cb65`), each
tagged with the phase that covers it, `n/a` (C#-only API such as `TypeConverter` or XML) or a
PARITY deviation. AC for each phase: every catalogue row tagged with that phase has a test in the
port, named after the row's `id`, or a comment naming the deviation. Ports add more where their
language allows: property tests, fuzzing, allocation tests, thread-safety tests, compile-time
checks.

The phase is the file's main subject, so a few rows need an API from a later phase: 39 phase-1 rows
of `NepaliDateConstructionTests`, `NepaliDatePropertiesTests` and `OptimizationVerificationTests`
build dates from strings (phase 3 parsing) or read `Today` (phase 4). Write those tests in phase 1
anyway: a row that expects a date or a validation error builds it with the constructor and a marker
naming the later phase; a row that needs the missing API is skipped with that marker (for example
`#[ignore = "phase 3: needs FromStr"]`, `t.Skip("phase 3: needs Parse")`). The later phase removes the
marker. The phase's AC counts these rows as covered; the later phase's AC includes removing every
marker that names it.

## Phase 1: core types and BS ↔ AD

- Generated tables (ALGORITHM §2): month lengths, month-start serials, year-start serials.
- Date type, month and weekday enums, error type with kinds (out of range, invalid month, invalid
  day, invalid Gregorian, parse kinds).
- Validation (ALGORITHM §3), serial conversion (§4), Gregorian via integer day arithmetic (§5),
  weekday (§7), day of year, month length, first and last day of month, comparison.
- AC: `month-boundaries.csv` (7176 rows) passes; all 109,212 serials round-trip; golden `dates.tsv`
  columns `ad`, `weekday`, `day_of_year`, `month_length` pass; 2081-04-32 valid, 2081-04-33 invalid;
  AD 1844-04-10 and 2143-04-16 out of range.

## Phase 2: arithmetic, diff, fiscal year, range

- Add days, add months and years with clamp and spill (§8), signed day difference, calendar
  breakdown (§8a), fiscal year and quarters (§8b), inclusive date range with length, contains and
  iteration.
- AC: golden `add-days.tsv` (9162 rows) and `add-months.tsv` (10180 rows, both columns); the §8 and
  §8a examples; golden `dates.tsv` fiscal and quarter columns; FY 2082 label in Nepali is
  `२०८२/८३`; every year's range length equals the sum of its month lengths; tests with negative
  amounts.

## Phase 3: formatting and parsing

- Names (FORMATTING §1), Devanagari digits, short, long and pattern formats, strict and lenient parse
  (PARSING), the canonical string forms (`YYYY/MM/DD` display, `YYYY-MM-DD` interchange).
- AC: golden `dates.tsv` text columns; `format-pattern.tsv` (451 rows) except D-04 rows; every row of
  `parse.tsv` with the right error kind; property or fuzz test: arbitrary input never crashes and only
  raises the port's parse error.

## Phase 4: patro package and platform interop

- The patro package `nepdate-patro` (ADR-0004), depending on the core package: tables per
  PATRO §3 (O(1) tithi index, sorted holiday and event serials, deduplicated names) and the queries
  `info`, `coverage` and `public holidays` (PATRO §2).
- Interop with the platform date type, and "today" with an explicit time zone defaulting to
  `Asia/Kathmandu`.
  If the clock is outside the supported range (before AD 1844-04-11 or after 2143-04-15), "today"
  returns the out-of-range error where the port's "today" can fail, and otherwise returns `MIN` or
  `MAX`, documented on the function.
- AC: T = 2190, H = 294, E = 1671; every PATRO §4 anchor; coverage 2077-01-01..2084-08-20; the core
  package's manifest has no dependency on `nepdate-patro`, and its built artefact doesn't contain
  the string `World Day Against Human Trafficking` (CI checks it; the artefact is the release binary
  of a small program that uses only the core for Rust, Go and Swift, the core jar for Kotlin, the
  core npm tarball for TypeScript and the core wheel for Python); the patro package and the core
  build and test on their own and are released together with the same version; with a clock at
  2024-07-30 18:30 UTC, today is BS 2081-04-15 in UTC and 2081-04-16 in Nepal.

## Phase 5: docs, benchmarks, release

- API docs on every public symbol; README quick start that is compiled or run in CI.
- Benchmarks per `BENCHMARK.md`: operations B1–B8 on the 64 dates, on the reference machine, against
  the port's `docs/competitors.md` and the C# NepDate baseline, with bytes allocated per operation
  and an "agrees with `shared/data`" column, published in `docs/benchmarks.md`.
- AC: every BENCHMARK §6 target met, or the miss reported to the maintainer with the numbers.
- Release workflow with preflight (version, tag and changelog agree), approval gate, notes from the
  changelog; the maintainer tags version 0.1.0.
