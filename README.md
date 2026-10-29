<p align="center">
  <img src="https://raw.githubusercontent.com/sanamhub/nepdate-c/main/docs/assets/nepdate-c-128.png" width="128" alt="NepDate for C logo">
</p>

<h1 align="center">nepdate-c</h1>

<p align="center">
  <a href="https://github.com/sanamhub/nepdate-c/actions/workflows/ci.yml"><img src="https://github.com/sanamhub/nepdate-c/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <img src="https://img.shields.io/badge/C-11-283593?logo=c" alt="C11">
  <a href="https://github.com/sanamhub/nepdate-c/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue" alt="License: MIT"></a>
</p>

Bikram Sambat (Nepali calendar) for C. Part of the NepDate family: shared data and spec live in the
hub [`sanamhub/nepdate`](https://github.com/sanamhub/nepdate); the C# original is
[NepDate](https://github.com/RajuPrasai/NepDate).

Plain ISO C11 with no allocation, no global state and no dependencies, so it also builds into Zig
projects and into extensions for languages that call C.

> **Status:** `0.1.0-beta.1` is prepared for its tag after review. Until it is tagged, use one of the
> [released languages](https://github.com/sanamhub/nepdate#packages).

## Build

CMake 3.25 or newer, Ninja and a C11 compiler (GCC 13, Clang 18, MSVC 2022, `zig cc`, or newer):

```sh
cmake --workflow --preset dev
```

`cmake --install` gives a CMake package and a pkg-config file, both named `nepdate-c`. The patro
(tithi, holidays and events) is a second target in the same package:

```cmake
find_package(nepdate-c CONFIG REQUIRED)
target_link_libraries(app PRIVATE nepdate-c::nepdate)
# optional: target_link_libraries(app PRIVATE nepdate-c::patro)
```

## Quick start

```c
#include <nepdate.h>
#include <stdio.h>

int main(void) {
    nepdate_date date;
    nepdate_ymd bs;
    if (nepdate_from_ad(2024, 7, 30, &date) != NEPDATE_OK ||
        nepdate_to_bs(date, &bs) != NEPDATE_OK) {
        return 1;
    }
    printf("BS %04d-%02d-%02d\n", bs.year, bs.month, bs.day);
    return 0;
}
```

## Packages

The CMake package installs the core and the patro target together. The vcpkg ports and the Conan
recipes install the core and the patro separately; the patro package is found with
`find_package(nepdate-c-patro CONFIG REQUIRED)`. The amalgamation is in `dist/`: `nepdate.c` with
`nepdate.h`, and `nepdate_patro.c` with `nepdate_patro.h`.

## Zig

`build.zig` and `build.zig.zon` are tested with Zig **0.17.0**, the stable release at the time. A
Zig project can add this repository as a path or fetch dependency and link the `nepdate-c` and
`nepdate-c-patro` static libraries; `tests/zig_consumer` is a working consumer. The consumer prints
`BS 2081-04-15` for AD 2024-07-30.

## More

| Topic | Where |
|---|---|
| Every public name, with signatures | [docs/API.md](https://github.com/sanamhub/nepdate-c/blob/main/docs/API.md) |
| Architecture decision records | [docs/adr](https://github.com/sanamhub/nepdate-c/blob/main/docs/adr/README.md) |
| What changed in each version | [CHANGELOG.md](https://github.com/sanamhub/nepdate-c/blob/main/CHANGELOG.md) |
| Measured B1 to B8, with Rust and C++ | [docs/benchmarks.md](https://github.com/sanamhub/nepdate-c/blob/main/docs/benchmarks.md) |
| The other C code for Bikram Sambat | [docs/competitors.md](https://github.com/sanamhub/nepdate-c/blob/main/docs/competitors.md) |
| Building, gates and pull requests | [CONTRIBUTING.md](https://github.com/sanamhub/nepdate-c/blob/main/CONTRIBUTING.md) |
| Reporting a vulnerability | [SECURITY.md](https://github.com/sanamhub/nepdate-c/blob/main/SECURITY.md) |
| Guides and examples | [nepdate.com/docs/c](https://nepdate.com/docs/c/) |
| A wrong date, holiday or tithi | the hub's [data error form](https://github.com/sanamhub/nepdate/issues/new/choose) |
| The other languages | [sanamhub/nepdate](https://github.com/sanamhub/nepdate#packages) |

## License

MIT, see [LICENSE](https://github.com/sanamhub/nepdate-c/blob/main/LICENSE). Calendar and patro
data from NepDate by Raju Prasai and contributors, MIT, see
[shared/data/NOTICE](https://github.com/sanamhub/nepdate-c/blob/main/shared/data/NOTICE). Not
affiliated with the Government of Nepal.
