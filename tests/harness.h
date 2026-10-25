/* A dependency-free test harness, so the tests build wherever the library does (ADR-0004).
 *
 *   TEST(adds_one_day) { CHECK_EQ_INT(add(1, 1), 2); }
 *
 *   int main(void) {
 *       RUN_TEST(adds_one_day);
 *       return harness_finish();
 *   }
 *
 * A failed check prints "file:line: check: details" to stderr and the test goes on, so one run
 * reports every failure. harness_finish() prints a summary and returns 1 if any check failed.
 * Each test program is one translation unit, so the counters below are per program. */
#ifndef NEPDATE_TESTS_HARNESS_H
#define NEPDATE_TESTS_HARNESS_H

#include <stdio.h>
#include <string.h>

static int harness_tests_run;
static int harness_tests_failed;
static int harness_checks_failed;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) harness_run(#name, test_##name)

#define CHECK(expr) harness_check((expr) ? 1 : 0, __FILE__, __LINE__, #expr)

/* Both sides are converted to long long, so compare values that fit in it. */
#define CHECK_EQ_INT(actual, expected)                                                             \
    harness_check_int((long long)(actual), (long long)(expected), __FILE__, __LINE__, #actual,    \
                      #expected)

/* NULL on either side is a failure unless both are NULL. */
#define CHECK_EQ_STR(actual, expected)                                                             \
    harness_check_str((actual), (expected), __FILE__, __LINE__, #actual, #expected)

static inline void harness_fail_location(const char *file, int line) {
    (void)fprintf(stderr, "%s:%d: ", file, line);
    harness_checks_failed++;
}

static inline void harness_check(int ok, const char *file, int line, const char *expr) {
    if (!ok) {
        harness_fail_location(file, line);
        (void)fprintf(stderr, "CHECK(%s) failed\n", expr);
    }
}

static inline void harness_check_int(long long actual, long long expected, const char *file,
                                     int line, const char *actual_expr,
                                     const char *expected_expr) {
    if (actual != expected) {
        harness_fail_location(file, line);
        (void)fprintf(stderr, "CHECK_EQ_INT(%s, %s) failed: %lld != %lld\n", actual_expr,
                      expected_expr, actual, expected);
    }
}

static inline void harness_check_str(const char *actual, const char *expected, const char *file,
                                     int line, const char *actual_expr,
                                     const char *expected_expr) {
    if (actual == NULL && expected == NULL) {
        return;
    }
    if (actual == NULL || expected == NULL || strcmp(actual, expected) != 0) {
        harness_fail_location(file, line);
        (void)fprintf(stderr, "CHECK_EQ_STR(%s, %s) failed: \"%s\" != \"%s\"\n", actual_expr,
                      expected_expr, actual == NULL ? "(null)" : actual,
                      expected == NULL ? "(null)" : expected);
    }
}

static inline void harness_run(const char *name, void (*test)(void)) {
    const int failed_before = harness_checks_failed;
    test();
    harness_tests_run++;
    if (harness_checks_failed != failed_before) {
        harness_tests_failed++;
        (void)fprintf(stderr, "FAILED %s\n", name);
    }
}

static inline int harness_finish(void) {
    (void)printf("%d tests, %d failed, %d failed checks\n", harness_tests_run,
                 harness_tests_failed, harness_checks_failed);
    return harness_tests_failed == 0 ? 0 : 1;
}

#endif /* NEPDATE_TESTS_HARNESS_H */
