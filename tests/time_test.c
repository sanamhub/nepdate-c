/* Unix instants, Nepal's offset change in 1986, and the optional struct tm bridge. */
#include "generated/calendar_tables.h"
#include "harness.h"

#include <nepdate.h>

#include <stdint.h>
#include <string.h>

#if NEPDATE_C_WITH_TM
#include <limits.h>
#include <time.h>

static struct tm tm_sentinel(void) {
    struct tm value = {0};
    value.tm_sec = 1;
    value.tm_min = 2;
    value.tm_hour = 3;
    value.tm_mday = 4;
    value.tm_mon = 5;
    value.tm_year = 6;
    value.tm_wday = 7;
    value.tm_yday = 8;
    value.tm_isdst = 9;
    return value;
}

static bool tm_is_same(const struct tm *left, const struct tm *right) {
    return left->tm_sec == right->tm_sec && left->tm_min == right->tm_min &&
           left->tm_hour == right->tm_hour && left->tm_mday == right->tm_mday &&
           left->tm_mon == right->tm_mon && left->tm_year == right->tm_year &&
           left->tm_wday == right->tm_wday && left->tm_yday == right->tm_yday &&
           left->tm_isdst == right->tm_isdst;
}
#endif

/* 2024-07-30 18:30:00 UTC: the PATRO/PORT-PLAN anchor. */
static const int64_t anchor_instant = INT64_C(1722364200);
/* AD 1844-04-11 00:00:00 UTC, the first supported day. */
static const int64_t first_instant = INT64_C(-3967488000);
/* AD 2143-04-15 00:00:00 UTC, the last supported day. */
static const int64_t last_instant = INT64_C(5468342400);
/* 1985-12-31 18:30:00 UTC, when Nepal moved from +05:30 to +05:45. */
static const int64_t nepal_transition = INT64_C(504901800);

static nepdate_date bs(int year, int month, int day) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_bs(year, month, day, &date), NEPDATE_OK);
    return date;
}

static void expect(nepdate_date date, int year, int month, int day) {
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(date, &ymd), NEPDATE_OK);
    CHECK_EQ_INT(ymd.year, year);
    CHECK_EQ_INT(ymd.month, month);
    CHECK_EQ_INT(ymd.day, day);
}

TEST(unix_anchor_adds_offset) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_unix(anchor_instant, 0, &date), NEPDATE_OK);
    expect(date, 2081, 4, 15);
    CHECK_EQ_INT(nepdate_from_unix(anchor_instant, 345, &date), NEPDATE_OK);
    expect(date, 2081, 4, 16);
    CHECK_EQ_INT(nepdate_from_unix(anchor_instant, -345, &date), NEPDATE_OK);
    expect(date, 2081, 4, 15);
}

TEST(unix_floors_before_the_epoch) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_unix(first_instant, 0, &date), NEPDATE_OK);
    expect(date, 1901, 1, 1);
    CHECK_EQ_INT(nepdate_from_unix(first_instant + 86399, 0, &date), NEPDATE_OK);
    expect(date, 1901, 1, 1);
    CHECK_EQ_INT(nepdate_from_unix(first_instant - 1, 0, &date), NEPDATE_E_OUT_OF_RANGE);
}

TEST(unix_range_edges) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_unix(last_instant, 0, &date), NEPDATE_OK);
    expect(date, 2199, 12, 30);
    CHECK_EQ_INT(nepdate_from_unix(last_instant + 86399, 0, &date), NEPDATE_OK);
    expect(date, 2199, 12, 30);
    CHECK_EQ_INT(nepdate_from_unix(last_instant + 86400, 0, &date), NEPDATE_E_OUT_OF_RANGE);
}

