/* The phase-4 rows of the C# test catalogue (shared/spec/vectors/csharp-tests/catalog.tsv).
 * The CalendarDataTests rows check this port's dense tables (PATRO section 3) and the queries
 * against them; the DateTimeExtensionsTests rows check the Unix interop. Tests are UTF-8; the
 * library headers are ASCII. */
#include "generated/calendar_tables.h"
#include "generated/patro_tables.h"
#include "harness.h"
#include "summary.h"

#include <nepdate.h>
#include <nepdate_patro.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static nepdate_date bs(int year, int month, int day) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_bs(year, month, day, &date), NEPDATE_OK);
    return date;
}

static uint32_t serial_of(nepdate_date date) { return (uint32_t)date.value - 1U; }

static nepdate_patro_name table_name(uint16_t index) {
    nepdate_patro_name name;
    const uint32_t start = nepdate_patro_name_offsets[(size_t)index * 2U];
    const uint32_t middle = nepdate_patro_name_offsets[(size_t)index * 2U + 1U];
    const uint32_t end = nepdate_patro_name_offsets[(size_t)index * 2U + 2U];
    name.np = nepdate_patro_name_blob + start;
    name.np_length = (size_t)(middle - start);
    name.en = nepdate_patro_name_blob + middle;
    name.en_length = (size_t)(end - middle);
    return name;
}

static bool name_is(nepdate_patro_name name, const char *np, const char *en) {
    return name.np != NULL && name.en != NULL && name.np_length == strlen(np) &&
           name.en_length == strlen(en) && memcmp(name.np, np, name.np_length) == 0 &&
           memcmp(name.en, en, name.en_length) == 0;
}

/* ---- CalendarDataTests: this port's tables ---- */

TEST(calendar_strings_pool_is_not_empty) { CHECK(nepdate_patro_name_blob[0] != '\0'); }

TEST(calendar_strings_have_non_empty_names) {
    const size_t names = sizeof nepdate_patro_name_offsets / sizeof nepdate_patro_name_offsets[0];
    CHECK_EQ_INT(names, 2U * summary_number("names") + 1U);
    size_t failures = 0;
    for (size_t k = 0; k + 1 < names; k += 2) {
        failures += nepdate_patro_name_offsets[k] < nepdate_patro_name_offsets[k + 1] ? 0U : 1U;
        failures += nepdate_patro_name_offsets[k + 1] < nepdate_patro_name_offsets[k + 2] ? 0U : 1U;
    }
    CHECK_EQ_INT(failures, 0);
}

TEST(month_start_has_expected_length) {
    CHECK_EQ_INT(sizeof nepdate_cal_month_start / sizeof nepdate_cal_month_start[0],
                 299U * 12U + 1U);
}

TEST(month_start_is_strictly_monotonic) {
    size_t failures = 0;
    for (size_t i = 0; i + 1 < sizeof nepdate_cal_month_start / sizeof nepdate_cal_month_start[0];
         i++) {
        failures += nepdate_cal_month_start[i] < nepdate_cal_month_start[i + 1] ? 0U : 1U;
    }
    CHECK_EQ_INT(failures, 0);
}

TEST(total_days_matches_expected_count) {
    const size_t count = sizeof nepdate_cal_month_start / sizeof nepdate_cal_month_start[0];
    CHECK_EQ_INT(nepdate_cal_month_start[count - 1U], 109212);
}

TEST(tithi_day_offsets_are_strictly_sorted) {
    size_t runs = 0;
    size_t tithi_days = 0;
    uint32_t first = 0;
    uint32_t last = 0;
    for (uint32_t i = 0; i < NEPDATE_PATRO_DAYS; i++) {
        const bool has = nepdate_patro_day_tithi[i] != 0xFFU;
        if (has && (i == 0 || nepdate_patro_day_tithi[i - 1U] == 0xFFU)) {
            runs++;
            first = i;
        }
        if (has) {
            last = i;
            tithi_days++;
        }
    }
    CHECK_EQ_INT(runs, 1);
    CHECK(first < last);
    CHECK(last < NEPDATE_PATRO_DAYS);
    CHECK_EQ_INT(tithi_days, summary_number("tithi_days"));
}

