# ADR-0013: The patro requires exactly the same core version

- Status: Accepted
- Date: 2026-11-17
- Deciders: Sanam Pakuwal
- Amends: ADR-0010 (the standalone patro finds any installed `nepdate-c`)

## Context

`nepdate-c` and `nepdate-c-patro` are released together under one version, and the patro reads
internals of the core that are not part of the public API. ADR-0010 made the patro a standalone
package that runs `find_package(nepdate-c CONFIG REQUIRED)` with no version check, so it could build
against any installed core. The other SDKs pin their patro package to the exact core version.

## Decision

- The standalone build in `patro/CMakeLists.txt` stops with a `FATAL_ERROR` when the `X.Y.Z` part of
  the found core version differs from its own `PROJECT_VERSION`. It does not use `find_package(...
  EXACT)`, because Conan's generated config reports the full `1.0.0-rc.1` while `project()` holds
  only numbers. `nepdate-cpp` does the same.
- The Conan patro recipe requires `nepdate-c/<version>`, which Conan reads as an exact version, and
  the vcpkg patro port names the same version as its `version>=` floor, the only form vcpkg has.
- `scripts/version.sh`, which the CI `version` job runs, and `tools/release_check.cmake` fail when
  either of those two differs from the core version, pre-release part included.
- The `install_consumer` test builds the standalone patro against the installed core, and checks that
  a core reporting another version is refused.

## Consequences

- A version bump changes the patro recipe and port in the same commit as the core version. CI fails
  until it does.
- A patro cannot be built against a core of another release.
