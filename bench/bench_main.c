/* B1 to B8 of BENCHMARK section 3 on the 64 dates of section 2. The answers of every operation
 * are compared with bench/golden.h before the first sample; a wrong answer ends the process, so a
 * broken build cannot publish a number. The timing loop reads QueryPerformanceCounter or
 * clock_gettime(CLOCK_MONOTONIC), warms up, and reports the median of at least 20 samples. */

#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 199309L
#endif

#include "bench.h"
#include "bench_competitors.h"

#include <nepdate.h>
#include <nepdate_patro.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#define BENCH_BARRIER() _ReadWriteBarrier()
#else
#define BENCH_BARRIER() __asm__ __volatile__("" ::: "memory")
#endif

#define BENCH_INPUTS 64U
#define BENCH_DEFAULT_RUNS 25U
#define BENCH_WARMUP 5U

static volatile uint64_t bench_sink;

static uint64_t mix(uint64_t hash, uint64_t value) {
    hash ^= value + UINT64_C(0x9e3779b97f4a7c15) + (hash << 6U) + (hash >> 2U);
    return hash;
}

static void fail(const char *what) {
    (void)fprintf(stderr, "bench: %s\n", what);
    abort();
}

static void consume(uint64_t value) {
    bench_sink ^= value;
    BENCH_BARRIER();
}

static uint64_t now_ns(void) {
#if defined(_WIN32)
    static LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    if (frequency.QuadPart == 0) {
        (void)QueryPerformanceFrequency(&frequency);
    }
    (void)QueryPerformanceCounter(&counter);
    return (uint64_t)((counter.QuadPart * 1000000000LL) / frequency.QuadPart);
#else
    struct timespec ts;
    (void)clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
#endif
}

static void pin_cpu(unsigned cpu) {
#if defined(_WIN32)
    const DWORD_PTR mask = ((DWORD_PTR)1U << cpu);
    (void)SetThreadAffinityMask(GetCurrentThread(), mask);
#else
    /* The POSIX runs use taskset -c 2; the command is recorded in docs/benchmarks.md. */
    (void)cpu;
#endif
}

static bool bytes_equal(const char *a, size_t a_length, const char *b, size_t b_length) {
    return a_length == b_length && (a_length == 0 || memcmp(a, b, a_length) == 0);
}

static bench_ymd date_fields(nepdate_date date) {
    nepdate_ymd ymd;
    bench_ymd result;
    if (nepdate_to_bs(date, &ymd) != NEPDATE_OK) {
        fail("nepdate_to_bs failed");
    }
    result.year = ymd.year;
    result.month = ymd.month;
    result.day = ymd.day;
    return result;
}

static bench_ymd ad_fields(nepdate_date date) {
    nepdate_ymd ymd;
    bench_ymd result;
    if (nepdate_to_ad(date, &ymd) != NEPDATE_OK) {
        fail("nepdate_to_ad failed");
    }
    result.year = ymd.year;
    result.month = ymd.month;
    result.day = ymd.day;
    return result;
}

static bool ymd_equal(bench_ymd a, bench_ymd b) {
    return a.year == b.year && a.month == b.month && a.day == b.day;
}

static void fail_date(const char *what, size_t index, bench_ymd got, bench_ymd want) {
    (void)fprintf(stderr, "bench: %s[%zu] is %d-%d-%d, want %d-%d-%d\n", what, index, got.year,
                  got.month, got.day, want.year, want.month, want.day);
    abort();
}

static nepdate_date bs_date(size_t index) {
    nepdate_date date;
    const bench_ymd in = bench_golden_data[index].bs;
    if (nepdate_from_bs(in.year, in.month, in.day, &date) != NEPDATE_OK) {
        fail("nepdate_from_bs failed on a golden input");
    }
    return date;
}

static char bench_text[64][16];
static size_t bench_text_length[64];