TEST(tithi_pool_ids_are_in_range) {
    size_t failures = 0;
    const unsigned long tithi_names = summary_number("tithi_names");
    for (size_t i = 0; i < NEPDATE_PATRO_DAYS; i++) {
        failures += nepdate_patro_day_tithi[i] != 0xFFU && nepdate_patro_day_tithi[i] >= tithi_names
                        ? 1U
                        : 0U;
    }
    CHECK_EQ_INT(failures, 0);
}

TEST(tithi_entry_count_matches_the_summary) {
    size_t tithi_days = 0;
    for (size_t i = 0; i < NEPDATE_PATRO_DAYS; i++) {
        tithi_days += nepdate_patro_day_tithi[i] != 0xFFU ? 1U : 0U;
    }
    CHECK_EQ_INT(tithi_days, summary_number("tithi_days"));
}

TEST(event_day_offsets_are_strictly_sorted) {
    size_t event_days = 0;
    for (size_t i = 0; i < NEPDATE_PATRO_DAYS; i++) {
        event_days +=
            nepdate_patro_day_event_start[i] < nepdate_patro_day_event_start[i + 1U] ? 1U : 0U;
    }
    CHECK_EQ_INT(event_days, summary_number("event_days"));
}

TEST(event_slice_start_is_non_decreasing) {
    size_t failures = 0;
    const size_t count =
        sizeof nepdate_patro_day_event_start / sizeof nepdate_patro_day_event_start[0];
    for (size_t i = 0; i + 1 < count; i++) {
        failures +=
            nepdate_patro_day_event_start[i] <= nepdate_patro_day_event_start[i + 1U] ? 0U : 1U;
    }
    CHECK_EQ_INT(failures, 0);
}

TEST(event_slice_start_sentinel_equals_entries_length) {
    const size_t count =
        sizeof nepdate_patro_day_event_start / sizeof nepdate_patro_day_event_start[0];
    CHECK_EQ_INT(count, (size_t)NEPDATE_PATRO_DAYS + 1U);
    CHECK_EQ_INT(nepdate_patro_day_event_start[count - 1U],
                 sizeof nepdate_patro_event_names / sizeof nepdate_patro_event_names[0]);
}

TEST(event_entries_are_in_range) {
    size_t failures = 0;
    for (size_t i = 0; i < sizeof nepdate_patro_event_names / sizeof nepdate_patro_event_names[0];
         i++) {
        failures += nepdate_patro_event_names[i] < NEPDATE_PATRO_NAMES ? 0U : 1U;
    }
    CHECK_EQ_INT(failures, 0);
}

TEST(event_day_count_matches_the_summary) {
    size_t event_days = 0;
    for (size_t i = 0; i < NEPDATE_PATRO_DAYS; i++) {
        event_days +=
            nepdate_patro_day_event_start[i] < nepdate_patro_day_event_start[i + 1U] ? 1U : 0U;
    }
    CHECK_EQ_INT(event_days, summary_number("event_days"));
}

TEST(holiday_day_offsets_are_strictly_sorted) {
    size_t failures = 0;
    for (size_t i = 1;
         i < sizeof nepdate_patro_holiday_values / sizeof nepdate_patro_holiday_values[0]; i++) {
        failures +=
            nepdate_patro_holiday_values[i - 1U] < nepdate_patro_holiday_values[i] ? 0U : 1U;
    }
    CHECK_EQ_INT(failures, 0);
}

TEST(holiday_count_matches_the_summary) {
    CHECK_EQ_INT(sizeof nepdate_patro_holiday_values / sizeof nepdate_patro_holiday_values[0],
                 summary_number("holidays"));
}

TEST(get_offset_base_year_returns_zero) {
    const nepdate_date first = bs(1901, 1, 1);
    CHECK_EQ_INT(serial_of(first), 0);
}

