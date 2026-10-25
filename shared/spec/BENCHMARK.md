# Benchmark protocol (every port, and the C# baseline)

Every port measures the same operations on the same inputs on the same machine, so the numbers can
be put side by side with C# NepDate, the speed reference this family is built on. Each port also
measures its own competitors (its `docs/competitors.md`) in the same harness.

## 1. Reference machine

All published numbers come from this machine, on AC power, with other applications closed:

| Item | Value |
|---|---|
| CPU | 12th Gen Intel Core i5-12500H (4 performance + 8 efficiency cores, 16 threads, base 3.1 GHz) |
| RAM | 31.7 GB |
| OS | Windows 11 Home 10.0.26300 |
| Power plan | Ultimate Performance |

The i5-12500H mixes performance and efficiency cores, and a benchmark that moves between them gives
noisy numbers. Run every benchmark pinned to one performance core: logical CPU 2 (affinity mask
`0x4`), for example `start /affinity 4 /wait <command>` from `cmd`, or the harness's own affinity
option. Record the exact command in the results file.

Ports that can't run on Windows natively (the Swift benchmark package needs Linux or macOS) run on
the CI machine instead and say so; their numbers are not put in the same column as the others.

## 2. Inputs

- **The 64 dates:** date number `i` = 0..63 is the BS date at serial `s0 + i * ((s1 - s0) / 63)`
  (integer division), where `s0` is the serial of BS 2000-01-01 and `s1` of 2090-12-30
  (ALGORITHM §2). Their AD equivalents are the inputs for AD to BS.
- The text inputs for parsing are those 64 dates formatted as `YYYY/MM/DD`.

## 3. Operations

Each operation runs over all 64 inputs in one benchmark iteration; report time per single
operation (iteration time / 64).

| ID | Operation | Unit |
|---|---|---|
| B1 | BS to AD: build the date from (y, m, d) and get the AD (y, m, d) | core |
| B2 | AD to BS: from AD (y, m, d) to the BS date | core |
| B3 | add 1000 days | core |
| B4 | add 13 months, Clamp | core |
| B5 | long format, Nepali, with weekday (`toLongString`/`long` equivalent) | core |
| B6 | strict parse of `YYYY/MM/DD` | core |
| B7 | patro info for the date (tithi, holiday flag, events) | patro |
| B8 | a month grid: patro info for all days of the month the date is in | patro |

Inputs for B7 and B8 are the 64 dates moved into the patro's coverage: date `i` becomes the date at
serial `c0 + i * ((c1 - c0) / 63)`, where `c0..c1` is the patro coverage (PATRO §2).

## 4. Measurement rules

- Release or optimised build, the language's normal settings, no special flags beyond what a user
  would set.
- Warm-up before measuring (JIT languages included); at least 10 measured iterations; report the
  mean and the standard deviation, or the harness's equivalent.
- The result of every operation is consumed (`black_box`, `Blackhole`, `DoNotOptimize`, returning
  it), so the compiler can't remove the work.
- **Allocations:** report bytes allocated per operation where the platform can measure it
  (BenchmarkDotNet `MemoryDiagnoser`, JMH `-prof gc`, Go `-benchmem`, a counting allocator in Rust
  and C++). Python and TypeScript report "not measured" unless a reliable method exists.
- **Agreement:** for every competitor, a column says whether its results for the 64 dates agree
  with `shared/data`. A fast wrong answer is listed, never ranked first.

## 5. Results

Each port writes `docs/benchmarks.md` with: the machine table above, the toolchain and library
versions, the exact command, and one table:

| ID | This port ns/op | bytes/op | Competitor A ns/op | agrees | ... | C# NepDate ns/op (from the hub) |
|---|---|---|---|---|---|---|

The C# column is the **.NET 10** column of the hub's `docs/benchmarks/dotnet.md`, measured
on the same machine with BenchmarkDotNet and the NepDate version pinned in `tools/dotnet-bench`.
Research on why C# NepDate is fast and where it allocates: `docs/research/dotnet-internals.md`.

## 6. Targets

| Port | Target for B1, B2, B7 |
|---|---|
| Rust, C++ | faster than C# NepDate, zero bytes allocated |
| Go, Swift, Kotlin | within 1.5× of C# NepDate, zero bytes allocated per operation where the language allows (Kotlin: the date object itself only) |
| TypeScript | within 3× of C# NepDate |
| Python | the fastest pure-Python option among its competitors |

A port that misses its target reports the numbers to the maintainer before release; it doesn't drop
features to get there.