static void make_text(size_t index, char *buffer, size_t size) {
    const bench_ymd in = bench_golden_data[index].bs;
    const int written = snprintf(buffer, size, "%04d/%02d/%02d", in.year, in.month, in.day);
    if (written < 0 || (size_t)written >= size) {
        fail("snprintf failed");
    }
}

static void prepare_texts(void) {
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        make_text(i, bench_text[i], sizeof bench_text[i]);
        bench_text_length[i] = strlen(bench_text[i]);
    }
}

static const bench_ymd *golden_ad(size_t index) { return &bench_golden_data[index].ad; }

static void check_b1(void) {
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        const bench_ymd got = ad_fields(bs_date(i));
        if (!ymd_equal(got, *golden_ad(i))) {
            fail_date("B1", i, got, *golden_ad(i));
        }
    }
}

static void check_b2(void) {
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        const bench_ymd ad = bench_golden_data[i].ad;
        nepdate_date date;
        if (nepdate_from_ad(ad.year, ad.month, ad.day, &date) != NEPDATE_OK) {
            fail("nepdate_from_ad failed on a golden input");
        }
        const bench_ymd got = date_fields(date);
        if (!ymd_equal(got, bench_golden_data[i].bs)) {
            fail_date("B2", i, got, bench_golden_data[i].bs);
        }
    }
}

static void check_b3(void) {
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        nepdate_date result;
        if (nepdate_add_days(bs_date(i), 1000, &result) != NEPDATE_OK) {
            fail("nepdate_add_days failed");
        }
        const bench_ymd got = date_fields(result);
        if (!ymd_equal(got, bench_golden_data[i].add_1000_days)) {
            fail_date("B3", i, got, bench_golden_data[i].add_1000_days);
        }
    }
}

static void check_b4(void) {
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        nepdate_date result;
        if (nepdate_add_months(bs_date(i), 13, NEPDATE_CLAMP, &result) != NEPDATE_OK) {
            fail("nepdate_add_months failed");
        }
        const bench_ymd got = date_fields(result);
        if (!ymd_equal(got, bench_golden_data[i].add_13_months)) {
            fail_date("B4", i, got, bench_golden_data[i].add_13_months);
        }
    }
}

static size_t long_text(nepdate_date date, char *buffer, size_t size) {
    const nepdate_long_options options = {true, true, true};
    const size_t length = nepdate_format_long(date, options, NEPDATE_NEPALI, buffer, size);
    if (length >= size) {
        fail("nepdate_format_long buffer too small");
    }
    return length;
}

static void check_b5(void) {
    char buffer[128];
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        const size_t length = long_text(bs_date(i), buffer, sizeof buffer);
        const char *want = bench_golden_data[i].long_ne;
        if (!bytes_equal(buffer, length, want, strlen(want))) {
            (void)fprintf(stderr, "bench: B5[%zu] is \"%s\", want \"%s\"\n", i, buffer, want);
            abort();
        }
    }
}

static void check_b6(void) {
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        nepdate_date date;
        if (nepdate_parse(bench_text[i], bench_text_length[i], &date) != NEPDATE_OK) {
            fail("nepdate_parse failed on a golden input");
        }
        const bench_ymd got = date_fields(date);
        if (!ymd_equal(got, bench_golden_data[i].bs)) {
            fail_date("B6", i, got, bench_golden_data[i].bs);
        }
    }
}

static nepdate_date patro_date(size_t index) {
    static nepdate_date dates[64];
    static bool ready = false;
    if (!ready) {
        const nepdate_range coverage = nepdate_patro_coverage();
        const int32_t span = coverage.end.value - coverage.start.value;
        const int32_t step = span / (int32_t)(BENCH_INPUTS - 1U);
        for (size_t i = 0; i < BENCH_INPUTS; ++i) {
            if (nepdate_add_days(coverage.start, (int32_t)i * step, &dates[i]) != NEPDATE_OK) {
                fail("building the patro inputs failed");
            }
        }
        ready = true;
    }
    return dates[index];
}

