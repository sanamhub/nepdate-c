# ADR-0005: Packaging

- Status: Accepted
- Date: 2026-10-25
- Deciders: Sanam Pakuwal

## Context

C has no package manager every user shares. CMake users want `find_package`, autotools and Meson
users want pkg-config, and Zig, PHP extensions and embedded projects often want two files they can
drop into their tree.

## Decision

- **CMake config package** `nepdate-c` (`nepdate-cConfig.cmake` and a version file with
  `SameMinorVersion`), installed by `cmake --install`.
- **pkg-config files** `nepdate-c.pc` and `nepdate-c-patro.pc`, generated from `.pc.in` templates.
- **Amalgamation:** a script builds one `nepdate.c` and `nepdate.h` pair per library, and each
  release attaches them, so a project can add two files and nothing else.
- **vcpkg and Conan:** an overlay port and a recipe live in the repository and CI builds them.
  Submitting them to the official registries waits for six months of public history (the vcpkg
  maturity rule), so not before 2027-04.
- **Zig:** `build.zig` and `build.zig.zon` let a Zig project depend on the repository directly.
- **Meson:** a wrap file later, if users ask.

## Consequences

- Each way in needs a consumer build in CI before the release that ships it.
- The amalgamation is generated, so a fix always goes into `src/` first.
