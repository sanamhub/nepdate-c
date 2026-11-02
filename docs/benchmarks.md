# Benchmarks (C)

Time and allocation of the C library on operations B1 to B8 of
[`shared/spec/BENCHMARK.md`](../shared/spec/BENCHMARK.md), measured with the harness in
[`bench/`](../bench/). C has no mature competitor that passes [`docs/competitors.md`](competitors.md),
so this run reports this port only. The C# column is the .NET 10 column of the hub's
[`docs/benchmarks/dotnet.md`](https://github.com/sanamhub/nepdate/blob/main/docs/benchmarks/dotnet.md),
run 2026-11-02.

## Machine

| Item | Value |
|---|---|
| CPU | 12th Gen Intel Core i5-12500H (4 performance + 8 efficiency cores, 16 threads, base 3.1 GHz) |
| RAM | 31.7 GB |
| OS | Windows 11 Home 10.0.26300 |
| Power plan | Ultimate Performance |
| Affinity | logical CPU 2 (mask `0x4`), set by the harness's `--affinity 2` |
| Compiler | MSVC 19.51.36260, x64, Release `/O2 /DNDEBUG` |
| CPU load before | 12, 10 and 9 percent, sampled 2 s apart |
| CPU load after | 15, 11 and 18 percent, sampled 2 s apart |

The machine had the owner's normal desktop applications open. No other build, test suite or
benchmark ran during the run. Two earlier attempts were discarded: one started at 42, 36 and 28
percent load, and one was disturbed between B5 and B6. Their files are kept as
`raw/2026-11-02/bench-msvc-loaded.json` and `raw/2026-11-02/bench-msvc-second.json`.

## Command

From the repository root, in a `vcvars64` environment:

```
cmake -S . -B build\bench -G Ninja -DCMAKE_BUILD_TYPE=Release
      -DNEPDATE_C_BUILD_TESTS=OFF -DNEPDATE_C_BUILD_BENCH=ON
cmake --build build\bench --target nepdate_bench
build\bench\bin\nepdate_bench.exe --json docs\raw\2026-11-02\bench-msvc.json --affinity 2
```

The harness warms up, takes 25 samples per operation and reports the median, divided by 64 inputs.
The raw file is [`raw/2026-11-02/bench-msvc.json`](raw/2026-11-02/bench-msvc.json), and the console
log is [`raw/2026-11-02/bench-msvc.log`](raw/2026-11-02/bench-msvc.log).

## Results

Time is nanoseconds per single operation (the harness's median of 25 runs over the 64 inputs,
divided by 64). The harness reports no allocation for any operation.

| ID | Operation | nepdate ns/op | bytes/op | C# .NET 10 ns/op | C# bytes/op |
|---|---|---:|---:|---:|---:|
| B1 | BS to AD | 4.94 | 0 | 13.50 | 0 |
| B2 | AD to BS | 6.05 | 0 | 20.10 | 0 |
| B3 | add 1000 days | 7.47 | 0 | 19.05 | 0 |
| B4 | add 13 months, Clamp | 9.14 | 0 | 20.19 | 0 |
| B5 | long Nepali date with weekday | 40.76 | 0 | 150.41 | 435 |
| B6 | strict parse `YYYY/MM/DD` | 23.70 | 0 | 22.42 | 0 |
| B7 | patro info of a day | 16.55 | 0 | 48.21 | 51 |
| B8 | patro month grid | 577.10 | 0 | 3,112.76 | 1,429 |

B1, B2 and B7 are faster than C# NepDate, and every row allocates nothing. B6 is slightly slower
than C# at 23.70 ns against 22.42 ns. The C port has no BENCHMARK section 6 target of its own; the
C++ target is not applied to it.

## Notes

- The answers of every operation are compared with `bench/golden.h` before the first sample. A
  wrong answer ends the process, so a broken build cannot publish a number.
- The harness reads `QueryPerformanceCounter` on Windows and `clock_gettime(CLOCK_MONOTONIC)`
  elsewhere.
- A fast wrong answer is never ranked first. There is no competitor row this time.