static void consume_info(uint64_t *hash, const nepdate_patro_day_info *info) {
    *hash = mix(*hash, info->has_tithi ? UINT64_C(1) : UINT64_C(0));
    *hash = mix(*hash, info->is_public_holiday ? UINT64_C(1) : UINT64_C(0));
    *hash = mix(*hash, (uint64_t)info->event_count);
    if (info->has_tithi) {
        *hash = mix(*hash, (uint64_t)info->tithi.np_length);
        *hash = mix(*hash, (uint64_t)info->tithi.en_length);
        for (size_t i = 0; i < info->tithi.np_length; ++i) {
            *hash = mix(*hash, (unsigned char)info->tithi.np[i]);
        }
        for (size_t i = 0; i < info->tithi.en_length; ++i) {
            *hash = mix(*hash, (unsigned char)info->tithi.en[i]);
        }
    }
}

/* The timed loops consume the fields and the text lengths, not every byte; hashing 30 names per
 * day would measure the hash. The checks above still compare every byte with the golden vector. */
static void consume_info_fast(uint64_t *hash, const nepdate_patro_day_info *info) {
    *hash = mix(*hash, info->has_tithi ? UINT64_C(1) : UINT64_C(0));
    *hash = mix(*hash, info->is_public_holiday ? UINT64_C(1) : UINT64_C(0));
    *hash = mix(*hash, (uint64_t)info->event_count);
    if (info->has_tithi) {
        *hash = mix(*hash, (uint64_t)info->tithi.np_length);
        *hash = mix(*hash, (uint64_t)info->tithi.en_length);
        *hash = mix(*hash, (unsigned char)info->tithi.en[0]);
    }
}

static void consume_events(uint64_t *hash, nepdate_date date) {
    nepdate_patro_name events[16];
    const size_t count = nepdate_patro_events(date, events, 16);
    *hash = mix(*hash, (uint64_t)count);
    for (size_t i = 0; i < count && i < 16; ++i) {
        *hash = mix(*hash, (uint64_t)events[i].np_length);
        *hash = mix(*hash, (uint64_t)events[i].en_length);
        *hash = mix(*hash, (unsigned char)events[i].en[0]);
    }
}

static uint64_t b7_checksum(void) {
    uint64_t hash = UINT64_C(0);
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        nepdate_patro_day_info info;
        if (nepdate_patro_info(patro_date(i), &info) != NEPDATE_OK) {
            fail("nepdate_patro_info failed on a patro input");
        }
        consume_info(&hash, &info);
    }
    return hash;
}

static uint64_t b8_checksum(void) {
    uint64_t hash = UINT64_C(0);
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        const nepdate_date date = patro_date(i);
        nepdate_ymd fields;
        if (nepdate_to_bs(date, &fields) != NEPDATE_OK) {
            fail("nepdate_to_bs failed in the month grid");
        }
        nepdate_range month;
        nepdate_range_iterator it;
        if (nepdate_range_of_month(fields.year, fields.month, &month) != NEPDATE_OK ||
            nepdate_range_begin(month, &it) != NEPDATE_OK) {
            fail("nepdate_range_of_month failed");
        }
        nepdate_date day;
        while (nepdate_range_next(&it, &day)) {
            nepdate_patro_day_info info;
            if (nepdate_patro_info(day, &info) != NEPDATE_OK) {
                fail("nepdate_patro_info failed in the month grid");
            }
            hash = mix(hash, (uint64_t)(uint32_t)day.value);
            consume_info(&hash, &info);
        }
    }
    return hash;
}

