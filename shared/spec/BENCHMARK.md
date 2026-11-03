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

## 7. Choosing competitors

A port is compared only with Bikram Sambat libraries that people use in that language, and only
with libraries old and maintained enough to be a fair comparison. The rule is the same for every
language, including languages added after this section was written.

### Candidates

Search the language's main package registry and GitHub for every library that converts between
Bikram Sambat and Gregorian dates. The registries are nuget.org, crates.io, pkg.go.dev, npm, PyPI,
Swift Package Index, Maven Central, vcpkg and Conan. Leave out the NepDate family itself, and forks
or copies of another candidate.

### Usage

Usage ranks the candidates that pass the maturity bar. Record the number and the date it was read.
Each ecosystem exposes a different measure, so each has its own minimum. The minimum is a floor for
"people use this", not a target, and it is fixed before the candidate list is ranked.

| Ecosystem | Usage measure | Minimum |
|---|---|---|
| nuget.org | total downloads, and downloads in the last 6 weeks from the nuget.org statistics report | 10,000 total and 250 in the last 6 weeks |
| crates.io | recent downloads (90 days) | 1,000 |
| npm | weekly downloads, and dependents from deps.dev | 1,000 weekly and 5 dependents |
| PyPI | last month from pypistats | 1,000 |
| Go | "Imported by" on pkg.go.dev | 3 importing packages |
| Maven | dependents on deps.dev | 1 dependent |
| Swift | GitHub stars and dependents, and whether the package is in the Swift Package Index | 10 stars, or listed in the Swift Package Index |
| C, C++ | GitHub stars and dependents, and whether the library is in vcpkg or Conan | 25 stars, or a vcpkg or Conan port |

### Maturity bar

A candidate must pass all of these before usage is considered:

- first release at least 12 months ago;
- a release or a commit in the last 36 months;
- a license;
- not archived or marked deprecated;
- it builds and runs on the current toolchain;
- the minimum usage for its ecosystem in the table above.

### Selection

Take the top three passing candidates by usage. If fewer than three pass, list fewer. If none pass,
the language has no competitor row and the docs say "no mature library to compare". Never pad the
list with a library that fails the bar.

### Correctness stays separate

Agreement on the 64 dates is still measured and shown for every selected library. It never
qualifies or disqualifies a library from being compared. A fast library with wrong dates is listed
but is never ranked first, as section 4 already requires.

### Evidence

Each port keeps one table in `docs/competitors.md` with every candidate found, its usage, the bar it
fails if any, and whether it was selected. The table records the date the usage numbers were read
and links the registry or repository it came from.
