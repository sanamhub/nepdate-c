/* The patro queries of PATRO.md section 2: tithi, public holidays, events and coverage. */
#include "harness.h"
#include "summary.h"

#include "generated/patro_tables.h"

#include <nepdate.h>
#include <nepdate_patro.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

static bool name_is(nepdate_patro_name name, const char *np, const char *en) {
    return name.np != NULL && name.en != NULL && name.np_length == strlen(np) &&
           name.en_length == strlen(en) && memcmp(name.np, np, name.np_length) == 0 &&
           memcmp(name.en, en, name.en_length) == 0;
}

static bool name_is_empty(nepdate_patro_name name) {
    return name.np == NULL && name.en == NULL && name.np_length == 0 && name.en_length == 0;
}

static bool name_is_same(nepdate_patro_name left, nepdate_patro_name right) {
    return left.np == right.np && left.np_length == right.np_length && left.en == right.en &&
           left.en_length == right.en_length;
}

static bool info_is_same(nepdate_patro_day_info left, nepdate_patro_day_info right) {
    return left.has_tithi == right.has_tithi && name_is_same(left.tithi, right.tithi) &&
           left.is_public_holiday == right.is_public_holiday &&
           left.event_count == right.event_count;
}

TEST(info_anchor_2081_01_01) {
    nepdate_patro_day_info info;
    CHECK_EQ_INT(nepdate_patro_info(bs(2081, 1, 1), &info), NEPDATE_OK);
    CHECK(info.has_tithi);
    CHECK(name_is(info.tithi, "पञ्चमी", "Panchami"));
    CHECK(info.is_public_holiday);
    CHECK_EQ_INT(info.event_count, 3);

    nepdate_patro_name events[3];
    CHECK_EQ_INT(nepdate_patro_events(bs(2081, 1, 1), events, 3), 3);
    CHECK(name_is(events[0], "नयाँ वर्ष", "New Year"));
    CHECK(name_is(events[1], "मेष संक्रान्ति", "Mesh Sankranti"));
    CHECK(name_is(events[2], "बिस्का: जात्रा", "Bisket Jatra"));
}

TEST(info_anchor_2081_01_02) {
    nepdate_patro_day_info info;
    CHECK_EQ_INT(nepdate_patro_info(bs(2081, 1, 2), &info), NEPDATE_OK);
    CHECK(info.has_tithi);
    CHECK(name_is(info.tithi, "षष्ठी", "Shashthi"));
    CHECK(!info.is_public_holiday);
    CHECK_EQ_INT(info.event_count, 0);
    CHECK_EQ_INT(nepdate_patro_events(bs(2081, 1, 2), NULL, 0), 0);
}

TEST(info_anchor_2081_02_15) {
    nepdate_patro_day_info info;
    CHECK_EQ_INT(nepdate_patro_info(bs(2081, 2, 15), &info), NEPDATE_OK);
    CHECK(info.is_public_holiday);
    CHECK_EQ_INT(info.event_count, 1);
    nepdate_patro_name event;
    CHECK_EQ_INT(nepdate_patro_events(bs(2081, 2, 15), &event, 1), 1);
    CHECK(name_is(event, "गणतन्त्र दिवस", "Republic Day"));
}

TEST(info_anchor_2080_11_01_event_order) {
    const nepdate_date date = bs(2080, 11, 1);
    nepdate_patro_day_info info;
    CHECK_EQ_INT(nepdate_patro_info(date, &info), NEPDATE_OK);
    CHECK_EQ_INT(info.event_count, 5);
    nepdate_patro_name events[5];
    CHECK_EQ_INT(nepdate_patro_events(date, events, 5), 5);
    CHECK(name_is(events[0], "मङ्गल चौथी व्रत", "Mangala Chauthi Brata"));
    CHECK(name_is(events[1], "जनयुद्ध दिवस", "People's War Day"));
    CHECK(name_is(events[2], "तिलकुन्द चौथी", "Tilakund Chauthi"));
    CHECK(name_is(events[3], "कुम्भ संक्रान्ति", "Kumbha Sankranti"));
    CHECK(name_is(events[4], "विश्व रेडियो दिवस", "World Radio Day"));
}

TEST(info_anchor_2081_04_15) {
    const nepdate_date date = bs(2081, 4, 15);
    nepdate_patro_day_info info;
    CHECK_EQ_INT(nepdate_patro_info(date, &info), NEPDATE_OK);
    CHECK_EQ_INT(info.event_count, 3);
    nepdate_patro_name events[3];
    CHECK_EQ_INT(nepdate_patro_events(date, events, 3), 3);
    CHECK(name_is(events[0], "खीर खाने दिन", "Kheer Khane Din"));
    CHECK(name_is(events[1], "अन्तराष्ट्रिय मानव बेचबिखन विरुद्ध दिवस",
                  "World Day Against Human Trafficking"));
    CHECK(name_is(events[2], "अन्तर्राष्ट्रिय मित्रता दिवस", "International Friendship Day"));
}

