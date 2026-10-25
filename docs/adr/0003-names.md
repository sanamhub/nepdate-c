# ADR-0003: Names that never collide with nepdate-cpp

- Status: Accepted
- Date: 2026-10-25
- Deciders: Sanam Pakuwal

## Context

The C++ port already owns the names `nepdate` and `nepdate-patro` in vcpkg, Conan and CMake, and the
header directory `<nepdate/...>`. A user who wants both ports, or a distribution that packages both,
must be able to install them into one prefix.

## Decision

| What | Core | Patro |
|---|---|---|
| vcpkg port, Conan recipe, pkg-config name | `nepdate-c` | `nepdate-c-patro` |
| CMake package | `nepdate-c` | `nepdate-c` (same package) |
| CMake target | `nepdate-c::nepdate` | `nepdate-c::patro` |
| Library file | `nepdate-c` (`libnepdate-c.so.0`, `nepdate-c.dll`) | `nepdate-c-patro` |
| Header | `<nepdate.h>` | `<nepdate_patro.h>` |

The headers sit directly under `include/`, so they never touch nepdate-cpp's `include/nepdate/`.

## Consequences

- `find_package(nepdate-c CONFIG REQUIRED)` and `find_package(nepdate CONFIG REQUIRED)` can sit in
  one project.
- Both ports installed into one prefix is a release test, not an assumption.
