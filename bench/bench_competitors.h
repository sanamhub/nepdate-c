#ifndef NEPDATE_BENCH_COMPETITORS_H
#define NEPDATE_BENCH_COMPETITORS_H

#include <stdbool.h>

/* One competitor from docs/competitors.md. A function returns false when the competitor has no
 * answer for the input, so the agreement count can tell "wrong" from "outside its range". */
typedef struct bench_rival {
    const char *name;
    bool (*to_ad)(int year, int month, int day, int *ad_year, int *ad_month, int *ad_day);
    bool (*to_bs)(int year, int month, int day, int *bs_year, int *bs_month, int *bs_day);
} bench_rival;

extern const bench_rival bench_rivals[1];
extern const int bench_rival_count;

#endif /* NEPDATE_BENCH_COMPETITORS_H */