TEST(get_offset_out_of_range_returns_negative_one) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_bs(1900, 1, 1, &date), NEPDATE_E_OUT_OF_RANGE);
}

TEST(all_offsets_are_unique) {
    size_t failures = 0;
    for (size_t i = 0; i + 1 < sizeof nepdate_cal_month_start / sizeof nepdate_cal_month_start[0];
         i++) {
        failures += nepdate_cal_month_start[i] < nepdate_cal_month_start[i + 1] ? 0U : 1U;
    }
    CHECK_EQ_INT(failures, 0);
}

/* ---- CalendarDataTests: the PATRO section 4 anchors through the API ---- */

TEST(anchor_2081_01_01) {
    nepdate_patro_day_info info;
    CHECK_EQ_INT(nepdate_patro_info(bs(2081, 1, 1), &info), NEPDATE_OK);
    CHECK(info.has_tithi);
    CHECK(name_is(info.tithi, "पञ्चमी", "Panchami"));
    nepdate_patro_name events[3];
    CHECK_EQ_INT(nepdate_patro_events(bs(2081, 1, 1), events, 3), 3);
    CHECK(name_is(events[0], "नयाँ वर्ष", "New Year"));
    CHECK(name_is(events[1], "मेष संक्रान्ति", "Mesh Sankranti"));
    CHECK(name_is(events[2], "बिस्का: जात्रा", "Bisket Jatra"));
}

TEST(anchor_2081_01_02_is_shashthi) {
    nepdate_patro_day_info info;
    CHECK_EQ_INT(nepdate_patro_info(bs(2081, 1, 2), &info), NEPDATE_OK);
    CHECK(info.has_tithi);
    CHECK(name_is(info.tithi, "षष्ठी", "Shashthi"));
}

TEST(anchor_2081_01_05_is_holiday) {
    nepdate_patro_day_info info;
    CHECK_EQ_INT(nepdate_patro_info(bs(2081, 1, 5), &info), NEPDATE_OK);
    CHECK(info.is_public_holiday);
}

TEST(anchor_2081_02_15_is_republic_day) {
    nepdate_patro_day_info info;
    CHECK_EQ_INT(nepdate_patro_info(bs(2081, 2, 15), &info), NEPDATE_OK);
    CHECK(info.is_public_holiday);
    nepdate_patro_name event;
    CHECK_EQ_INT(nepdate_patro_events(bs(2081, 2, 15), &event, 1), 1);
    CHECK(name_is(event, "गणतन्त्र दिवस", "Republic Day"));
}

TEST(anchor_2080_11_01_calendar_data) {
    nepdate_patro_day_info info;
    CHECK_EQ_INT(nepdate_patro_info(bs(2080, 11, 1), &info), NEPDATE_OK);
    CHECK(info.has_tithi);
    CHECK(name_is(info.tithi, "चतुर्थी", "Chaturthi"));
    CHECK_EQ_INT(info.event_count, 5);
}

TEST(before_range_has_no_data) {
    nepdate_patro_day_info info;
    CHECK_EQ_INT(nepdate_patro_info(bs(2070, 6, 15), &info), NEPDATE_OK);
    CHECK(!info.has_tithi);
    CHECK(!info.is_public_holiday);
    CHECK_EQ_INT(info.event_count, 0);
}

TEST(after_range_has_no_data) {
    nepdate_patro_day_info info;
    CHECK_EQ_INT(nepdate_patro_info(bs(2090, 1, 1), &info), NEPDATE_OK);
    CHECK(!info.has_tithi);
    CHECK(!info.is_public_holiday);
    CHECK_EQ_INT(info.event_count, 0);
}

TEST(outside_tithi_range_has_no_tithi) {
    nepdate_patro_day_info info;
    CHECK_EQ_INT(nepdate_patro_info(bs(2077, 5, 1), &info), NEPDATE_OK);
    CHECK(!info.has_tithi);
}

TEST(event_names_have_both_languages) {
    nepdate_patro_name events[8];
    const size_t count = nepdate_patro_events(bs(2081, 1, 1), events, 8);
    CHECK_EQ_INT(count, 3);
    for (size_t i = 0; i < count; i++) {
        CHECK(events[i].np_length > 0);
        CHECK(events[i].en_length > 0);
    }
}

