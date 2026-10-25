# ADR-0004: Quality gates and CI

- Status: Accepted
- Date: 2026-10-25
- Deciders: Sanam Pakuwal

## Context

The library promises no undefined behaviour for any input, no allocation, no libc and an exact
exported surface. Each promise needs a check that fails a build, on every compiler a user has.

## Decision

### One command

`cmake -P tools/ci.cmake` runs every gate in order, prints each command, stops at the first failure
and ends with `ci: <passed>/<total> gates passed (<skipped> skipped)`. `-DONLY=<gate>,...` runs a
subset. A gate whose tool is missing is skipped with a warning locally and fails when the
environment sets `NEPDATE_C_CI=1`, as CI does.

| # | Gate | Check |
|---|---|---|
| 1 | `format` | clang-format 18 `--dry-run --Werror` on every C source outside `shared/`, build trees and `src/generated/` |
| 2 | `ascii` | every byte under `include/` and `src/` is 7-bit ASCII (ADR-0001) |
| 3 | `codegen` | build `tools/codegen` and run `--check` |
| 4 | `build-and-test` | presets `dev` (Debug, static), `release` (Release, static) and `shared` (Debug, shared): configure, build, `ctest` |
| 5 | `cxx-headers` | the public headers compiled as C++11 and C++20 with the C++ warning set |
| 6 | `tidy` | clang-tidy 18, `.clang-tidy`, warnings as errors |
| 7 | `cppcheck` | cppcheck on the library and tools, warnings as errors |
| 8 | `analyzer` | GCC `-fanalyzer` |
| 9 | `sanitizers` | ASan and UBSan, `-fno-sanitize-recover=all` (MSVC: ASan only) |
| 10 | `msan` | MemorySanitizer (Clang, Linux) |
| 11 | `valgrind` | every test under Valgrind memcheck, leaks are errors (Linux) |
| 12 | `coverage` | GCC `--coverage` and gcovr: 100 % line and branch coverage of `src/` |
| 13 | `no-libc` | the core object file's only undefined symbols are `memcpy` and `memset`, which compilers emit for struct copies |
| 14 | `api-surface` | the shared library's exported symbols equal the functions in `docs/API.md` |

The patro library adds an isolation gate: the built core library must not contain the bytes
`World Day Against Human Trafficking`, and the patro library must (hub ADR-0004).

### Warnings

GCC and Clang: `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wcast-qual
-Wstrict-prototypes -Wmissing-prototypes -Wvla -pedantic-errors -Werror`. MSVC:
`/W4 /WX /permissive-`. They are PRIVATE options of the library, tests and tools, so a user of the
library inherits none of them.

### Tests

A small in-repo harness, `tests/harness.h`: no third-party framework, so the tests build wherever
the library does. CTest runs each test executable.

### CI (GitHub Actions)

| Job | Runner | Runs |
|---|---|---|
| `msvc` | `windows-2022`, `windows-2025-vs2026` | gates 3, 4, 5, 9, 13, 14 |
| `gcc` | `ubuntu-24.04`, GCC 13 and 14 | gates 4, 13, 14 |
| `clang` | `ubuntu-24.04`, Clang 18 | gates 4, 13, 14 |
| `zig` | `ubuntu-24.04`, `zig cc` | gate 4 |
| `apple` | `macos-15` | gates 4, 9, 14 |
| `lint` | `ubuntu-24.04` | gates 1, 2, 3, 6, 7, 8 |
| `sanitizers` | `ubuntu-24.04`, Clang 18 | gates 9, 10 |
| `valgrind` | `ubuntu-24.04` | gate 11 |
| `coverage` | `ubuntu-24.04`, GCC 14 | gate 12 |
| `i686` | `ubuntu-24.04`, GCC `-m32` | the tests |
| `s390x` | `ubuntu-24.04`, cross GCC under QEMU (big-endian) | the tests |
| `cxx-headers` | `ubuntu-24.04` | gate 5 |
| `shared-check` | hub reusable workflow | `shared/` matches `shared/VERSION` |

`codeql.yml` runs CodeQL for C. Every `uses:` is pinned to a 40-character commit SHA with the version
in a comment, the hub's workflows too. `permissions: contents: read`; `concurrency` cancels
superseded runs; Dependabot updates the pins weekly.

## Consequences

- The same command runs locally and in CI; a local run says which gates it skipped.
- Gates 7, 8, 10 and 11 need Linux tools, so a Windows-only contributor relies on CI for them.
