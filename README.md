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

> **Status:** in development, nothing is released. The build, the checks and CI are in place; the
> calendar code is being written. Until the first release, use one of the
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

```c
#include <nepdate.h>
#include <stdio.h>

int main(void) {
    printf("nepdate-c %s\n", nepdate_version());
    return 0;
}
```

## More

| Topic | Where |
|---|---|
| Every public name, with signatures | [docs/API.md](https://github.com/sanamhub/nepdate-c/blob/main/docs/API.md) |
| Architecture decision records | [docs/adr](https://github.com/sanamhub/nepdate-c/blob/main/docs/adr/README.md) |
| What changed in each version | [CHANGELOG.md](https://github.com/sanamhub/nepdate-c/blob/main/CHANGELOG.md) |
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
