# ADR-0007: The no-libc gate allows linker symbols

- Status: Accepted
- Date: 2026-10-25
- Deciders: Sanam Pakuwal
- Amends: ADR-0004, gate 13

## Context

Gate 13 of ADR-0004 (`no-libc`) allows the core object file two undefined symbols, `memcpy` and
`memset`, which compilers emit for struct copies. Once the core indexes its calendar tables, MSVC
on x64 addresses them relative to `__ImageBase`, a symbol the linker defines for every image. It is
not a C library function, and no compiler option removes it.

## Decision

The `no-libc` gate allows `memcpy`, `memset` and `__ImageBase`. Any other undefined symbol in the
core object file still fails the gate.

## Consequences

- The gate keeps its purpose: the core calls nothing from the C library beyond the two functions
  every freestanding toolchain provides.
- ADR-0004 stays as written; its gate 13 row is read together with this ADR.