TEST(info_outside_coverage_is_empty) {
    const nepdate_date dates[] = {bs(2070, 6, 15), bs(2090, 1, 1), bs(2084, 8, 21)};
    for (size_t i = 0; i < sizeof dates / sizeof dates[0]; i++) {
        nepdate_patro_day_info info;
        CHECK_EQ_INT(nepdate_patro_info(dates[i], &info), NEPDATE_OK);
        CHECK(!info.has_tithi);
        CHECK(name_is_empty(info.tithi));
        CHECK(!info.is_public_holiday);
        CHECK_EQ_INT(info.event_count, 0);
        CHECK_EQ_INT(nepdate_patro_events(dates[i], NULL, 0), 0);
    }
}

TEST(info_invalid_date_leaves_the_output_alone) {
    nepdate_patro_day_info info = {true, {"x", 1, "y", 1}, true, 7};
    const nepdate_patro_day_info before = info;
    CHECK_EQ_INT(nepdate_patro_info((nepdate_date){-1}, &info), NEPDATE_E_INVALID_DATE);
    CHECK(info_is_same(info, before));
    CHECK_EQ_INT(nepdate_patro_info(bs(2081, 1, 1), NULL), NEPDATE_E_NULL);
}

TEST(events_honour_capacity) {
    const nepdate_date date = bs(2081, 1, 1);
    nepdate_patro_name events[3] = {{"x", 1, "y", 1}, {"x", 1, "y", 1}, {"x", 1, "y", 1}};
    const nepdate_patro_name untouched = events[2];
    CHECK_EQ_INT(nepdate_patro_events(date, events, 2), 3);
    CHECK(name_is(events[0], "नयाँ वर्ष", "New Year"));
    CHECK(name_is(events[1], "मेष संक्रान्ति", "Mesh Sankranti"));
    CHECK(name_is_same(events[2], untouched));
    CHECK_EQ_INT(nepdate_patro_events(date, NULL, 3), 3);
    CHECK_EQ_INT(nepdate_patro_events(date, events, 0), 3);

    nepdate_patro_name one = {"x", 1, "y", 1};
    const nepdate_patro_name before = one;
    CHECK_EQ_INT(nepdate_patro_events((nepdate_date){-1}, &one, 1), 0);
    CHECK(name_is_same(one, before));
}

TEST(coverage_is_the_summary_range) {
    char first[11];
    char last[11];
    summary_day("first_day", first, sizeof first);
    summary_day("last_day", last, sizeof last);
    int fy = 0, fm = 0, fd = 0, ly = 0, lm = 0, ld = 0;
    CHECK_EQ_INT(sscanf(first, "%d-%d-%d", &fy, &fm, &fd), 3);
    CHECK_EQ_INT(sscanf(last, "%d-%d-%d", &ly, &lm, &ld), 3);
    const nepdate_range range = nepdate_patro_coverage();
    expect(range.start, fy, fm, fd);
    expect(range.end, ly, lm, ld);
}

TEST(public_holidays_are_ascending) {
    nepdate_date holidays[NEPDATE_PATRO_HOLIDAYS];
    const size_t count = (size_t)summary_number("holidays");
    CHECK_EQ_INT(count, (size_t)NEPDATE_PATRO_HOLIDAYS);
    CHECK_EQ_INT(nepdate_patro_public_holidays(holidays, count), count);
    int32_t previous = 0;
    for (size_t i = 0; i < count; i++) {
        CHECK(holidays[i].value > previous);
        previous = holidays[i].value;
        nepdate_patro_day_info info;
        CHECK_EQ_INT(nepdate_patro_info(holidays[i], &info), NEPDATE_OK);
        CHECK(info.is_public_holiday);
    }
    CHECK_EQ_INT(nepdate_patro_public_holidays(NULL, count), count);
    CHECK_EQ_INT(nepdate_patro_public_holidays(holidays, 0), count);
}

TEST(public_holidays_honour_capacity) {
    nepdate_date holidays[3];
    const size_t count = (size_t)summary_number("holidays");
    memset(holidays, 0xA5, sizeof holidays);
    CHECK_EQ_INT(nepdate_patro_public_holidays(holidays, 2), count);
    CHECK_EQ_INT(holidays[2].value, (int32_t)0xA5A5A5A5U);
}

int main(void) {
    RUN_TEST(info_anchor_2081_01_01);
    RUN_TEST(info_anchor_2081_01_02);
    RUN_TEST(info_anchor_2081_02_15);
    RUN_TEST(info_anchor_2080_11_01_event_order);
    RUN_TEST(info_anchor_2081_04_15);
    RUN_TEST(info_outside_coverage_is_empty);
    RUN_TEST(info_invalid_date_leaves_the_output_alone);
    RUN_TEST(events_honour_capacity);
    RUN_TEST(coverage_is_the_summary_range);
    RUN_TEST(public_holidays_are_ascending);
    RUN_TEST(public_holidays_honour_capacity);
    return harness_finish();
}
