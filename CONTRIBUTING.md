# Contributing

Bug reports, fixes and documentation improvements are welcome. For a new feature or an API change,
open an issue first so the design can be agreed before you write code.

This project follows the [Code of Conduct](CODE_OF_CONDUCT.md). Report security problems as
described in [SECURITY.md](SECURITY.md), not in a public issue.

## Scope

The shared calendar and patro data, the spec and the golden vectors live in the hub
([`sanamhub/nepdate`](https://github.com/sanamhub/nepdate)) and are copied into `shared/`
read-only. A wrong date, holiday or tithi is a data issue: report it in the hub, not here.

## Setup

You need CMake 3.25 or newer, Ninja and a C11 compiler: GCC 13, Clang 18, MSVC 2022 or `zig cc`,
or newer. Nothing else; the tests use the in-repo harness `tests/harness.h`.

```sh
cmake --workflow --preset dev
```

That configures, builds and tests the `dev` preset (Debug, static library, tests and the table
generator). The generated tables under `src/generated/` are committed; after a change to
`shared/data/` or `tools/codegen/`, regenerate them with `cmake --build --preset dev --target
codegen`.

## Gates

`cmake -P tools/ci.cmake` runs every gate in order and stops at the first failure; ADR-0004 lists
them. `-DONLY=format,tidy` runs a subset. A gate whose tool is missing is skipped with a warning;
CI sets `NEPDATE_C_CI=1`, which turns a skip into a failure.

| Gate | Needs |
|---|---|
| `format` | clang-format 18 |
| `tidy` | clang-tidy 18 |
| `cppcheck` | cppcheck |
| `analyzer`, `coverage` | GCC (and gcovr for `coverage`) |
| `sanitizers` | Clang or GCC with ASan and UBSan, or MSVC |
| `msan` | Clang on Linux |
| `valgrind` | Valgrind on Linux |
| `no-libc`, `api-surface` | `nm`, or `dumpbin` from a Visual Studio developer prompt |

On a machine where the default compiler is not the one you want, copy `CMakePresets.json` to the
gitignored `CMakeUserPresets.json` and add `local-*` presets that inherit the public ones; the
script uses them when it sees `local-dev`.

## Pull requests

- Keep a pull request to one change.
- Add or update a test for any behaviour change. The shared vector suites must keep passing.
- Public API changes go in `docs/API.md` in the same commit; the `api-surface` gate compares the
  two.
- Add a line to the `Unreleased` section of [CHANGELOG.md](CHANGELOG.md) for anything a user would
  notice.
- A significant design decision gets an ADR in `docs/adr/`. Accepted ADRs are not edited; a later
  ADR supersedes an earlier one.
- CI must pass.

## Commit messages

[Conventional Commits](https://www.conventionalcommits.org/): `type(scope): summary`, imperative,
lower case, no trailing period. Types: `feat`, `fix`, `docs`, `chore`, `refactor`, `test`, `build`,
`ci`, `perf`. The body says why, not what.
