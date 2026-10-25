/* nepdate-c: Bikram Sambat (Nepali calendar) dates for C. The public surface is docs/API.md. */
#ifndef NEPDATE_H
#define NEPDATE_H

#define NEPDATE_VERSION_MAJOR 0
#define NEPDATE_VERSION_MINOR 0
#define NEPDATE_VERSION_PATCH 0
/* The part after "-" in a pre-release version such as 0.1.0-beta.1, empty for a stable one. */
#define NEPDATE_VERSION_PRERELEASE ""
#define NEPDATE_VERSION_STRING "0.0.0"

/* Exported functions carry NEPDATE_API (ADR-0002). A Windows DLL needs dllexport while it is built
 * and dllimport in its users; the CMake target defines NEPDATE_STATIC for a static library, and a
 * project that compiles the sources itself defines it too. */
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(NEPDATE_STATIC)
#define NEPDATE_API
#elif defined(NEPDATE_BUILDING)
#define NEPDATE_API __declspec(dllexport)
#else
#define NEPDATE_API __declspec(dllimport)
#endif
#elif defined(__GNUC__)
#define NEPDATE_API __attribute__((visibility("default")))
#else
#define NEPDATE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the version of the compiled library as "MAJOR.MINOR.PATCH" or
 * "MAJOR.MINOR.PATCH-PRERELEASE", a static string. It equals NEPDATE_VERSION_STRING when the header
 * and the library come from the same release. */
NEPDATE_API const char *nepdate_version(void);

#ifdef __cplusplus
}
#endif

#endif /* NEPDATE_H */
