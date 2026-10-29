# Benchmarks: C, Rust, C++ and C# NepDate

B1 to B8 of [`shared/spec/BENCHMARK.md`](../shared/spec/BENCHMARK.md) on the 64 dates of section 2.
The C rows were measured for this report; the Rust and C++ rows were measured in the same session on
the same machine; the C# column is copied from the hub's .NET 10 run. The C competitor is the one
library found by [`docs/competitors.md`](competitors.md).

## Machine and state

| Item | Value |
|---|---|
| CPU | 12th Gen Intel Core i5-12500H (4 performance + 8 efficiency cores, 16 threads, 3.1 GHz) |
| RAM | 31.7 GB |
| OS | Windows 11 Home 10.0.26300 |
| Power plan | Ultimate Performance |
| Affinity | logical CPU 2 (mask `0x4`), `start /affinity 4 /wait` |
| State | not fully idle |

Browsers, editors, the Acer control panel and Task Manager were closed before the C runs and did not
hold the benchmark core. Windows services and the desktop session were still running, so the machine
was not quiet to the standard of BENCHMARK section 1. Every row was measured under that same
background load, in one session.

## Toolchains and versions

| Item | Value |
|---|---|
| C, MSVC | 19.51.36260, x64, Release `/O2 /DNDEBUG` |
| C, clang-cl | 23.1.4, x64, Release `/O2 /DNDEBUG` |
| Rust | rustc 1.99.0, `lto = "fat"`, one codegen unit |
| C++ | MSVC 19.51.36260, x64, Release `/O2 /DNDEBUG` |
| C++ port | `main` at `2520cff` |
| Rust port | `3936a58` |
| C benchmark | `nepdate_bench`, 25 samples, median; the amalgamation row includes the library sources in the harness's translation unit |
| C++ benchmark | Google Benchmark 1.9.5, 10 repetitions, aggregate mean CPU time |
| Rust benchmark | Criterion 0.8.2, 100 samples, mean |
| C# baseline | NepDate 2.0.7 on .NET 10, hub `docs/benchmarks/dotnet.md` |

The C harness checks every operation against `bench/golden.h` before timing and calls `abort()` on
a wrong answer. It times B1 to B8 with `QueryPerformanceCounter`, a compiler barrier, five warm-up
passes and the median of 25 samples.

## C results

`library` is a normal out-of-line call into `libnepdate-c`; `amalgamation` compiles
`dist/nepdate.c` and `dist/nepdate_patro.c` into the harness, the inlined case. Nanoseconds per
operation.

| ID | MSVC library | MSVC amalgamation | clang-cl library | clang-cl amalgamation |
|---|---:|---:|---:|---:|
| B1 | 4.871 | 4.673 | 5.297 | 4.405 |
| B2 | 6.020 | 5.857 | 6.610 | 5.354 |
| B3 | 7.565 | 5.252 | 5.923 | 4.546 |
| B4 | 9.359 | 7.937 | 8.422 | 6.434 |
| B5 | 42.115 | 33.709 | 38.918 | 38.674 |
| B6 | 24.497 | 22.769 | 15.961 | 15.467 |
| B7 | 15.625 | 15.516 | 13.207 | 12.634 |
| B8 | 534.410 | 503.159 | 421.684 | 360.170 |

Bytes allocated per operation is 0 for every C row: the library makes no allocation call, and the
harness does not count its own setup. The patro rows call both `nepdate_patro_info` and
`nepdate_patro_events`, because B7 and B8 ask for the tithi, the holiday flag and the events.

## Other ports

| ID | Rust ns/op | C++ ns/op | C# .NET 10 ns/op |
|---|---:|---:|---:|
| B1 | 3.12 | 6.42 | 13.29 |
| B2 | 3.43 | 7.13 | 18.64 |
| B3 | 3.32 | 6.63 | 17.50 |
| B4 | 2.18 | 5.00 | 18.85 |
| B5 | 18.48 | 23.92 | 153.02 |
| B6 | 14.35 | 11.05 | 21.55 |
| B7 | 3.38 | 11.88 | 47.93 |
| B8 | 209.19 | 457.65 | 3,132.66 |