static void check_b7(void) {
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        nepdate_patro_day_info info;
        if (nepdate_patro_info(patro_date(i), &info) != NEPDATE_OK) {
            fail("nepdate_patro_info failed");
        }
        const bench_golden *want = &bench_golden_data[i];
        if (info.has_tithi != want->has_tithi || info.is_public_holiday != want->holiday ||
            info.event_count != (size_t)want->events) {
            (void)fprintf(stderr, "bench: B7[%zu] disagrees with the golden info\n", i);
            abort();
        }
        if (want->has_tithi) {
            if (!bytes_equal(info.tithi.en, info.tithi.en_length, want->tithi_en,
                             want->tithi_en_length)) {
                (void)fprintf(stderr, "bench: B7[%zu] tithi disagrees with the golden vector\n", i);
                abort();
            }
        }
        nepdate_patro_name events[16];
        if (nepdate_patro_events(patro_date(i), events, 16) != (size_t)want->events) {
            (void)fprintf(stderr, "bench: B7[%zu] event count disagrees with the golden vector\n",
                          i);
            abort();
        }
    }
    if (b7_checksum() != bench_b7_checksum) {
        fail("B7 checksum disagrees with the golden vector");
    }
}

static void check_b8(void) {
    if (b8_checksum() != bench_b8_checksum) {
        fail("B8 checksum disagrees with the golden vector");
    }
}

static void check_golden(void) {
    check_b1();
    check_b2();
    check_b3();
    check_b4();
    check_b5();
    check_b6();
    check_b7();
    check_b8();
}

typedef void (*operation)(uint64_t *hash);

static void b1(uint64_t *hash) {
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        nepdate_date date;
        nepdate_ymd ad;
        if (nepdate_from_bs(bench_golden_data[i].bs.year, bench_golden_data[i].bs.month,
                            bench_golden_data[i].bs.day, &date) != NEPDATE_OK ||
            nepdate_to_ad(date, &ad) != NEPDATE_OK) {
            abort();
        }
        *hash = mix(*hash, (uint64_t)(uint32_t)ad.year);
        *hash = mix(*hash, (uint64_t)(uint32_t)ad.month);
        *hash = mix(*hash, (uint64_t)(uint32_t)ad.day);
    }
}

static void b2(uint64_t *hash) {
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        const bench_ymd ad = bench_golden_data[i].ad;
        nepdate_date date;
        nepdate_ymd bs;
        if (nepdate_from_ad(ad.year, ad.month, ad.day, &date) != NEPDATE_OK ||
            nepdate_to_bs(date, &bs) != NEPDATE_OK) {
            abort();
        }
        *hash = mix(*hash, (uint64_t)(uint32_t)bs.year);
        *hash = mix(*hash, (uint64_t)(uint32_t)bs.month);
        *hash = mix(*hash, (uint64_t)(uint32_t)bs.day);
    }
}

static void b3(uint64_t *hash) {
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        nepdate_date date;
        nepdate_date result;
        nepdate_ymd bs;
        if (nepdate_from_bs(bench_golden_data[i].bs.year, bench_golden_data[i].bs.month,
                            bench_golden_data[i].bs.day, &date) != NEPDATE_OK ||
            nepdate_add_days(date, 1000, &result) != NEPDATE_OK ||
            nepdate_to_bs(result, &bs) != NEPDATE_OK) {
            abort();
        }
        *hash = mix(*hash, (uint64_t)(uint32_t)bs.year);
        *hash = mix(*hash, (uint64_t)(uint32_t)bs.month);
        *hash = mix(*hash, (uint64_t)(uint32_t)bs.day);
    }
}

static void b4(uint64_t *hash) {
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        nepdate_date date;
        nepdate_date result;
        nepdate_ymd bs;
        if (nepdate_from_bs(bench_golden_data[i].bs.year, bench_golden_data[i].bs.month,
                            bench_golden_data[i].bs.day, &date) != NEPDATE_OK ||
            nepdate_add_months(date, 13, NEPDATE_CLAMP, &result) != NEPDATE_OK ||
            nepdate_to_bs(result, &bs) != NEPDATE_OK) {
            abort();
        }
        *hash = mix(*hash, (uint64_t)(uint32_t)bs.year);
        *hash = mix(*hash, (uint64_t)(uint32_t)bs.month);
        *hash = mix(*hash, (uint64_t)(uint32_t)bs.day);
    }
}

