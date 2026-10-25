# nepdate-c

Bikram Sambat (Nepali calendar) dates for C: conversion to and from the Gregorian calendar,
arithmetic, formatting and parsing, with no allocation and no dependencies. It is plain ISO C11,
so Zig and language extensions can also compile it directly.

**Status: in development. Nothing is released yet.** The repository holds the build, the quality
gates and a version function; the calendar code is not written yet. The C++, Rust, Go, TypeScript,
Python, Swift and Kotlin ports are released; see [nepdate.com](https://nepdate.com).

## Build

CMake 3.25 or newer, Ninja and a C11 compiler (GCC 13, Clang 18, MSVC 2022, `zig cc`, or newer):

```sh
cmake --workflow --preset dev
```

`cmake --install` gives a CMake package and a pkg-config file, both named `nepdate-c`:

```cmake
find_package(nepdate-c CONFIG REQUIRED)
target_link_libraries(app PRIVATE nepdate-c::nepdate)
```

```c
#include <nepdate.h>
#include <stdio.h>

int main(void) {
    printf("nepdate-c %s\n", nepdate_version());
    return 0;
}
```

## Documentation

- [docs/API.md](docs/API.md): the public surface.
- [docs/adr/](docs/adr/README.md): design decisions.
- [CONTRIBUTING.md](CONTRIBUTING.md): setup and the quality gates.

## License

MIT. See [LICENSE](LICENSE).
