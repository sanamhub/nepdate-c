# Benchmarks (C)

Time and allocation of the C library on operations B1 to B8 of
[`shared/spec/BENCHMARK.md`](../shared/spec/BENCHMARK.md), measured with the harness in
[`bench/`](../bench/). C has no mature competitor that passes [`docs/competitors.md`](competitors.md),
so this run reports this port only. The C# column is the .NET 10 column of the hub's
[`docs/benchmarks/dotnet.md`](https://github.com/sanamhub/nepdate/blob/main/docs/benchmarks/dotnet.md),
run 2026-11-16.

## Machine

| Item | Value |
|---|---|
| CPU | 12th Gen Intel Core i5-12500H (4 performance + 8 efficiency cores, 16 threads, base 3.1 GHz) |
| RAM | 31.7 GB |
| OS | Windows 11 Home 10.0.26300 |
| Power plan | Ultimate Performance |
| Affinity | logical CPU 2 (mask `0x4`), set by the harness's `--affinity 2` |
| Compiler | MSVC 19.52.37029 (toolset 14.52.37025), x64, Release `/O2 /DNDEBUG` |
| Idle gate before the run | 5.5 percent average, 14.9 peak (30 one-second samples) |
| Load during the run | one sample, 10.9 percent (the whole run lasts about one second) |

The machine was idle: no other build, test suite or benchmark ran during the run. Three more runs of
the same binary straight afterwards, kept in the log and not published, agree with the table to
within about 10 percent on every row (B1 4.96 to 5.54 ns, B6 11.76 to 11.85 ns, B8 526 to 530 ns).

## Command

From the repository root, in a `vcvars64` environment:

```
cmake -S . -B build\bench -G Ninja -DCMAKE_BUILD_TYPE=Release
      -DNEPDATE_C_BUILD_TESTS=OFF -DNEPDATE_C_BUILD_BENCH=ON
cmake --build build\bench --target nepdate_bench
build\bench\bin\nepdate_bench.exe --json docs\raw\2026-11-16\bench-msvc.json --affinity 2
```

The harness warms up, takes 25 samples per operation and reports the median, divided by 64 inputs.
The raw file is [`raw/2026-11-16/bench-msvc.json`](raw/2026-11-16/bench-msvc.json), and the console
log is [`raw/2026-11-16/bench-msvc.log`](raw/2026-11-16/bench-msvc.log).

## Results

Time is nanoseconds per single operation (the harness's median of 25 runs over the 64 inputs,
divided by 64). The harness reports no allocation for any operation.

| ID | Operation | nepdate ns/op | bytes/op | C# .NET 10 ns/op | C# bytes/op |
|---|---|---:|---:|---:|---:|
| B1 | BS to AD | 5.40 | 0 | 13.27 | 0 |
| B2 | AD to BS | 7.42 | 0 | 18.70 | 0 |
| B3 | add 1000 days | 8.29 | 0 | 20.06 | 0 |
| B4 | add 13 months, Clamp | 11.08 | 0 | 19.30 | 0 |
| B5 | long Nepali date with weekday | 32.75 | 0 | 139.04 | 435 |
| B6 | strict parse `YYYY/MM/DD` | 11.92 | 0 | 21.13 | 0 |
| B7 | patro info of a day | 16.57 | 0 | 48.63 | 51 |
| B8 | patro month grid | 540.18 | 0 | 3,274.50 | 1,429 |

Every row is faster than C# NepDate and allocates nothing. BENCHMARK section 6 gives C no fixed
target: the numbers are reported beside C# NepDate, as above.

Against the 2026-11-02 run, B6 is half the time (23.70 ns then) and B2 and B4 are about 20 percent
slower (6.05 and 9.14 ns then). The source did not change; the MSVC compiler did (19.51.36260 then,
19.52.37029 now), and the three repeat runs show the same shape.

## Notes

- The answers of every operation are compared with `bench/golden.h` before the first sample. A
  wrong answer ends the process, so a broken build cannot publish a number.
- The harness reads `QueryPerformanceCounter` on Windows and `clock_gettime(CLOCK_MONOTONIC)`
  elsewhere.
- A fast wrong answer is never ranked first. There is no competitor row this time.