static void b5(uint64_t *hash) {
    char buffer[128];
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        const size_t length = long_text(bs_date(i), buffer, sizeof buffer);
        *hash = mix(*hash, (uint64_t)length);
        *hash = mix(*hash, (unsigned char)buffer[0]);
        *hash = mix(*hash, (unsigned char)buffer[length - 1U]);
    }
}

static void b6(uint64_t *hash) {
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        nepdate_date date;
        nepdate_ymd bs;
        if (nepdate_parse(bench_text[i], bench_text_length[i], &date) != NEPDATE_OK ||
            nepdate_to_bs(date, &bs) != NEPDATE_OK) {
            abort();
        }
        *hash = mix(*hash, (uint64_t)(uint32_t)bs.year);
        *hash = mix(*hash, (uint64_t)(uint32_t)bs.month);
        *hash = mix(*hash, (uint64_t)(uint32_t)bs.day);
    }
}

static void b7(uint64_t *hash) {
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        const nepdate_date date = patro_date(i);
        nepdate_patro_day_info info;
        if (nepdate_patro_info(date, &info) != NEPDATE_OK) {
            abort();
        }
        consume_info_fast(hash, &info);
        consume_events(hash, date);
    }
}

static void b8(uint64_t *hash) {
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        const nepdate_date date = patro_date(i);
        nepdate_ymd fields;
        if (nepdate_to_bs(date, &fields) != NEPDATE_OK) {
            abort();
        }
        nepdate_range month;
        nepdate_range_iterator it;
        if (nepdate_range_of_month(fields.year, fields.month, &month) != NEPDATE_OK ||
            nepdate_range_begin(month, &it) != NEPDATE_OK) {
            abort();
        }
        nepdate_date day;
        while (nepdate_range_next(&it, &day)) {
            nepdate_patro_day_info info;
            if (nepdate_patro_info(day, &info) != NEPDATE_OK) {
                abort();
            }
            *hash = mix(*hash, (uint64_t)(uint32_t)day.value);
            consume_info_fast(hash, &info);
            consume_events(hash, day);
        }
    }
}

static void run(operation op, uint64_t passes) {
    uint64_t hash = 0;
    for (uint64_t i = 0; i < passes; ++i) {
        op(&hash);
        BENCH_BARRIER();
    }
    consume(hash);
}

static void sort(uint64_t *values, size_t count) {
    for (size_t i = 1; i < count; ++i) {
        const uint64_t value = values[i];
        size_t j = i;
        while (j > 0 && values[j - 1] > value) {
            values[j] = values[j - 1];
            --j;
        }
        values[j] = value;
    }
}

static double median(uint64_t *values, size_t count) {
    sort(values, count);
    if ((count % 2U) != 0U) {
        return (double)values[count / 2U];
    }
    return ((double)values[count / 2U - 1U] + (double)values[count / 2U]) / 2.0;
}

static uint64_t passes_for(operation op) {
    const uint64_t start = now_ns();
    run(op, 1);
    const uint64_t elapsed = now_ns() - start;
    const double target = 2000000.0;
    double passes = target / (double)(elapsed == 0 ? 1 : elapsed);
    if (passes < 1.0) {
        passes = 1.0;
    }
    if (passes > 100000.0) {
        passes = 100000.0;
    }
    return (uint64_t)passes;
}

static double measure(operation op, unsigned runs) {
    const uint64_t passes = passes_for(op);
    for (unsigned i = 0; i < BENCH_WARMUP; ++i) {
        run(op, passes);
    }
    uint64_t *samples = (uint64_t *)calloc(runs, sizeof *samples);
    if (samples == NULL) {
        fail("calloc failed");
    }
    for (unsigned i = 0; i < runs; ++i) {
        const uint64_t start = now_ns();
        run(op, passes);
        samples[i] = now_ns() - start;
    }
    const double total = median(samples, runs);
    free(samples);
    return total / ((double)BENCH_INPUTS * (double)passes);
}

