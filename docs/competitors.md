# Other C code for Bikram Sambat

Searched 2026-10-29. Terms: `bikram sambat language:C`, `nepali calendar language:C`, `nepdate
language:C` and `bikram sambat` on GitHub's repository search; `nepdate`, `nepali` and `bikram` with
`vcpkg search`; `nepdate*`, `nepali*` and `bikram*` with `conan search -r conancenter`.

GitHub returns three repositories for the first three terms: this port, a Qt desktop widget
(`khumnath/nepdate`) and `aatishnn/nepdate`. The widget is an application, not a library, and its
conversion code needs Qt. `aatishnn/nepdate` is a small C library and is the only C competitor
compiled by `bench/`.

vcpkg and Conan Center return nothing for any term: no C Bikram Sambat library is published in
either registry.

## The one C competitor

`aatishnn/nepdate`, commit `18e5d3f9`, is fetched by `bench/CMakeLists.txt` into the build tree and
never vendored. Its GitHub metadata says GPL-2.0 and the source header says GPL-3.0 or later; it is
compiled only in the benchmark, never linked into a released library. It covers BS 2000 to 2090 and
converts AD to BS by walking days.

| Repository | Version | Licence | BS range | B1 agrees | B2 agrees |
|---|---|---:|---|---:|---:|
| [`aatishnn/nepdate`](https://github.com/aatishnn/nepdate) | `18e5d3f9`, 2015-03-18 | GPL | 2000 to 2090 | 62/64 | 60/64 (2 no answer) |

The 64 dates of BENCHMARK section 2 include years where its table differs from `shared/data`. A fast
wrong answer is listed, never ranked first. The benchmark also has a `shared/data` check for the
golden inputs, so an input the competitor answers differently is not hidden by the timing loop.

The three C++ competitors of the C++ port (`nepdate-cli`, `aatishnn/nepdate` and `nepalical`) are
not C libraries; the C++ port's own `docs/competitors.md` covers them. Rust's competitor crates are
covered by the Rust port.

## Not benchmarked

`khumnath/nepdate` is a Qt application. Its conversion code is a header class, but it is part of a
desktop program, not a C library a C project can depend on. No other C source tree found on GitHub
exposes a Bikram Sambat conversion API.