TEST(unix_extremes_are_out_of_range) {
    nepdate_date date = {-7};
    CHECK_EQ_INT(nepdate_from_unix(INT64_MIN, 0, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_from_unix(INT64_MAX, 0, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_from_unix(anchor_instant, INT32_MIN, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_from_unix(anchor_instant, INT32_MAX, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(date.value, -7);
}

TEST(unix_null_out) { CHECK_EQ_INT(nepdate_from_unix(anchor_instant, 0, NULL), NEPDATE_E_NULL); }

TEST(today_nepal_uses_the_nepal_offset) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_today_nepal(anchor_instant, &date), NEPDATE_OK);
    expect(date, 2081, 4, 16);
    CHECK_EQ_INT(nepdate_today_nepal(anchor_instant, NULL), NEPDATE_E_NULL);
}

TEST(today_nepal_switches_offset_in_1986) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_today_nepal(nepal_transition - 1, &date), NEPDATE_OK);
    expect(date, 2042, 9, 16);
    CHECK_EQ_INT(nepdate_today_nepal(nepal_transition, &date), NEPDATE_OK);
    expect(date, 2042, 9, 17);
    CHECK_EQ_INT(date.value, bs(2042, 9, 17).value);
}

TEST(today_nepal_out_of_range) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_today_nepal(INT64_MIN, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_today_nepal(INT64_MAX, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_today_nepal(first_instant - 86400, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_today_nepal(last_instant + 86400, &date), NEPDATE_E_OUT_OF_RANGE);
}

#if NEPDATE_C_WITH_TM

TEST(tm_round_trip) {
    struct tm in;
    memset(&in, 0, sizeof in);
    in.tm_year = 2024 - 1900;
    in.tm_mon = 6;
    in.tm_mday = 30;
    in.tm_hour = 12;
    in.tm_isdst = 1;
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_tm(&in, &date), NEPDATE_OK);
    expect(date, 2081, 4, 15);

    struct tm out;
    memset(&out, 0xA5, sizeof out);
    CHECK_EQ_INT(nepdate_to_tm(date, &out), NEPDATE_OK);
    CHECK_EQ_INT(out.tm_year, 2024 - 1900);
    CHECK_EQ_INT(out.tm_mon, 6);
    CHECK_EQ_INT(out.tm_mday, 30);
    CHECK_EQ_INT(out.tm_hour, 0);
    CHECK_EQ_INT(out.tm_min, 0);
    CHECK_EQ_INT(out.tm_sec, 0);
    CHECK_EQ_INT(out.tm_wday, 2);
    CHECK_EQ_INT(out.tm_yday, 211);
    CHECK_EQ_INT(out.tm_isdst, -1);

    nepdate_date back = {0};
    CHECK_EQ_INT(nepdate_from_tm(&out, &back), NEPDATE_OK);
    CHECK_EQ_INT(back.value, date.value);

    struct tm january;
    memset(&january, 0, sizeof january);
    january.tm_year = 2023 - 1900;
    january.tm_mon = 0;
    january.tm_mday = 1;
    CHECK_EQ_INT(nepdate_from_tm(&january, &date), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_tm(date, &out), NEPDATE_OK);
    CHECK_EQ_INT(out.tm_yday, 0);

    struct tm march;
    memset(&march, 0, sizeof march);
    march.tm_year = 2023 - 1900;
    march.tm_mon = 2;
    march.tm_mday = 1;
    CHECK_EQ_INT(nepdate_from_tm(&march, &date), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_tm(date, &out), NEPDATE_OK);
    CHECK_EQ_INT(out.tm_yday, 59);
}

TEST(tm_errors_leave_the_date_alone) {
    struct tm in;
    memset(&in, 0, sizeof in);
    in.tm_year = 2024 - 1900;
    in.tm_mon = 6;
    in.tm_mday = 30;
    nepdate_date date = {-7};
    CHECK_EQ_INT(nepdate_from_tm(NULL, &date), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_from_tm(&in, NULL), NEPDATE_E_NULL);
    in.tm_year = INT_MAX;
    CHECK_EQ_INT(nepdate_from_tm(&in, &date), NEPDATE_E_OUT_OF_RANGE);
    in.tm_year = 2024 - 1900;
    in.tm_mon = -1;
    CHECK_EQ_INT(nepdate_from_tm(&in, &date), NEPDATE_E_INVALID_GREGORIAN);
    in.tm_mon = 12;
    CHECK_EQ_INT(nepdate_from_tm(&in, &date), NEPDATE_E_INVALID_GREGORIAN);
    in.tm_mon = 6;
    in.tm_mday = 0;
    CHECK_EQ_INT(nepdate_from_tm(&in, &date), NEPDATE_E_INVALID_GREGORIAN);
    in.tm_mday = 32;
    CHECK_EQ_INT(nepdate_from_tm(&in, &date), NEPDATE_E_INVALID_GREGORIAN);
    in.tm_year = 1844 - 1900;
    in.tm_mon = 3;
    in.tm_mday = 10;
    CHECK_EQ_INT(nepdate_from_tm(&in, &date), NEPDATE_E_OUT_OF_RANGE);
    in.tm_year = 2143 - 1900;
    in.tm_mon = 3;
    in.tm_mday = 16;
    CHECK_EQ_INT(nepdate_from_tm(&in, &date), NEPDATE_E_OUT_OF_RANGE);
    in.tm_year = INT_MIN;
    in.tm_mon = 0;
    in.tm_mday = 1;
    CHECK_EQ_INT(nepdate_from_tm(&in, &date), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(date.value, -7);
}

TEST(tm_matches_ad_for_every_date) {
    size_t failures = 0;
    for (uint32_t serial = 0; serial < (uint32_t)NEPDATE_CAL_DAYS; serial++) {
        const nepdate_date date = {(int32_t)serial + 1};
        nepdate_ymd ad = {0, 0, 0};
        struct tm out = {0};
        failures += nepdate_to_ad(date, &ad) == NEPDATE_OK ? 0U : 1U;
        failures += nepdate_to_tm(date, &out) == NEPDATE_OK ? 0U : 1U;
        failures += out.tm_year == ad.year - 1900 ? 0U : 1U;
        failures += out.tm_mon == ad.month - 1 ? 0U : 1U;
        failures += out.tm_mday == ad.day ? 0U : 1U;
        nepdate_date back = {0};
        failures +=
            nepdate_from_tm(&out, &back) == NEPDATE_OK && back.value == date.value ? 0U : 1U;
        nepdate_weekday weekday = NEPDATE_SUNDAY;
        failures += nepdate_weekday_of(date, &weekday) == NEPDATE_OK && (int)weekday == out.tm_wday
                        ? 0U
                        : 1U;
    }
    CHECK_EQ_INT(failures, 0);
}

TEST(tm_output_rejects_an_invalid_date) {
    struct tm out = tm_sentinel();
    const struct tm before = out;
    CHECK_EQ_INT(nepdate_to_tm((nepdate_date){-7}, &out), NEPDATE_E_INVALID_DATE);
    CHECK(tm_is_same(&out, &before));
    CHECK_EQ_INT(nepdate_to_tm(bs(2081, 4, 15), NULL), NEPDATE_E_NULL);
}

#endif

int main(void) {
    RUN_TEST(unix_anchor_adds_offset);
    RUN_TEST(unix_floors_before_the_epoch);
    RUN_TEST(unix_range_edges);
    RUN_TEST(unix_extremes_are_out_of_range);
    RUN_TEST(unix_null_out);
    RUN_TEST(today_nepal_uses_the_nepal_offset);
    RUN_TEST(today_nepal_switches_offset_in_1986);
    RUN_TEST(today_nepal_out_of_range);
#if NEPDATE_C_WITH_TM
    RUN_TEST(tm_round_trip);
    RUN_TEST(tm_errors_leave_the_date_alone);
    RUN_TEST(tm_matches_ad_for_every_date);
    RUN_TEST(tm_output_rejects_an_invalid_date);
#endif
    return harness_finish();
}
