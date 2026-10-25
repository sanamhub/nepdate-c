# ADR-0001: Language, toolchain and generated tables

- Status: Accepted
- Date: 2026-10-25
- Deciders: Sanam Pakuwal

## Context

The C port has two jobs. It is the family's reference native implementation, so it must match the
Rust port's speed with no undefined behaviour, and it is a core that other languages can embed:
Zig compiles C directly, and PHP, Python and Ruby extensions are written in C. Both jobs need the
most portable C a compiler accepts, not the newest.

## Decision

- **ISO C11 exactly:** `-std=c11 -pedantic-errors` on GCC and Clang, `/std:c11` on MSVC. No
  compiler extensions (`CMAKE_C_EXTENSIONS OFF`). No C23, not even behind `#if`: Clang and MSVC
  support it only in part, and a feature test in a header changes what each user compiles.
- **Compilers:** the code builds warning-free with GCC 13 and newer, Clang 18 and newer, MSVC 2022
  and 2026, and `zig cc`. ADR-0004 lists the warning set and the CI job for each.
- **Headers compile as C++ too:** `extern "C"` guards, and no designated initialiser or compound
  literal in a public header, because C++ before C++20 rejects both.
- **Tables are generated.** `tools/codegen/` is a C program, built only with
  `NEPDATE_C_BUILD_TOOLS=ON`. It reads `shared/data/` (the hub's data, copied read-only), checks it,
  and writes the headers under `src/generated/`. `--check` generates in memory, prints each file that
  differs from the disk, and exits 1 without writing. Output is deterministic: fixed formatting, LF
  line endings on every platform, no timestamps. Generated files are committed, so users and package
  managers never run the generator, and nothing reads data at run time.
- **Source files are ASCII.** Generated Devanagari is written as three-digit octal escapes
  (`"\340\244\270"`), never hex escapes, which swallow any hex digit that follows. The readable text
  goes in a comment beside it. Test sources may hold UTF-8.

## Consequences

- C11 gives `_Static_assert`, `<stdint.h>`, `<stdbool.h>` and anonymous unions, which is all the
  library needs. Users on C99-only compilers can't build it.
- The generator needs only the C compiler that builds the library, so contributors need no Python
  or JSON library.
- A data change in the hub reaches users only through a regenerated, committed header and a new
  release.
