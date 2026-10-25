# Architecture Decision Records (C port)

Decisions for this repository only. Decisions that bind every port are in the hub
(`sanamhub/nepdate`, `docs/adr/`), in particular hub ADR-0004 (patro in a separate package).
Immutable once Accepted; supersede with a new ADR.

| ADR | Title | Status |
|---|---|---|
| [0001](0001-language-and-toolchain.md) | Language, toolchain and generated tables | Accepted |
| [0002](0002-api-and-abi.md) | API and ABI conventions | Accepted |
| [0003](0003-names.md) | Names that never collide with nepdate-cpp | Accepted |
| [0004](0004-gates.md) | Quality gates and CI | Accepted, gate 13 amended by 0007 |
| [0005](0005-packaging.md) | Packaging | Accepted |
| [0006](0006-date-layout.md) | The date holds a day number | Accepted |
| [0007](0007-no-libc-linker-symbols.md) | The no-libc gate allows linker symbols | Accepted |