static double rival_ns(const bench_rival *rival, bool to_ad, unsigned runs) {
    const uint64_t passes = 10;
    uint64_t *samples = (uint64_t *)calloc(runs, sizeof *samples);
    if (samples == NULL) {
        fail("calloc failed");
    }
    for (unsigned i = 0; i < BENCH_WARMUP; ++i) {
        uint64_t hash = 0;
        for (size_t j = 0; j < BENCH_INPUTS; ++j) {
            int y = 0;
            int m = 0;
            int d = 0;
            const bench_ymd in = to_ad ? bench_golden_data[j].bs : bench_golden_data[j].ad;
            const bool ok = to_ad ? rival->to_ad(in.year, in.month, in.day, &y, &m, &d)
                                  : rival->to_bs(in.year, in.month, in.day, &y, &m, &d);
            hash = mix(hash, ok ? UINT64_C(1) : UINT64_C(0));
            hash = mix(hash, (uint64_t)(uint32_t)y);
            hash = mix(hash, (uint64_t)(uint32_t)m);
            hash = mix(hash, (uint64_t)(uint32_t)d);
        }
        consume(hash);
    }
    for (unsigned i = 0; i < runs; ++i) {
        const uint64_t start = now_ns();
        uint64_t hash = 0;
        for (uint64_t p = 0; p < passes; ++p) {
            for (size_t j = 0; j < BENCH_INPUTS; ++j) {
                int y = 0;
                int m = 0;
                int d = 0;
                const bench_ymd in = to_ad ? bench_golden_data[j].bs : bench_golden_data[j].ad;
                const bool ok = to_ad ? rival->to_ad(in.year, in.month, in.day, &y, &m, &d)
                                      : rival->to_bs(in.year, in.month, in.day, &y, &m, &d);
                hash = mix(hash, ok ? UINT64_C(1) : UINT64_C(0));
                hash = mix(hash, (uint64_t)(uint32_t)y);
                hash = mix(hash, (uint64_t)(uint32_t)m);
                hash = mix(hash, (uint64_t)(uint32_t)d);
            }
            BENCH_BARRIER();
        }
        consume(hash);
        samples[i] = now_ns() - start;
    }
    const double total = median(samples, runs);
    free(samples);
    return total / ((double)BENCH_INPUTS * (double)passes);
}

static void agrees(const bench_rival *rival) {
    unsigned b1 = 0;
    unsigned b1_no_answer = 0;
    unsigned b2 = 0;
    unsigned b2_no_answer = 0;
    for (size_t i = 0; i < BENCH_INPUTS; ++i) {
        int y = 0;
        int m = 0;
        int d = 0;
        const bench_ymd bs = bench_golden_data[i].bs;
        if (!rival->to_ad(bs.year, bs.month, bs.day, &y, &m, &d)) {
            ++b1_no_answer;
        } else if (y == bench_golden_data[i].ad.year && m == bench_golden_data[i].ad.month &&
                   d == bench_golden_data[i].ad.day) {
            ++b1;
        }
        const bench_ymd ad = bench_golden_data[i].ad;
        if (!rival->to_bs(ad.year, ad.month, ad.day, &y, &m, &d)) {
            ++b2_no_answer;
        } else if (y == bs.year && m == bs.month && d == bs.day) {
            ++b2;
        }
    }
    (void)printf("%s: B1 agrees %u/64 (no answer %u), B2 agrees %u/64 (no answer %u)\n",
                 rival->name, b1, b1_no_answer, b2, b2_no_answer);
}