Rust and C++ allocate 0 bytes per operation. The C# column is the hub's .NET 10 measurement, not a
run on this machine in this session.

## C competitor

The only C library of `docs/competitors.md`, `aatishnn/nepdate` at `18e5d3f9`, has no month
arithmetic, formatting, parsing or patro data, so only B1 and B2 apply. Its table disagrees with
`shared/data` on some of the 64 dates: B1 is 62/64 and B2 is 60/64 with 2 inputs outside its
1944 to 2033 AD range.

| Compiler | B1 ns/op | B2 ns/op |
|---|---:|---:|
| MSVC | 26,801.562 | 23,111.562 |
| clang-cl | 17,797.188 | 64,213.438 |

## Is C faster than Rust?

No. Comparing the inlined C build (the form a project gets by compiling the amalgamation) with
Rust, per operation:

| ID | C MSVC amalgamation | Rust | C / Rust |
|---|---:|---:|---:|
| B1 | 4.673 | 3.12 | 1.50 |
| B2 | 5.857 | 3.43 | 1.71 |
| B3 | 5.252 | 3.32 | 1.58 |
| B4 | 7.937 | 2.18 | 3.64 |
| B5 | 33.709 | 18.48 | 1.82 |
| B6 | 22.769 | 14.35 | 1.59 |
| B7 | 15.516 | 3.38 | 4.59 |
| B8 | 503.159 | 209.19 | 2.41 |

C is slower than Rust on all eight operations in this session. The out-of-line numbers are slower
still on B2 to B8, so the comparison does not depend on the call boundary. This measurement gives
no reason for another port to switch to the C core for speed.

The C port does beat C# NepDate on B1, B2 and B7, which are the BENCHMARK section 6 target
operations: 4.87 against 13.29, 6.02 against 18.64 and 15.63 against 47.93 (MSVC library). Rust and
C++ also beat those targets on all three. The roadmap's separate goal for this port, B1 and B2
within noise of Rust, is missed by 1.50x and 1.71x even in the inlined build. The numbers are
reported as measured; the benchmark was not changed to close the gap.

## Commands

MSVC C, from a Visual Studio x64 environment:

```
cmake -S . -B build/bench-msvc -G Ninja -DCMAKE_BUILD_TYPE=Release
      -DCMAKE_C_COMPILER=cl -DNEPDATE_C_BUILD_TESTS=OFF -DNEPDATE_C_BUILD_TOOLS=OFF
      -DNEPDATE_C_BUILD_BENCH=ON
cmake --build build/bench-msvc -j 4
start /affinity 4 /wait build\bench-msvc\bin\nepdate_bench.exe --runs 25
      --machine "Intel Core i5-12500H laptop, Windows 11 10.0.26300"
      --json bench\results\i5-12500h-msvc.json
```

The clang-cl rows use `-DCMAKE_C_COMPILER=clang-cl` and `build/bench-clang-cl`; the amalgamation
rows use `nepdate_bench_inlined.exe` in the same build tree.

C++:

```
cmake -S bench -B build/bench-msvc -G Ninja -DCMAKE_BUILD_TYPE=Release
      -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake
      -DVCPKG_MANIFEST_DIR=. -DVCPKG_MANIFEST_FEATURES=dev
cmake --build build/bench-msvc -j 4
start /affinity 4 /wait build\bench-msvc\nepdate_bench.exe --benchmark_repetitions=10
      --benchmark_report_aggregates_only=true --benchmark_out=bench.json
      --benchmark_out_format=json --benchmark_time_unit=ns
```

Rust, in a worktree of the Rust repository:

```
cargo bench -p nepdate-bench --bench compare
```

The C++ and Rust figures are per operation; both harnesses time all 64 inputs per iteration. The
C++ figure is the aggregate mean CPU time divided by 64, the Rust figure the Criterion mean divided
by 64.