TEST(api_tithi_matches_the_table) {
    const nepdate_date date = bs(2081, 1, 1);
    const uint32_t offset = serial_of(date) - NEPDATE_PATRO_FIRST_SERIAL;
    const nepdate_patro_name table = table_name(nepdate_patro_day_tithi[offset]);
    nepdate_patro_day_info info;
    CHECK_EQ_INT(nepdate_patro_info(date, &info), NEPDATE_OK);
    CHECK(info.has_tithi);
    CHECK_EQ_INT(info.tithi.np_length, table.np_length);
    CHECK_EQ_INT(info.tithi.en_length, table.en_length);
    CHECK(memcmp(info.tithi.np, table.np, table.np_length) == 0);
    CHECK(memcmp(info.tithi.en, table.en, table.en_length) == 0);
}

TEST(all_covered_days_are_in_bounds) {
    size_t failures = 0;
    for (uint32_t i = 0; i < NEPDATE_PATRO_DAYS; i++) {
        const uint8_t tithi = nepdate_patro_day_tithi[i];
        failures += tithi != 0xFFU && tithi >= 32U ? 1U : 0U;
        const uint16_t first = nepdate_patro_day_event_start[i];
        failures += nepdate_patro_day_event_start[i + 1U] < first ? 1U : 0U;
    }
    CHECK_EQ_INT(failures, 0);
}

TEST(all_covered_days_query_without_failure) {
    size_t failures = 0;
    for (uint32_t i = 0; i < NEPDATE_PATRO_DAYS; i++) {
        const nepdate_date date = {(int32_t)(NEPDATE_PATRO_FIRST_SERIAL + i + 1U)};
        nepdate_patro_day_info info;
        failures += nepdate_patro_info(date, &info) == NEPDATE_OK ? 0U : 1U;
        nepdate_patro_name events[12];
        failures += nepdate_patro_events(date, events, 12) == info.event_count ? 0U : 1U;
        if (info.has_tithi) {
            failures += info.tithi.np_length > 0 && info.tithi.en_length > 0 ? 0U : 1U;
        }
    }
    CHECK_EQ_INT(failures, 0);
}

TEST(all_tithi_names_are_non_empty) {
    size_t failures = 0;
    for (uint16_t k = 0; k < 32; k++) {
        const nepdate_patro_name name = table_name(k);
        failures += name.np_length > 0 && name.en_length > 0 ? 0U : 1U;
    }
    CHECK_EQ_INT(failures, 0);
}

TEST(all_holiday_values_are_in_bounds) {
    size_t failures = 0;
    for (size_t i = 0;
         i < sizeof nepdate_patro_holiday_values / sizeof nepdate_patro_holiday_values[0]; i++) {
        const uint32_t serial = (uint32_t)nepdate_patro_holiday_values[i] - 1U;
        failures += serial >= NEPDATE_PATRO_FIRST_SERIAL &&
                            serial < NEPDATE_PATRO_FIRST_SERIAL + NEPDATE_PATRO_DAYS
                        ? 0U
                        : 1U;
    }
    CHECK_EQ_INT(failures, 0);
}

TEST(all_event_offsets_are_in_bounds) {
    size_t failures = 0;
    for (size_t i = 0; i < NEPDATE_PATRO_DAYS; i++) {
        failures += nepdate_patro_day_event_start[i] <=
                            sizeof nepdate_patro_event_names / sizeof nepdate_patro_event_names[0]
                        ? 0U
                        : 1U;
    }
    CHECK_EQ_INT(failures, 0);
}

TEST(holiday_count_from_the_api_matches_the_summary) {
    CHECK_EQ_INT(nepdate_patro_public_holidays(NULL, 0), summary_number("holidays"));
}

TEST(calendar_info_event_array_is_never_null) {
    nepdate_patro_name events[1];
    CHECK(nepdate_patro_events(bs(2081, 1, 1), events, 1) > 0);
}

