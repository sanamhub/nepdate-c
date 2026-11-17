# ADR-0014: The tidy gate runs clang-tidy 22

- Status: Accepted
- Date: 2026-11-17
- Deciders: Sanam Pakuwal
- Amends: ADR-0004, gate 6 and the `lint` job

## Context

ADR-0004 made clang-tidy 18 the tidy gate. Clang 22 is the newest stable LLVM release and already
builds the code in the `clang`, `sanitizers` and `fuzz` jobs. Its clang-tidy finds more: missing
parentheses in mixed arithmetic, an index the static analyzer can't bound, file reads that go on
after a short read, and test code that indexes with a column it never found.

The C11 standard, the oldest supported compilers and the compiler matrix are not part of this
decision and do not change.

## Decision

- Gate 6 runs clang-tidy 22 with `.clang-tidy`, warnings as errors. `tools/ci.cmake` skips the gate
  locally, and fails it in CI, when plain `clang-tidy` is not version 22.
- A new `tidy` job on `ubuntu-26.04` runs gate 6 with `clang-tidy-22` and `clang-22` from the
  Ubuntu archive, the same way the `sanitizers` job gets Clang 22. The `lint` job keeps gates 1, 2,
  3, 7 and 8 with the tools it had.
- The new findings are fixed in the code. One more check is disabled:
  `readability-implicit-bool-conversion`. clang-tidy 22 reports the `int` that C11 gives `==`, `&&`
  and `!` as a conversion to `bool`, and a `bool` used as the condition of `?:` as a conversion to
  `int`. Its fixes, `(a == b) != 0` and `(int)flag`, make the code harder to read, and every finding
  in the code is one of these two shapes. `.clang-tidy` says why next to each disabled check.

## Consequences

- Contributors need clang-tidy 22 for the tidy gate; Ubuntu 26.04 and the LLVM apt repository have
  it.
- The fixes change no behaviour, public API or ABI. In the library they add parentheses that keep
  the existing order of operations, and `nepdate_from_ad` computes its table index as a signed
  `int`, which the analyzer can bound.
- clang-tidy no longer reports an implicit conversion of a plain integer or pointer to `bool`.
  Code review has to catch one.
