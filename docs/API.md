# API

The public surface of `nepdate-c`. Everything listed here is declared in `<nepdate.h>`; nothing
else is exported (ADR-0002). The API-surface gate compares the shared library's export table with
the functions on this page.

## Version

```c
#define NEPDATE_VERSION_MAJOR 0
#define NEPDATE_VERSION_MINOR 0
#define NEPDATE_VERSION_PATCH 0
#define NEPDATE_VERSION_PRERELEASE ""
#define NEPDATE_VERSION_STRING "0.0.0"

const char *nepdate_version(void);
```

The macros give the version of the header. `NEPDATE_VERSION_PRERELEASE` is the part after `-` in
a pre-release such as `0.1.0-beta.1`, and empty for a stable version.

`nepdate_version` returns the version of the compiled library as a static string, equal to
`NEPDATE_VERSION_STRING` when the header and the library come from the same release. It can't
fail.

## Linking

| Build | Define in the user's code |
|---|---|
| Shared library through CMake or pkg-config | nothing |
| Static library through CMake or pkg-config | nothing (the target and the `.pc` file add `NEPDATE_STATIC`) |
| Sources compiled into the user's project | `NEPDATE_STATIC` |
