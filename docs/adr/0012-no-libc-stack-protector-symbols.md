# ADR-0012: The no-libc gate allows the stack protector's symbols

- Status: Accepted
- Date: 2026-11-03
- Deciders: Sanam Pakuwal
- Amends: ADR-0007

## Context

GCC on Ubuntu, and other toolchains that turn on `-fstack-protector-strong` by default, add a
canary to every function that passes the address of a local to a call the optimizer does not
inline. A failed check calls `__stack_chk_fail`; toolchains without thread-local storage also read
the global `__stack_chk_guard`. Which functions get a canary depends on the optimization level, not
on the code: at `-Os` the 0.1 core already needed it in a dozen range functions, and at `-O3` the
batch parse needs it because `nepdate_parse` is not inlined into it.

The core calls nothing from the C library itself. These symbols come from a hardening option the
user's toolchain chose, and a freestanding build that has no C library also builds without the
option.

## Decision

The `no-libc` gate allows `__stack_chk_fail` and `__stack_chk_guard` beside the symbols of
ADR-0007. The core is not built with `-fno-stack-protector`, so a toolchain's hardening default
stays in force.

## Consequences

- The gate no longer fails on how the optimizer happens to inline a function, so code is not
  rewritten to avoid a canary.
- Any other undefined symbol in the core object file still fails the gate.