/* ---- DateTimeExtensionsTests ---- */

TEST(to_nepali_date_valid_datetime) {
    /* 2023-09-01 12:00:00 UTC. */
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_unix(INT64_C(1693569600), 0, &date), NEPDATE_OK);
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(date, &ymd), NEPDATE_OK);
    CHECK_EQ_INT(ymd.year, 2080);
}

TEST(to_nepali_date_min_value_is_out_of_range) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_ad(1, 1, 1, &date), NEPDATE_E_OUT_OF_RANGE);
}

TEST(to_nepali_date_max_value_is_out_of_range) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_ad(9999, 12, 31, &date), NEPDATE_E_OUT_OF_RANGE);
}

TEST(to_nepali_date_before_min_supported_is_out_of_range) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_ad(1844, 4, 10, &date), NEPDATE_E_OUT_OF_RANGE);
}

TEST(to_nepali_date_after_max_supported_is_out_of_range) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_ad(2143, 4, 16, &date), NEPDATE_E_OUT_OF_RANGE);
}

int main(void) {
    RUN_TEST_NAMED("CalendarDataTests.CalendarStrings_Pool_IsNonEmpty",
                   calendar_strings_pool_is_not_empty);
    RUN_TEST_NAMED("CalendarDataTests.CalendarStrings_Pool_AllEntriesHaveNonEmptyNpAndEn",
                   calendar_strings_have_non_empty_names);
    RUN_TEST_NAMED("CalendarDataTests.CalendarOffsets_MonthStart_HasExpectedLength",
                   month_start_has_expected_length);
    RUN_TEST_NAMED("CalendarDataTests.CalendarOffsets_MonthStart_IsStrictlyMonotonic",
                   month_start_is_strictly_monotonic);
    RUN_TEST_NAMED("CalendarDataTests.CalendarOffsets_TotalDays_MatchesExpectedCount",
                   total_days_matches_expected_count);
    RUN_TEST_NAMED("CalendarDataTests.TithiData_DayOffsets_IsStrictlySorted",
                   tithi_day_offsets_are_strictly_sorted);
    RUN_TEST_NAMED("CalendarDataTests.TithiData_AllPoolIds_AreInRange",
                   tithi_pool_ids_are_in_range);
    RUN_TEST_NAMED("CalendarDataTests.TithiData_EntryCount_Matches2190",
                   tithi_entry_count_matches_the_summary);
    RUN_TEST_NAMED("CalendarDataTests.EventData_DayOffsets_IsStrictlySorted",
                   event_day_offsets_are_strictly_sorted);
    RUN_TEST_NAMED("CalendarDataTests.EventData_SliceStart_IsNonDecreasing",
                   event_slice_start_is_non_decreasing);
    RUN_TEST_NAMED("CalendarDataTests.EventData_SliceStart_SentinelEqualsEntriesLength",
                   event_slice_start_sentinel_equals_entries_length);
    RUN_TEST_NAMED("CalendarDataTests.EventData_AllEntries_AreInRange", event_entries_are_in_range);
    RUN_TEST_NAMED("CalendarDataTests.EventData_DayCount_Matches1671",
                   event_day_count_matches_the_summary);
    RUN_TEST_NAMED("CalendarDataTests.HolidayData_DayOffsets_IsStrictlySorted",
                   holiday_day_offsets_are_strictly_sorted);
    RUN_TEST_NAMED("CalendarDataTests.HolidayData_Count_Matches294",
                   holiday_count_matches_the_summary);
    RUN_TEST_NAMED("CalendarDataTests.CalendarOffsets_GetOffset_BaseYear_Month1_Day1_ReturnsZero",
                   get_offset_base_year_returns_zero);
    RUN_TEST_NAMED("CalendarDataTests.CalendarOffsets_GetOffset_OutOfRange_ReturnsNegativeOne",
                   get_offset_out_of_range_returns_negative_one);
    RUN_TEST_NAMED("CalendarDataTests.CalendarOffsets_GetOffset_AllOffsetsAreUniqueSentinel",
                   all_offsets_are_unique);
    RUN_TEST_NAMED("CalendarDataTests.NepaliDate_2081_01_01_CalendarData", anchor_2081_01_01);
    RUN_TEST_NAMED("CalendarDataTests.NepaliDate_2081_01_02_Tithi_IsShashthi",
                   anchor_2081_01_02_is_shashthi);
    RUN_TEST_NAMED("CalendarDataTests.NepaliDate_2081_01_05_IsPublicHoliday",
                   anchor_2081_01_05_is_holiday);
    RUN_TEST_NAMED("CalendarDataTests.NepaliDate_2081_02_15_IsPublicHoliday_GanatantraDiwas",
                   anchor_2081_02_15_is_republic_day);
    RUN_TEST_NAMED("CalendarDataTests.NepaliDate_2080_11_01_CalendarData",
                   anchor_2080_11_01_calendar_data);
    RUN_TEST_NAMED("CalendarDataTests.NepaliDate_BeforeRange_AllPropertiesReturnDefaults",
                   before_range_has_no_data);
    RUN_TEST_NAMED("CalendarDataTests.NepaliDate_AfterRange_AllPropertiesReturnDefaults",
                   after_range_has_no_data);
    RUN_TEST_NAMED("CalendarDataTests.NepaliDate_OutsideTithiRange_TithiIsEmpty",
                   outside_tithi_range_has_no_tithi);
    RUN_TEST_NAMED("CalendarDataTests.NepaliDate_EventsNp_And_EventsEn_AlwaysHaveSameLength",
                   event_names_have_both_languages);
    RUN_TEST_NAMED("CalendarDataTests.GetCalendarInfo_TithiMatchesFlatProperties",
                   api_tithi_matches_the_table);
    RUN_TEST_NAMED("CalendarDataTests.AllDaysInScrapedRange_OffsetsAreInBounds",
                   all_covered_days_are_in_bounds);
    RUN_TEST_NAMED("CalendarDataTests.AllDaysInScrapedRange_NoExceptionOnPropertyAccess",
                   all_covered_days_query_without_failure);
    RUN_TEST_NAMED("CalendarDataTests.AllTithiEntries_ResolveToNonEmptyStrings",
                   all_tithi_names_are_non_empty);
    RUN_TEST_NAMED("CalendarDataTests.AllHolidayOffsets_AreInBounds",
                   all_holiday_values_are_in_bounds);
    RUN_TEST_NAMED("CalendarDataTests.AllEventOffsets_AreInBounds",
                   all_event_offsets_are_in_bounds);
    RUN_TEST_NAMED("CalendarDataTests.HolidayCount_ViaApi_Matches294",
                   holiday_count_from_the_api_matches_the_summary);
    RUN_TEST_NAMED("CalendarDataTests.CalendarInfo_EventArrays_AreNeverNull",
                   calendar_info_event_array_is_never_null);
    RUN_TEST_NAMED("DateTimeExtensionsTests.ToNepaliDate_ValidDateTime_ReturnsCorrectNepaliDate",
                   to_nepali_date_valid_datetime);
    RUN_TEST_NAMED(
        "DateTimeExtensionsTests.ToNepaliDate_MinValue_ThrowsArgumentOutOfRangeException",
        to_nepali_date_min_value_is_out_of_range);
    RUN_TEST_NAMED(
        "DateTimeExtensionsTests.ToNepaliDate_MaxValue_ThrowsArgumentOutOfRangeException",
        to_nepali_date_max_value_is_out_of_range);
    RUN_TEST_NAMED("DateTimeExtensionsTests.ToNepaliDate_DateBeforeMinSupported_"
                   "ThrowsArgumentOutOfRangeException",
                   to_nepali_date_before_min_supported_is_out_of_range);
    RUN_TEST_NAMED("DateTimeExtensionsTests.ToNepaliDate_DateAfterMaxSupported_"
                   "ThrowsArgumentOutOfRangeException",
                   to_nepali_date_after_max_supported_is_out_of_range);
    return harness_finish();
}
