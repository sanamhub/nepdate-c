# ADR-0006: The date holds a day number

- Status: Accepted
- Date: 2026-10-25
- Deciders: Sanam Pakuwal

## Context

ADR-0002 left open whether `nepdate_date` stores `{int16_t year; uint8_t month; uint8_t day;}` or a
day count, to be settled by measuring B1 (BS to AD), B2 (AD to BS) and adding days both ways. A
`{year, month, day}` struct is easier to read in a debugger; a day count makes arithmetic one
addition but needs a lookup to get the month back.

Both layouts were built as a separate benchmark program, each with the fastest code found for it,
and the same validation the library does (every function checks its date argument). Neither
variant is kept in the repository.

| Layout | BS to AD | AD to BS | Add days | Read year, month, day |
|---|---|---|---|---|
| `{year, month, day}` | range check, then a table of the AD date of every BS month start plus at most one carry | AD month-start table, then an estimate of the month from the day count with one correction | day count, add, back to `{year, month, day}` | free (the struct fields) |
| day count | range check, then Gregorian date by Neri and Schneider's integer algorithm | AD month-start table, nothing else | one addition and a range check | the month estimate and one correction |

## Measurements

Reference machine (`shared/spec/BENCHMARK.md` section 1: Intel Core i5-12500H, Windows 11 Home
10.0.26300, Ultimate Performance), pinned to logical CPU 2 with `start /affinity 4`, CPU load 1 to
4 % before the runs. The 64 dates of BENCHMARK section 2; each sample runs 20,000 passes over
them; the figure is the median of 101 samples after 20 warm-up samples, and the median of three
runs. The functions sit in their own translation unit, so they are called, not inlined, as a
user of the library calls them. Nanoseconds per operation:

| Operation | `{y, m, d}` MSVC | day count MSVC | `{y, m, d}` Clang | day count Clang |
|---|---:|---:|---:|---:|
| B1 BS to AD | 4.39 | 4.73 | 4.79 | 3.95 |
| B2 AD to BS | 4.97 | 2.95 | 3.79 | 2.95 |
| B3 add 1000 days | 4.66 | 0.95 | 4.06 | 1.30 |
| read year, month and day | 1.85 | 2.54 | 2.62 | 2.82 |
| B1 + B2 + B3 | 14.02 | 8.63 | 12.64 | 8.20 |

MSVC 19.51.36260 `/O2`; Clang 22.1.8 from zig 0.17.0, `-O2`; both `-std=c11` / `/std:c11`, x64.

Two findings shaped the `{year, month, day}` numbers. Writing the three fields as three stores
made the next 4-byte read of the struct wait on a failed store-to-load forwarding: B1 took 9.4 ns
with MSVC and 9.8 ns with Clang until the fields were written as one 32-bit store. And B1 through
`days_from_civil` and Neri-Schneider instead of the AD-of-month-start table took 5.9 ns (MSVC) and
5.3 ns (Clang).

## Decision

- `nepdate_date` is `struct { int32_t value; }`, where `value` is 1 plus the number of days since
  BS 1901-01-01. Valid values are 1 to 109,212. The day count is faster overall with both
  compilers (8.6 against 14.0 ns on MSVC, 8.2 against 12.6 ns on Clang), and the only operation it
  loses, reading the fields, loses by 0.2 to 0.7 ns.
- The 1 is added so that a zero-initialised `nepdate_date` is not a date: every function returns
  `NEPDATE_E_INVALID_DATE` for it, as for any other value outside 1 to 109,212 (hub PARITY D-01).
  The offset folds into constants, so it costs nothing.
- Callers read the fields through `nepdate_to_bs`. `value` is documented as private.
- The tables are month lengths, month-start and year-start day counts (ALGORITHM section 2), plus
  the day count of the first day of every AD month from 1844-04 to 2143-04, which makes AD to BS
  one load.

## Consequences

- Validation of a date argument is one subtraction and one unsigned compare, for every bit
  pattern.
- Comparing two valid dates is comparing two integers.
- Formatting (B5) pays the month lookup once per date; it is 2 to 3 ns against a format that
  writes dozens of bytes.
