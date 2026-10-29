/* nepdate-c-patro: tithi, public holidays and events for the covered BS years. This library
 * depends on <nepdate.h>; the core never depends on it. The public surface is docs/API.md. */
#ifndef NEPDATE_PATRO_H
#define NEPDATE_PATRO_H

#include <nepdate.h>

#include <stdbool.h>
#include <stddef.h>

/* Exported patro functions carry NEPDATE_PATRO_API. A Windows DLL needs dllexport while it is
 * built and dllimport in its users; the CMake target defines NEPDATE_PATRO_STATIC for a static
 * library, and a project that compiles the sources itself defines it too. */
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(NEPDATE_PATRO_STATIC)
#define NEPDATE_PATRO_API
#elif defined(NEPDATE_PATRO_BUILDING)
#define NEPDATE_PATRO_API __declspec(dllexport)
#else
#define NEPDATE_PATRO_API __declspec(dllimport)
#endif
#elif defined(__GNUC__)
#define NEPDATE_PATRO_API __attribute__((visibility("default")))
#else
#define NEPDATE_PATRO_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* One name in both languages: np is UTF-8 Nepali, en is UTF-8 English, and neither is
 * NUL-terminated. A name with no value has both pointers NULL and both lengths 0. */
typedef struct nepdate_patro_name {
    const char *np;
    size_t np_length;
    const char *en;
    size_t en_length;
} nepdate_patro_name;

/* What the patro says about one day (PATRO section 2). A day outside the coverage has no tithi,
 * is not a public holiday and has no events. */
typedef struct nepdate_patro_day_info {
    bool has_tithi;
    nepdate_patro_name tithi;
    bool is_public_holiday;
    size_t event_count;
} nepdate_patro_day_info;

/* Stores what the patro says about date in *out. For a date this library made it returns
 * NEPDATE_OK; days outside 2077-01-01..2084-08-20 get the empty value. Returns NEPDATE_E_NULL when
 * out is NULL and NEPDATE_E_INVALID_DATE for a date no function made; *out is unchanged on error.
 */
NEPDATE_PATRO_API nepdate_status nepdate_patro_info(nepdate_date date, nepdate_patro_day_info *out);

/* Returns the smallest BS range containing every day with metadata, 2077-01-01..2084-08-20. It
 * can't fail. */
NEPDATE_PATRO_API nepdate_range nepdate_patro_coverage(void);

/* Writes the public holiday dates in ascending order and returns how many there are (294 today).
 * It writes at most capacity dates; a NULL dates or capacity 0 writes nothing and still returns
 * the count. */
NEPDATE_PATRO_API size_t nepdate_patro_public_holidays(nepdate_date *dates, size_t capacity);

/* Writes the events of date in file (display) order and returns how many there are. It writes at
 * most capacity names; a NULL events or capacity 0 writes nothing and still returns the count. A
 * date no function made, or a day outside the coverage, has no events and returns 0. */
NEPDATE_PATRO_API size_t nepdate_patro_events(nepdate_date date, nepdate_patro_name *events,
                                              size_t capacity);

#ifdef __cplusplus
}
#endif

#endif /* NEPDATE_PATRO_H */
