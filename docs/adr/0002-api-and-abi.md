# ADR-0002: API and ABI conventions

- Status: Accepted
- Date: 2026-10-25
- Deciders: Sanam Pakuwal

## Context

A C library is called from C, from C++, and through FFI from other languages. Its ABI is what those
callers see, so the conventions have to hold for every input a caller can pass, including bytes that
were never a valid date.

## Decision

- **Prefixes:** `nepdate_` for functions and types, `NEPDATE_` for macros and enum constants,
  `nepdate_patro_` for the patro library.
- **`nepdate_date`** is a plain 4-byte value struct, passed by value. Every function that takes one
  is free of undefined behaviour for every bit pattern: it validates the value and returns
  `NEPDATE_E_INVALID_DATE` (or the fallback its documentation names) instead of indexing a table with
  it. A fuzz target proves it.
- **Layout is open.** Whether the struct holds `{year, month, day}` or a day serial is decided by
  measuring B1, B2 and B5 (`shared/spec/BENCHMARK.md`) both ways before the first conversion code
  merges. A later ADR records the choice and the numbers.
- **Errors:** a `nepdate_status` enum, 0 for success; results go through out-pointers. No `errno`,
  no locale, no `abort`, no `printf`.
- **Text output** goes to a caller's buffer with `snprintf` semantics: the return value is the length
  needed, and the buffer is always NUL-terminated when its size is not 0.
- **Symbols are hidden by default** (`-fvisibility=hidden`). Only declarations marked `NEPDATE_API`
  are exported: `__declspec(dllexport)` while building the DLL, `__declspec(dllimport)` when using it,
  nothing for a static library (`NEPDATE_STATIC`, set by the CMake target).
- **Shared library SONAME** `libnepdate-c.so.0` while the version is 0.x. Static and shared builds
  are both tested.
- **Thread-safe by construction:** there is no mutable state anywhere, so no locks and no
  initialisation call.

## Consequences

- Every function pays one range check on its date argument. The check is a compare and a branch the
  predictor learns, and it is what makes FFI callers safe.
- Out-pointers and status codes are verbose in C, but they map onto every FFI without allocation.
- Changing the exported list needs `docs/API.md` changed in the same commit (ADR-0004, API surface).
