# ADR-0008: BS to AD reads the AD month table

- Status: Accepted
- Date: 2026-10-25
- Deciders: Sanam Pakuwal
- Amends: ADR-0006

## Context

ADR-0006 chose the day-count layout and computed the Gregorian date of a BS day with Neri and
Schneider's integer algorithm. B1 is `nepdate_from_bs` followed by `nepdate_to_ad`, two exported
calls. In one measurement session it took 5.37 ns with MSVC and 4.61 ns with Clang, against 3.06 ns for
the Rust port on the same 64 dates. A split put 1.79 ns and 3.60 ns in the two MSVC calls, so the
Gregorian step was the larger part. Its disassembly has five multiplies; the existing AD month
table already holds the first day of every AD month from 1844-04 to 2143-04.

## Decision

- `tools/codegen` derives `NEPDATE_CAL_AD_ESTIMATE_MUL` and `SHIFT` for
  `nepdate_cal_ad_month_start`, the same way it derives the BS month estimate. It checks every day
  of the calendar: the estimate is never a later month than the day is in, and at most one month
  earlier.
- `nepdate_to_ad` multiplies the day count once, reads one AD month start and steps forward once if
  needed. It no longer calls `civil_from_days`.
- No public signature changes. The generated constants are an implementation detail.

## Measurements

Reference machine and method of ADR-0006: the 64 dates of BENCHMARK section 2, pinned to logical CPU
2, 20,000 passes per sample, the median of 101 samples after 20 warm-up samples. Nanoseconds per
operation, one session:

| Measurement | MSVC before | MSVC after | Clang before | Clang after | MSVC after, LTO |
|---|---:|---:|---:|---:|---:|
| `nepdate_from_bs` | 1.79 | 1.98 | 2.28 | 2.29 | 1.33 |
| `nepdate_to_ad` | 3.60 | 2.45 | 2.94 | 2.94 | 1.48 |
| B1, both calls | 5.37 | 4.15 | 4.61 | 4.39 | 2.65 |
| B2, `nepdate_from_ad` | 3.44 | 3.51 | 3.59 | 3.58 | 1.97 |

MSVC 19.51.36260 `/O2`; Clang 22.1.8 from zig 0.17.0, `-O2`. LTO is MSVC `/GL` and `/LTCG`. Rust
`cargo bench` in the same session reports 195.63 ns for 64 B1 calls (3.06 ns each) and 214.47 ns for
B2 (3.35 ns each). Its release profile uses `lto = "fat"` and one codegen unit.

The Rust benchmark compiles the call and the conversion in one image and the compiler inlines both
`NepaliDate::new` and `to_gregorian` into the loop. The C calls stay out of line because ADR-0006
measures them as a user calls the library. The same C code with LTO inlines them too: B1 is 2.65 ns,
13% faster than Rust, and B2 is 1.97 ns. Without LTO the remaining B1 gap is the cost of two calls
the compiler cannot inline, not the calendar arithmetic.

## Consequences

- B1 is within reach of Rust when the user builds with LTO. A C project that compares itself with
  Rust should use `/GL /LTCG` or `-flto`, as Rust does by default; the repository keeps the ordinary
  Release flags of ADR-0004.
- The generator proves the new estimate over all 109,212 days. A data change that breaks it fails
  codegen, not a test at run time.
- `nepdate_cal_ad_month_start` now has two users, `nepdate_from_ad` and `nepdate_to_ad`, so the AD
  table is checked in both directions.