static void json_string(FILE *file, const char *text) {
    (void)fputc('"', file);
    for (const unsigned char *p = (const unsigned char *)text; *p != 0; ++p) {
        if (*p == '"' || *p == '\\') {
            (void)fputc('\\', file);
            (void)fputc((int)*p, file);
        } else if (*p < 0x20U) {
            (void)fprintf(file, "\\u%04x", (unsigned)*p);
        } else {
            (void)fputc((int)*p, file);
        }
    }
    (void)fputc('"', file);
}

int main(int argc, char **argv) {
    const char *json_path = NULL;
    const char *machine = "Intel Core i5-12500H laptop";
    unsigned runs = BENCH_DEFAULT_RUNS;
    bool use_affinity = true;
    unsigned cpu = 2;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--json") == 0 && i + 1 < argc) {
            json_path = argv[++i];
        } else if (strcmp(argv[i], "--machine") == 0 && i + 1 < argc) {
            machine = argv[++i];
        } else if (strcmp(argv[i], "--runs") == 0 && i + 1 < argc) {
            runs = (unsigned)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--affinity") == 0 && i + 1 < argc) {
            cpu = (unsigned)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--no-affinity") == 0) {
            use_affinity = false;
        } else {
            (void)fprintf(stderr,
                          "usage: %s [--json PATH] [--machine TEXT] [--runs N] "
                          "[--affinity CPU|--no-affinity]\n",
                          argv[0]);
            return 2;
        }
    }
    if (runs < 20U) {
        (void)fprintf(stderr, "bench: at least 20 runs are required\n");
        return 2;
    }
    if (use_affinity) {
        pin_cpu(cpu);
    }

    prepare_texts();
    check_golden();
    (void)printf("bench: golden vectors agree\n");

    operation operations[] = {b1, b2, b3, b4, b5, b6, b7, b8};
    const char *ids[] = {"B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8"};
    double ns[8];
    for (size_t i = 0; i < 8; ++i) {
        ns[i] = measure(operations[i], runs);
        (void)printf("%s %.3f ns/op\n", ids[i], ns[i]);
    }

    double rival_b1[8];
    double rival_b2[8];
    unsigned rivals = (unsigned)bench_rival_count;
    if (rivals > 8U) {
        rivals = 8U;
    }
    for (unsigned i = 0; i < rivals; ++i) {
        agrees(&bench_rivals[i]);
        rival_b1[i] = rival_ns(&bench_rivals[i], true, runs);
        rival_b2[i] = rival_ns(&bench_rivals[i], false, runs);
        (void)printf("%s %.3f %.3f ns/op\n", bench_rivals[i].name, rival_b1[i], rival_b2[i]);
    }

    if (json_path != NULL) {
        FILE *file = fopen(json_path, "wb");
        if (file == NULL) {
            (void)fprintf(stderr, "bench: cannot write %s\n", json_path);
            return 1;
        }
        (void)fputs("{\n  \"id\": \"c\",\n  \"name\": \"C\",\n  \"machine\": ", file);
        json_string(file, machine);
        (void)fputs(",\n  \"ops\": [", file);
        for (size_t i = 0; i < 8; ++i) {
            (void)fprintf(file, "%s%.3f", i == 0 ? "" : ", ", ns[i]);
        }
        (void)fputs("],\n  \"competitors\": [", file);
        for (unsigned i = 0; i < rivals; ++i) {
            (void)fputs(i == 0 ? "\n    " : ",\n    ", file);
            (void)fputs("{\"name\": ", file);
            json_string(file, bench_rivals[i].name);
            (void)fprintf(file, ", \"b1\": %.3f, \"b2\": %.3f}", rival_b1[i], rival_b2[i]);
        }
        (void)fputs(rivals == 0 ? "],\n" : "\n  ],\n", file);
        (void)fputs("  \"source\": "
                    "\"https://github.com/sanamhub/nepdate-c/blob/main/docs/benchmarks.md\"\n}\n",
                    file);
        (void)fclose(file);
    }
    return 0;
}
