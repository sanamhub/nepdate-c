#ifndef NEPDATE_BENCH_H
#define NEPDATE_BENCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct bench_ymd {
    int year;
    int month;
    int day;
} bench_ymd;

/* One of the 64 inputs of BENCHMARK section 2, with the answers every operation must reproduce
 * before a nanosecond is measured. The header is generated from the same data and checked against
 * the golden vector files; the benchmark refuses to run when an answer differs. */
typedef struct bench_golden {
    bench_ymd bs;
    bench_ymd ad;
    bench_ymd add_1000_days;
    bench_ymd add_13_months;
    const char *long_ne;
    bool has_tithi;
    bool holiday;
    unsigned events;
    const char *tithi_en;
    size_t tithi_en_length;
} bench_golden;

extern const bench_golden bench_golden_data[64];
extern const uint64_t bench_b7_checksum;
extern const uint64_t bench_b8_checksum;

#endif /* NEPDATE_BENCH_H */
