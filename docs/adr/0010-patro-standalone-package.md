# ADR-0010: The patro has a standalone package for package managers

- Status: Proposed
- Date: 2026-10-29
- Deciders: Sanam Pakuwal
- Amends: ADR-0003

## Context

ADR-0003 gives the core and the patro one CMake package, `nepdate-c`, with the targets
`nepdate-c::nepdate` and `nepdate-c::patro`. A build from the repository root still does that. The
vcpkg and Conan ports are separate packages, though: `nepdate-c` and `nepdate-c-patro`. A package
manager installs each package into its own tree, so the patro package cannot append a target to the
core package's `nepdate-cConfig.cmake`.

## Decision

- The root build and `cmake --install` keep the one `nepdate-c` package with both targets.
- A standalone patro build, which is what the patro vcpkg and Conan packages use, installs
  `nepdate-c-patroConfig.cmake`, `nepdate-c-patroConfigVersion.cmake` and the target
  `nepdate-c::patro`. It finds the installed `nepdate-c` package and links its core.
- The library file, header, pkg-config name and CMake target name do not change. Only the package
  name a consumer passes to `find_package` differs when the patro was installed on its own.
- The side-by-side test installs this repository and `nepdate-cpp` into one prefix and builds one
  consumer of each.

## Consequences

- A consumer of the CMake package from the root uses `find_package(nepdate-c)` and links
  `nepdate-c::patro`; a consumer of the vcpkg or Conan patro package uses
  `find_package(nepdate-c-patro)` and links the same target.
- The two packages cannot collide: the headers are `nepdate.h` and `nepdate_patro.h`, the libraries
  are `nepdate-c` and `nepdate-c-patro`, and the CMake package directories are `nepdate-c` and
  `nepdate-c-patro`.
- The Conan patro recipe no longer names a component in `cpp_info.requires`; Conan 2 propagates the
  core package's headers and library through the graph.
