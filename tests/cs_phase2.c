/* The phase-2 rows of the C# test catalogue, each named after its catalog.tsv id. Rows a PARITY
 * deviation covers have a cs-deviation marker naming the D-ID. */
#include "harness.h"

#include <nepdate.h>

#include <stdint.h>
#include <string.h>

static nepdate_date bs(int year, int month, int day) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_bs(year, month, day, &date), NEPDATE_OK);
    return date;
}

/* 2024-07-30 18:30 UTC, the current-date rows' injected clock. */
static nepdate_date today_nepal(void) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_today_nepal(INT64_C(1722364200), &date), NEPDATE_OK);
    return date;
}

static nepdate_status parse_lenient_status(const char *text) {
    nepdate_date date = {0};
    return nepdate_parse_lenient(text, strlen(text), &date);
}

static void expect(nepdate_date date, int year, int month, int day) {
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(date, &ymd), NEPDATE_OK);
    CHECK_EQ_INT(ymd.year, year);
    CHECK_EQ_INT(ymd.month, month);
    CHECK_EQ_INT(ymd.day, day);
}

static int fiscal_year(nepdate_date date) {
    int fiscal_year = 0;
    CHECK_EQ_INT(nepdate_fiscal_year_of(date, &fiscal_year), NEPDATE_OK);
    return fiscal_year;
}

static nepdate_date fiscal_start(int fiscal_year) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_fiscal_year_start(fiscal_year, &date), NEPDATE_OK);
    return date;
}

static nepdate_date fiscal_end(int fiscal_year) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_fiscal_year_end(fiscal_year, &date), NEPDATE_OK);
    return date;
}

static nepdate_date quarter_start(int fiscal_year, nepdate_quarter quarter) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_quarter_start(fiscal_year, quarter, &date), NEPDATE_OK);
    return date;
}

static nepdate_date quarter_end(int fiscal_year, nepdate_quarter quarter) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_quarter_end(fiscal_year, quarter, &date), NEPDATE_OK);
    return date;
}

static nepdate_date add_days(nepdate_date date, int32_t days) {
    nepdate_date result = {0};
    CHECK_EQ_INT(nepdate_add_days(date, days, &result), NEPDATE_OK);
    return result;
}

static nepdate_date add_months(nepdate_date date, int32_t months, nepdate_overflow overflow) {
    nepdate_date result = {0};
    CHECK_EQ_INT(nepdate_add_months(date, months, overflow, &result), NEPDATE_OK);
    return result;
}

static nepdate_range make_range(nepdate_date start, nepdate_date end) {
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_make(start, end, &range), NEPDATE_OK);
    return range;
}

static bool ranges_equal(nepdate_range a, nepdate_range b) {
    return a.start.value == b.start.value && a.end.value == b.end.value;
}

/* ---------- Core/FiscalYearInstanceMethodTests.cs ---------- */

TEST(fiscal_start_shrawan) { expect(fiscal_start(fiscal_year(bs(2080, 4, 15))), 2080, 4, 1); }

TEST(fiscal_start_chaitra) { expect(fiscal_start(fiscal_year(bs(2080, 12, 15))), 2080, 4, 1); }

TEST(fiscal_start_baishakh) { expect(fiscal_start(fiscal_year(bs(2080, 1, 15))), 2079, 4, 1); }

TEST(fiscal_start_ashad) { expect(fiscal_start(fiscal_year(bs(2080, 3, 15))), 2079, 4, 1); }

TEST(fiscal_start_offset_positive) {
    expect(fiscal_start(fiscal_year(bs(2080, 4, 15)) + 1), 2081, 4, 1);
}

TEST(fiscal_start_offset_negative) {
    expect(fiscal_start(fiscal_year(bs(2080, 4, 15)) - 1), 2079, 4, 1);
}

TEST(fiscal_end_shrawan) { expect(fiscal_end(fiscal_year(bs(2080, 4, 15))), 2081, 3, 31); }

TEST(fiscal_end_chaitra) { expect(fiscal_end(fiscal_year(bs(2080, 12, 15))), 2081, 3, 31); }

TEST(fiscal_end_baishakh) { expect(fiscal_end(fiscal_year(bs(2080, 1, 15))), 2080, 3, 31); }

TEST(fiscal_end_offset_positive) {
    expect(fiscal_end(fiscal_year(bs(2080, 4, 15)) + 1), 2082, 3, 32);
}

TEST(fiscal_both_q2) {
    const int fy = fiscal_year(bs(2080, 7, 15));
    expect(fiscal_start(fy), 2080, 4, 1);
    expect(fiscal_end(fy), 2081, 3, 31);
}

TEST(fiscal_both_q4) {
    const int fy = fiscal_year(bs(2081, 2, 10));
    expect(fiscal_start(fy), 2080, 4, 1);
    expect(fiscal_end(fy), 2081, 3, 31);
}

TEST(quarter_start_q1) { expect(quarter_start(2080, NEPDATE_Q1), 2080, 4, 1); }

TEST(quarter_start_q2) { expect(quarter_start(2080, NEPDATE_Q2), 2080, 7, 1); }

TEST(quarter_start_q3) { expect(quarter_start(2080, NEPDATE_Q3), 2080, 10, 1); }

TEST(quarter_start_q4) { expect(quarter_start(2080, NEPDATE_Q4), 2081, 1, 1); }

TEST(quarter_end_q1) { expect(quarter_end(2080, NEPDATE_Q1), 2080, 6, 30); }

TEST(quarter_end_q2) { expect(quarter_end(2080, NEPDATE_Q2), 2080, 9, 29); }

TEST(quarter_end_q3) { expect(quarter_end(2080, NEPDATE_Q3), 2080, 12, 30); }

TEST(quarter_end_q4) { expect(quarter_end(2080, NEPDATE_Q4), 2081, 3, 31); }

TEST(quarter_both_q2) {
    expect(quarter_start(2080, NEPDATE_Q2), 2080, 7, 1);
    expect(quarter_end(2080, NEPDATE_Q2), 2080, 9, 29);
}

TEST(quarter_both_q3) {
    expect(quarter_start(2080, NEPDATE_Q3), 2080, 10, 1);
    expect(quarter_end(2080, NEPDATE_Q3), 2080, 12, 30);
}

TEST(quarter_current_q1_start) {
    const nepdate_date date = bs(2080, 4, 15);
    nepdate_quarter quarter = NEPDATE_Q1;
    CHECK_EQ_INT(nepdate_quarter_of(date, &quarter), NEPDATE_OK);
    expect(quarter_start(fiscal_year(date), quarter), 2080, 4, 1);
}

TEST(quarter_current_q1_end) {
    const nepdate_date date = bs(2080, 4, 15);
    nepdate_quarter quarter = NEPDATE_Q1;
    CHECK_EQ_INT(nepdate_quarter_of(date, &quarter), NEPDATE_OK);
    expect(quarter_end(fiscal_year(date), quarter), 2080, 6, 30);
}

TEST(quarter_current_q2_start) {
    const nepdate_date date = bs(2080, 7, 15);
    nepdate_quarter quarter = NEPDATE_Q1;
    CHECK_EQ_INT(nepdate_quarter_of(date, &quarter), NEPDATE_OK);
    expect(quarter_start(fiscal_year(date), quarter), 2080, 7, 1);
}

TEST(quarter_current_q3_start) {
    const nepdate_date date = bs(2080, 10, 15);
    nepdate_quarter quarter = NEPDATE_Q1;
    CHECK_EQ_INT(nepdate_quarter_of(date, &quarter), NEPDATE_OK);
    expect(quarter_start(fiscal_year(date), quarter), 2080, 10, 1);
}

TEST(quarter_current_q3_end) {
    const nepdate_date date = bs(2080, 10, 15);
    nepdate_quarter quarter = NEPDATE_Q1;
    CHECK_EQ_INT(nepdate_quarter_of(date, &quarter), NEPDATE_OK);
    expect(quarter_end(fiscal_year(date), quarter), 2080, 12, 30);
}

TEST(quarter_start_q1_offset_positive) {
    expect(quarter_start(fiscal_year(bs(2080, 4, 15)) + 1, NEPDATE_Q1), 2081, 4, 1);
}

TEST(quarter_start_q1_offset_negative) {
    expect(quarter_start(fiscal_year(bs(2080, 4, 15)) - 1, NEPDATE_Q1), 2079, 4, 1);
}

TEST(fiscal_start_all_fiscal_months_1) {
    expect(fiscal_start(fiscal_year(bs(2080, 4, 1))), 2080, 4, 1);
}

TEST(fiscal_start_all_fiscal_months_2) {
    expect(fiscal_start(fiscal_year(bs(2080, 6, 1))), 2080, 4, 1);
}

TEST(fiscal_start_all_fiscal_months_3) {
    expect(fiscal_start(fiscal_year(bs(2080, 7, 1))), 2080, 4, 1);
}

TEST(fiscal_start_all_fiscal_months_4) {
    expect(fiscal_start(fiscal_year(bs(2080, 12, 1))), 2080, 4, 1);
}

TEST(fiscal_start_q4_months_1) { expect(fiscal_start(fiscal_year(bs(2080, 1, 1))), 2079, 4, 1); }

TEST(fiscal_start_q4_months_2) { expect(fiscal_start(fiscal_year(bs(2080, 2, 1))), 2079, 4, 1); }

TEST(fiscal_start_q4_months_3) { expect(fiscal_start(fiscal_year(bs(2080, 3, 1))), 2079, 4, 1); }

TEST(quarter_start_from_q4_q1) {
    expect(quarter_start(fiscal_year(bs(2081, 2, 10)), NEPDATE_Q1), 2080, 4, 1);
}

TEST(quarter_start_from_q4_q2) {
    expect(quarter_start(fiscal_year(bs(2081, 2, 10)), NEPDATE_Q2), 2080, 7, 1);
}

TEST(quarter_start_from_q4_q3) {
    expect(quarter_start(fiscal_year(bs(2081, 2, 10)), NEPDATE_Q3), 2080, 10, 1);
}

TEST(quarter_start_from_q4_q4) {
    expect(quarter_start(fiscal_year(bs(2081, 2, 10)), NEPDATE_Q4), 2081, 1, 1);
}

TEST(quarter_start_from_q4_current) {
    const nepdate_date date = bs(2081, 2, 10);
    nepdate_quarter quarter = NEPDATE_Q1;
    CHECK_EQ_INT(nepdate_quarter_of(date, &quarter), NEPDATE_OK);
    expect(quarter_start(fiscal_year(date), quarter), 2081, 1, 1);
}

TEST(quarter_end_from_q4_q1) {
    expect(quarter_end(fiscal_year(bs(2081, 2, 10)), NEPDATE_Q1), 2080, 6, 30);
}

TEST(quarter_end_from_q4_q3) {
    expect(quarter_end(fiscal_year(bs(2081, 2, 10)), NEPDATE_Q3), 2080, 12, 30);
}

TEST(quarter_end_from_q4_q4) {
    expect(quarter_end(fiscal_year(bs(2081, 2, 10)), NEPDATE_Q4), 2081, 3, 31);
}

TEST(quarter_end_from_q4_current) {
    const nepdate_date date = bs(2081, 2, 10);
    nepdate_quarter quarter = NEPDATE_Q1;
    CHECK_EQ_INT(nepdate_quarter_of(date, &quarter), NEPDATE_OK);
    expect(quarter_end(fiscal_year(date), quarter), 2081, 3, 31);
}

/* ---------- Core/FiscalYearTests.cs ---------- */

TEST(fiscal_tests_start_and_end_2080) {
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_fiscal_year(2080, &range), NEPDATE_OK);
    expect(range.start, 2080, 4, 1);
    expect(range.end, 2081, 3, 31);
}

TEST(fiscal_tests_invalid_1843) {
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_fiscal_year(1843, &range), NEPDATE_E_OUT_OF_RANGE);
}

TEST(fiscal_tests_invalid_2200) {
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_fiscal_year(2200, &range), NEPDATE_E_OUT_OF_RANGE);
}

TEST(fiscal_tests_start_2080) { expect(fiscal_start(2080), 2080, 4, 1); }

TEST(fiscal_tests_end_2080) { expect(fiscal_end(2080), 2081, 3, 31); }

TEST(fiscal_tests_instance_start_and_end) {
    const int fy = fiscal_year(bs(2080, 6, 15));
    expect(fiscal_start(fy), 2080, 4, 1);
    expect(fiscal_end(fy), 2081, 3, 31);
}

TEST(fiscal_tests_instance_start) {
    expect(fiscal_start(fiscal_year(bs(2080, 6, 15))), 2080, 4, 1);
}

TEST(fiscal_tests_instance_end) { expect(fiscal_end(fiscal_year(bs(2080, 6, 15))), 2081, 3, 31); }

TEST(fiscal_tests_quarter_start_and_end) {
    expect(quarter_start(2080, NEPDATE_Q1), 2080, 4, 1);
    expect(quarter_end(2080, NEPDATE_Q1), 2080, 6, 30);
}

TEST(fiscal_tests_quarter_start) { expect(quarter_start(2080, NEPDATE_Q2), 2080, 7, 1); }

TEST(fiscal_tests_quarter_end) { expect(quarter_end(2080, NEPDATE_Q3), 2080, 12, 30); }

/* ---------- Core/NepaliDateArithmeticEdgeCaseTests.cs ---------- */

TEST(add_days_zero) {
    const nepdate_date date = bs(2080, 5, 15);
    CHECK_EQ_INT(add_days(date, 0).value, date.value);
}

TEST(add_days_positive_month) { expect(add_days(bs(2080, 5, 31), 1), 2080, 6, 1); }

TEST(add_days_negative_month) { expect(add_days(bs(2080, 6, 1), -1), 2080, 5, 31); }

TEST(add_days_positive_year) { expect(add_days(bs(2080, 12, 30), 1), 2081, 1, 1); }

TEST(add_days_negative_year) { expect(add_days(bs(2081, 1, 1), -1), 2080, 12, 30); }

TEST(add_days_large_positive) {
    const nepdate_date start = bs(2080, 1, 1);
    const nepdate_date result = add_days(start, 365);
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(result, &ymd), NEPDATE_OK);
    CHECK(ymd.year == 2081 || ymd.year == 2080);
    CHECK(result.value >= start.value);
}

TEST(add_months_positive_year) {
    expect(add_months(bs(2080, 11, 15), 5, NEPDATE_CLAMP), 2081, 4, 15);
}

TEST(add_months_negative_year) {
    expect(add_months(bs(2080, 3, 15), -5, NEPDATE_CLAMP), 2079, 10, 15);
}

TEST(add_months_more_than_12) {
    expect(add_months(bs(2080, 5, 15), 14, NEPDATE_CLAMP), 2081, 7, 15);
}

TEST(add_months_exactly_12) { expect(add_months(bs(2080, 6, 15), 12, NEPDATE_CLAMP), 2081, 6, 15); }

TEST(add_months_capped) {
    const nepdate_date result = add_months(bs(2081, 4, 32), 5, NEPDATE_CLAMP);
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(result, &ymd), NEPDATE_OK);
    CHECK_EQ_INT(ymd.year, 2081);
    CHECK_EQ_INT(ymd.month, 9);
    CHECK(ymd.day >= 1 && ymd.day <= 29);
}

TEST(add_months_negative_same_day) {
    expect(add_months(bs(2080, 8, 10), -1, NEPDATE_CLAMP), 2080, 7, 10);
}

/* cs-deviation: NepaliDateArithmeticEdgeCaseTests.AddMonths_HalfMonth_AddsApproximately15Days D-02 */
/* cs-deviation: NepaliDateArithmeticEdgeCaseTests.AddMonths_NegativeHalfMonth_SubtractsApproximately15Days D-02 */
/* cs-deviation: NepaliDateArithmeticEdgeCaseTests.AddMonths_OneAndHalfMonths_AddsApproximately46Days D-02 */
/* cs-deviation: NepaliDateArithmeticEdgeCaseTests.AddMonths_FractionalValue_ReturnsDateFurtherThanZeroMonths D-02 */

TEST(constructor_midnight_ignores_time) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_ad(2023, 9, 1, &date), NEPDATE_OK);
    expect(date, 2080, 5, 15);
}

TEST(constructor_noon_ignores_time) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_ad(2023, 9, 1, &date), NEPDATE_OK);
    expect(date, 2080, 5, 15);
}

TEST(constructor_end_of_day_ignores_time) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_ad(2023, 9, 1, &date), NEPDATE_OK);
    expect(date, 2080, 5, 15);
}

TEST(constructor_same_english_date) {
    nepdate_date morning = {0};
    nepdate_date evening = {0};
    CHECK_EQ_INT(nepdate_from_ad(2023, 1, 1, &morning), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_from_ad(2023, 1, 1, &evening), NEPDATE_OK);
    CHECK_EQ_INT(morning.value, evening.value);
}

/* cs-deviation: NepaliDateArithmeticEdgeCaseTests.TryParse_AutoAdjust_DayYearMonthFormat_ReturnsCorrectDate D-06
 * (lenient parsing accepts year-month-day and day-month-year, not a day-year-month guess) */
/* cs-deviation: NepaliDateArithmeticEdgeCaseTests.TryParse_AutoAdjust_MonthDayYearInvertedOrder_ReturnsCorrectDate D-06
 * (lenient parsing does not guess a month-day-year order) */
/* cs-deviation: NepaliDateArithmeticEdgeCaseTests.TryParse_AutoAdjust_TwoDigitYear_ExpandsToCurrentMillennium D-06
 * (this port never expands a two-digit year; lenient parsing returns Ambiguous) */
TEST(try_parse_auto_adjust_invalid_date) {
    CHECK(parse_lenient_status("2080/13/40") != NEPDATE_OK);
}
TEST(try_parse_auto_adjust_valid_standard) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_parse_lenient("2080/05/15", 10, &date), NEPDATE_OK);
    expect(date, 2080, 5, 15);
}

/* ---------- Core/NepaliDateManipulationTests.cs ---------- */

TEST(month_end_date) {
    nepdate_date end = {0};
    CHECK_EQ_INT(nepdate_last_day_of_month(bs(2080, 5, 15), &end), NEPDATE_OK);
    expect(end, 2080, 5, 31);
}

TEST(manipulation_add_months_1) {
    expect(add_months(bs(2080, 5, 15), 1, NEPDATE_CLAMP), 2080, 6, 15);
}

TEST(manipulation_add_months_2) {
    expect(add_months(bs(2080, 5, 15), -1, NEPDATE_CLAMP), 2080, 4, 15);
}

TEST(manipulation_add_months_3) {
    expect(add_months(bs(2080, 5, 15), 12, NEPDATE_CLAMP), 2081, 5, 15);
}

TEST(manipulation_add_months_4) {
    expect(add_months(bs(2081, 4, 32), 5, NEPDATE_CLAMP), 2081, 9, 29);
}

TEST(manipulation_add_months_5) {
    expect(add_months(bs(2081, 4, 32), 5, NEPDATE_OVERFLOW), 2081, 10, 3);
}

TEST(manipulation_add_days_1) { expect(add_days(bs(2080, 5, 15), 5), 2080, 5, 20); }

TEST(manipulation_add_days_2) { expect(add_days(bs(2080, 5, 15), -5), 2080, 5, 10); }

TEST(manipulation_add_days_3) { expect(add_days(bs(2080, 5, 31), 1), 2080, 6, 1); }

TEST(manipulation_add_days_4) { expect(add_days(bs(2080, 12, 30), 5), 2081, 1, 5); }

TEST(manipulation_subtract) {
    int32_t days = 0;
    CHECK_EQ_INT(nepdate_diff_days(bs(2080, 5, 1), bs(2080, 5, 16), &days), NEPDATE_OK);
    CHECK_EQ_INT(days, 15);
}

TEST(manipulation_subtract_same) {
    int32_t days = -1;
    const nepdate_date date = bs(2080, 5, 15);
    CHECK_EQ_INT(nepdate_diff_days(date, date, &days), NEPDATE_OK);
    CHECK_EQ_INT(days, 0);
}

/* ---------- Core/NepaliDateRangeExtendedTests.cs ---------- */

TEST(range_calendar_year) {
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_year(2080, &range), NEPDATE_OK);
    expect(range.start, 2080, 1, 1);
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(range.end, &ymd), NEPDATE_OK);
    CHECK_EQ_INT(ymd.year, 2080);
    CHECK_EQ_INT(ymd.month, 12);
    int length = 0;
    CHECK_EQ_INT(nepdate_month_length(range.end, &length), NEPDATE_OK);
    CHECK_EQ_INT(ymd.day, length);
}

TEST(range_calendar_year_length) {
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_year(2080, &range), NEPDATE_OK);
    int length = 0;
    CHECK_EQ_INT(nepdate_range_length(range, &length), NEPDATE_OK);
    CHECK(length >= 365 && length <= 366);
}

TEST(current_month_start_is_first_day) {
    const nepdate_date today = today_nepal();
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(today, &ymd), NEPDATE_OK);
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_month(ymd.year, ymd.month, &range), NEPDATE_OK);
    expect(range.start, ymd.year, ymd.month, 1);
}

TEST(current_month_end_is_last_day) {
    const nepdate_date today = today_nepal();
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(today, &ymd), NEPDATE_OK);
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_month(ymd.year, ymd.month, &range), NEPDATE_OK);
    int length = 0;
    CHECK_EQ_INT(nepdate_month_length(today, &length), NEPDATE_OK);
    expect(range.end, ymd.year, ymd.month, length);
}

TEST(current_month_contains_today) {
    const nepdate_date today = today_nepal();
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(today, &ymd), NEPDATE_OK);
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_month(ymd.year, ymd.month, &range), NEPDATE_OK);
    bool contains = false;
    CHECK_EQ_INT(nepdate_range_contains(range, today, &contains), NEPDATE_OK);
    CHECK(contains);
}

TEST(current_fiscal_year_contains_today) {
    const nepdate_date today = today_nepal();
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_fiscal_year(fiscal_year(today), &range), NEPDATE_OK);
    bool contains = false;
    CHECK_EQ_INT(nepdate_range_contains(range, today, &contains), NEPDATE_OK);
    CHECK(contains);
}

TEST(current_fiscal_year_start_is_shrawan) {
    const nepdate_date today = today_nepal();
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_fiscal_year(fiscal_year(today), &range), NEPDATE_OK);
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(range.start, &ymd), NEPDATE_OK);
    CHECK_EQ_INT(ymd.month, 4);
    CHECK_EQ_INT(ymd.day, 1);
}

TEST(current_fiscal_year_end_is_ashadh) {
    const nepdate_date today = today_nepal();
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_fiscal_year(fiscal_year(today), &range), NEPDATE_OK);
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(range.end, &ymd), NEPDATE_OK);
    CHECK_EQ_INT(ymd.month, 3);
}

TEST(current_calendar_year_contains_today) {
    const nepdate_date today = today_nepal();
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(today, &ymd), NEPDATE_OK);
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_year(ymd.year, &range), NEPDATE_OK);
    bool contains = false;
    CHECK_EQ_INT(nepdate_range_contains(range, today, &contains), NEPDATE_OK);
    CHECK(contains);
}

TEST(current_calendar_year_start_is_baishakh_1) {
    const nepdate_date today = today_nepal();
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(today, &ymd), NEPDATE_OK);
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_year(ymd.year, &range), NEPDATE_OK);
    expect(range.start, ymd.year, 1, 1);
}

/* cs-deviation: NepaliDateRangeExtendedTests.SplitByFiscalQuarter_EmptyRange_ReturnsEmptyArray D-01 no empty range */
/* cs-deviation: NepaliDateRangeExtendedTests.DatesWithInterval_EmptyRange_YieldsNothing D-01 no empty range */
/* cs-deviation: NepaliDateRangeExtendedTests.WeekendDays_EmptyRange_ReturnsEmpty D-01 no empty range */
/* cs-deviation: NepaliDateRangeExtendedTests.WorkingDays_EmptyRange_ReturnsEmpty D-01 no empty range */
/* cs-deviation: NepaliDateRangeExtendedTests.Union_LeftRangeIsEmpty_ReturnsRightRange D-01 no empty range */
/* cs-deviation: NepaliDateRangeExtendedTests.Union_RightRangeIsEmpty_ReturnsLeftRange D-01 no empty range */
/* cs-deviation: NepaliDateRangeExtendedTests.IsAdjacentTo_EmptyRange_ReturnsFalse D-01 no empty range */

TEST(split_quarter_within_one) {
    const nepdate_range range = make_range(bs(2080, 4, 5), bs(2080, 5, 10));
    nepdate_range parts[8] = {{{0}, {0}}};
    size_t written = 0;
    CHECK_EQ_INT(nepdate_range_split_by_fiscal_quarter(range, parts, 8, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 1);
    expect(parts[0].start, 2080, 4, 5);
    expect(parts[0].end, 2080, 5, 10);
}

TEST(split_quarter_two) {
    const nepdate_range range = make_range(bs(2080, 6, 25), bs(2080, 7, 5));
    nepdate_range parts[8] = {{{0}, {0}}};
    size_t written = 0;
    CHECK_EQ_INT(nepdate_range_split_by_fiscal_quarter(range, parts, 8, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 2);
    expect(parts[0].start, 2080, 6, 25);
    expect(parts[0].end, 2080, 6, 30);
    expect(parts[1].start, 2080, 7, 1);
    expect(parts[1].end, 2080, 7, 5);
}

TEST(split_quarter_full_fiscal_year) {
    nepdate_range year = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_fiscal_year(2080, &year), NEPDATE_OK);
    nepdate_range parts[8] = {{{0}, {0}}};
    size_t written = 0;
    CHECK_EQ_INT(nepdate_range_split_by_fiscal_quarter(year, parts, 8, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 4);
    expect(parts[0].start, 2080, 4, 1);
    expect(parts[0].end, 2080, 6, 30);
    expect(parts[1].start, 2080, 7, 1);
    expect(parts[2].start, 2080, 10, 1);
    expect(parts[3].start, 2081, 1, 1);
    CHECK_EQ_INT(parts[3].end.value, year.end.value);
}

TEST(split_quarter_in_q4) {
    const nepdate_range range = make_range(bs(2081, 1, 10), bs(2081, 2, 20));
    nepdate_range parts[8] = {{{0}, {0}}};
    size_t written = 0;
    CHECK_EQ_INT(nepdate_range_split_by_fiscal_quarter(range, parts, 8, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 1);
    expect(parts[0].start, 2081, 1, 10);
    expect(parts[0].end, 2081, 2, 20);
}

TEST(interval_zero) {
    nepdate_range_day_cursor cursor = {0, 0, 0, 0};
    CHECK_EQ_INT(
        nepdate_range_dates_with_interval(make_range(bs(2080, 5, 1), bs(2080, 5, 5)), 0, &cursor),
        NEPDATE_E_OUT_OF_RANGE);
}

TEST(interval_negative) {
    nepdate_range_day_cursor cursor = {0, 0, 0, 0};
    CHECK_EQ_INT(
        nepdate_range_dates_with_interval(make_range(bs(2080, 5, 1), bs(2080, 5, 5)), -1, &cursor),
        NEPDATE_E_OUT_OF_RANGE);
}

TEST(interval_one) {
    const nepdate_range range = make_range(bs(2080, 5, 1), bs(2080, 5, 5));
    nepdate_range_day_cursor cursor = {0, 0, 0, 0};
    CHECK_EQ_INT(nepdate_range_dates_with_interval(range, 1, &cursor), NEPDATE_OK);
    int count = 0;
    nepdate_date date = {0};
    while (nepdate_range_day_next(&cursor, &date)) {
        expect(date, 2080, 5, 1 + count);
        count++;
    }
    CHECK_EQ_INT(count, 5);
}

TEST(interval_two) {
    const nepdate_range range = make_range(bs(2080, 5, 1), bs(2080, 5, 8));
    nepdate_range_day_cursor cursor = {0, 0, 0, 0};
    CHECK_EQ_INT(nepdate_range_dates_with_interval(range, 2, &cursor), NEPDATE_OK);
    int count = 0;
    nepdate_date date = {0};
    while (nepdate_range_day_next(&cursor, &date)) {
        expect(date, 2080, 5, 1 + 2 * count);
        count++;
    }
    CHECK_EQ_INT(count, 4);
}

TEST(interval_larger_than_range) {
    const nepdate_range range = make_range(bs(2080, 5, 1), bs(2080, 5, 5));
    nepdate_range_day_cursor cursor = {0, 0, 0, 0};
    CHECK_EQ_INT(nepdate_range_dates_with_interval(range, 10, &cursor), NEPDATE_OK);
    nepdate_date date = {0};
    CHECK(nepdate_range_day_next(&cursor, &date));
    expect(date, 2080, 5, 1);
    CHECK(!nepdate_range_day_next(&cursor, &date));
}

TEST(weekends_both_days) {
    const nepdate_range range = make_range(bs(2081, 4, 13), bs(2081, 4, 21));
    nepdate_range_day_cursor cursor = {0, 0, 0, 0};
    CHECK_EQ_INT(nepdate_range_weekend_days(range, true, &cursor), NEPDATE_OK);
    int count = 0;
    nepdate_date date = {0};
    while (nepdate_range_day_next(&cursor, &date)) {
        count++;
    }
    CHECK_EQ_INT(count, 3);
}

TEST(weekends_saturday_only) {
    const nepdate_range range = make_range(bs(2081, 4, 13), bs(2081, 4, 21));
    nepdate_range_day_cursor cursor = {0, 0, 0, 0};
    CHECK_EQ_INT(nepdate_range_weekend_days(range, false, &cursor), NEPDATE_OK);
    int count = 0;
    nepdate_date date = {0};
    while (nepdate_range_day_next(&cursor, &date)) {
        nepdate_weekday weekday = NEPDATE_SUNDAY;
        CHECK_EQ_INT(nepdate_weekday_of(date, &weekday), NEPDATE_OK);
        CHECK_EQ_INT(weekday, NEPDATE_SATURDAY);
        count++;
    }
    CHECK_EQ_INT(count, 1);
}

TEST(weekends_none_in_range) {
    /* Monday to Friday of the week that starts on Sunday 2081-04-13. */
    const nepdate_range range = make_range(bs(2081, 4, 14), bs(2081, 4, 18));
    nepdate_range_day_cursor cursor = {0, 0, 0, 0};
    CHECK_EQ_INT(nepdate_range_weekend_days(range, true, &cursor), NEPDATE_OK);
    nepdate_date date = {0};
    CHECK(!nepdate_range_day_next(&cursor, &date));
}

TEST(working_days_saturday_only) {
    const nepdate_range range = make_range(bs(2081, 4, 13), bs(2081, 4, 21));
    nepdate_range_day_cursor cursor = {0, 0, 0, 0};
    CHECK_EQ_INT(nepdate_range_working_days(range, false, &cursor), NEPDATE_OK);
    int count = 0;
    nepdate_date date = {0};
    while (nepdate_range_day_next(&cursor, &date)) {
        nepdate_weekday weekday = NEPDATE_SUNDAY;
        CHECK_EQ_INT(nepdate_weekday_of(date, &weekday), NEPDATE_OK);
        CHECK(weekday != NEPDATE_SATURDAY);
        count++;
    }
    CHECK_EQ_INT(count, 8);
}

TEST(working_days_both_weekend_days) {
    const nepdate_range range = make_range(bs(2081, 4, 13), bs(2081, 4, 21));
    nepdate_range_day_cursor cursor = {0, 0, 0, 0};
    CHECK_EQ_INT(nepdate_range_working_days(range, true, &cursor), NEPDATE_OK);
    int count = 0;
    nepdate_date date = {0};
    while (nepdate_range_day_next(&cursor, &date)) {
        nepdate_weekday weekday = NEPDATE_SATURDAY;
        CHECK_EQ_INT(nepdate_weekday_of(date, &weekday), NEPDATE_OK);
        CHECK(weekday != NEPDATE_SATURDAY && weekday != NEPDATE_SUNDAY);
        count++;
    }
    CHECK_EQ_INT(count, 6);
}

TEST(from_day_count_zero) {
    const nepdate_date start = bs(2080, 5, 1);
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_make(start, add_days(start, -1), &range), NEPDATE_E_OUT_OF_RANGE);
}

TEST(from_day_count_negative) {
    const nepdate_date start = bs(2080, 5, 1);
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_make(start, add_days(start, -6), &range), NEPDATE_E_OUT_OF_RANGE);
}

TEST(intersect_disjoint) {
    const nepdate_range first = make_range(bs(2080, 1, 1), bs(2080, 1, 10));
    const nepdate_range second = make_range(bs(2080, 2, 1), bs(2080, 2, 10));
    nepdate_range intersection = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_intersect(first, second, &intersection), NEPDATE_E_NO_OVERLAP);
}

TEST(intersect_adjacent) {
    const nepdate_range first = make_range(bs(2080, 1, 1), bs(2080, 1, 10));
    const nepdate_range second = make_range(bs(2080, 1, 11), bs(2080, 1, 20));
    nepdate_range intersection = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_intersect(first, second, &intersection), NEPDATE_E_NO_OVERLAP);
}

TEST(intersect_inside) {
    const nepdate_range outer = make_range(bs(2080, 1, 1), bs(2080, 1, 31));
    const nepdate_range inner = make_range(bs(2080, 1, 10), bs(2080, 1, 20));
    nepdate_range intersection = {{0}, {0}};
    /* The outer range is deliberately the first argument; tidy would guess the two range
     * arguments might be swapped. */
    // NOLINTNEXTLINE(readability-suspicious-call-argument)
    CHECK_EQ_INT(nepdate_range_intersect(outer, inner, &intersection), NEPDATE_OK);
    expect(intersection.start, 2080, 1, 10);
    expect(intersection.end, 2080, 1, 20);
}

TEST(union_spans_gap) {
    const nepdate_range first = make_range(bs(2080, 1, 1), bs(2080, 1, 10));
    const nepdate_range second = make_range(bs(2080, 1, 20), bs(2080, 1, 25));
    nepdate_range combined = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_union(first, second, &combined), NEPDATE_OK);
    expect(combined.start, 2080, 1, 1);
    expect(combined.end, 2080, 1, 25);
}

TEST(contains_exact_start) {
    bool contains = false;
    CHECK_EQ_INT(nepdate_range_contains(make_range(bs(2080, 1, 5), bs(2080, 1, 20)), bs(2080, 1, 5),
                                        &contains),
                 NEPDATE_OK);
    CHECK(contains);
}

TEST(contains_exact_end) {
    bool contains = false;
    CHECK_EQ_INT(nepdate_range_contains(make_range(bs(2080, 1, 5), bs(2080, 1, 20)),
                                        bs(2080, 1, 20), &contains),
                 NEPDATE_OK);
    CHECK(contains);
}

TEST(contains_before_start) {
    bool contains = true;
    CHECK_EQ_INT(nepdate_range_contains(make_range(bs(2080, 1, 5), bs(2080, 1, 20)), bs(2080, 1, 4),
                                        &contains),
                 NEPDATE_OK);
    CHECK(!contains);
}

TEST(contains_after_end) {
    bool contains = true;
    CHECK_EQ_INT(nepdate_range_contains(make_range(bs(2080, 1, 5), bs(2080, 1, 20)),
                                        bs(2080, 1, 21), &contains),
                 NEPDATE_OK);
    CHECK(!contains);
}

TEST(contains_empty_range) {
    nepdate_range range = {{0}, {0}};
    const nepdate_date start = bs(2080, 5, 10);
    CHECK_EQ_INT(nepdate_range_make(start, add_days(start, -1), &range), NEPDATE_E_OUT_OF_RANGE);
}

TEST(adjacent_gap_between) {
    const nepdate_range first = make_range(bs(2080, 1, 1), bs(2080, 1, 10));
    const nepdate_range second = make_range(bs(2080, 1, 12), bs(2080, 1, 20));
    bool adjacent = true;
    CHECK_EQ_INT(nepdate_range_is_adjacent_to(first, second, &adjacent), NEPDATE_OK);
    CHECK(!adjacent);
}

TEST(adjacent_overlapping) {
    const nepdate_range first = make_range(bs(2080, 1, 1), bs(2080, 1, 15));
    const nepdate_range second = make_range(bs(2080, 1, 10), bs(2080, 1, 20));
    bool adjacent = true;
    CHECK_EQ_INT(nepdate_range_is_adjacent_to(first, second, &adjacent), NEPDATE_OK);
    CHECK(!adjacent);
}

TEST(except_disjoint) {
    const nepdate_range range = make_range(bs(2080, 1, 1), bs(2080, 1, 15));
    const nepdate_range other = make_range(bs(2080, 2, 1), bs(2080, 2, 10));
    nepdate_range pieces[2] = {{{0}, {0}}, {{0}, {0}}};
    size_t written = 0;
    CHECK_EQ_INT(nepdate_range_except(range, other, pieces, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 1);
    expect(pieces[0].start, 2080, 1, 1);
    expect(pieces[0].end, 2080, 1, 15);
}

TEST(except_covered) {
    const nepdate_range range = make_range(bs(2080, 1, 5), bs(2080, 1, 15));
    const nepdate_range other = make_range(bs(2080, 1, 1), bs(2080, 1, 31));
    nepdate_range pieces[2] = {{{0}, {0}}, {{0}, {0}}};
    size_t written = 99;
    CHECK_EQ_INT(nepdate_range_except(range, other, pieces, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 0);
}

TEST(except_overlaps_start) {
    const nepdate_range range = make_range(bs(2080, 1, 5), bs(2080, 1, 15));
    const nepdate_range other = make_range(bs(2080, 1, 1), bs(2080, 1, 10));
    nepdate_range pieces[2] = {{{0}, {0}}, {{0}, {0}}};
    size_t written = 0;
    CHECK_EQ_INT(nepdate_range_except(range, other, pieces, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 1);
    expect(pieces[0].start, 2080, 1, 11);
    expect(pieces[0].end, 2080, 1, 15);
}

TEST(except_overlaps_end) {
    const nepdate_range range = make_range(bs(2080, 1, 5), bs(2080, 1, 15));
    const nepdate_range other = make_range(bs(2080, 1, 10), bs(2080, 1, 31));
    nepdate_range pieces[2] = {{{0}, {0}}, {{0}, {0}}};
    size_t written = 0;
    CHECK_EQ_INT(nepdate_range_except(range, other, pieces, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 1);
    expect(pieces[0].start, 2080, 1, 5);
    expect(pieces[0].end, 2080, 1, 9);
}

TEST(enumerator_three_days) {
    nepdate_range range = make_range(bs(2080, 5, 13), bs(2080, 5, 15));
    nepdate_range_iterator iterator = {0, 0};
    CHECK_EQ_INT(nepdate_range_begin(range, &iterator), NEPDATE_OK);
    nepdate_date date = {0};
    int count = 0;
    while (nepdate_range_next(&iterator, &date)) {
        expect(date, 2080, 5, 13 + count);
        count++;
    }
    CHECK_EQ_INT(count, 3);
}

TEST(enumerator_empty) {
    nepdate_range range = {{0}, {0}};
    const nepdate_date start = bs(2080, 5, 10);
    CHECK_EQ_INT(nepdate_range_make(start, add_days(start, -1), &range), NEPDATE_E_OUT_OF_RANGE);
}

TEST(enumerator_single) {
    nepdate_range range = make_range(bs(2080, 5, 15), bs(2080, 5, 15));
    nepdate_range_iterator iterator = {0, 0};
    CHECK_EQ_INT(nepdate_range_begin(range, &iterator), NEPDATE_OK);
    nepdate_date date = {0};
    int count = 0;
    while (nepdate_range_next(&iterator, &date)) {
        expect(date, 2080, 5, 15);
        count++;
    }
    CHECK_EQ_INT(count, 1);
}

/* cs-deviation: NepaliDateRangeExtendedTests.ToString_EmptyRange_ReturnsEmptyRangeString D-01 no empty range */
/* cs-deviation: NepaliDateRangeExtendedTests.ToString_WithDateFormat_EmptyRange_ReturnsEmptyRangeString D-01 no empty range */

TEST(range_to_string_default) {
    const nepdate_range range = make_range(bs(2080, 1, 1), bs(2080, 1, 15));
    char text[64];
    const size_t length = nepdate_range_format(range, text, sizeof text);
    CHECK_EQ_INT(length, 23);
    CHECK_EQ_STR(text, "2080/01/01 - 2080/01/15");
}

TEST(range_to_string_order_and_separator) {
    const nepdate_range range = make_range(bs(2080, 1, 1), bs(2080, 1, 15));
    char text[64];
    const size_t length = nepdate_range_format_short(range, NEPDATE_DMY, NEPDATE_SEP_DASH,
                                                     NEPDATE_ENGLISH, text, sizeof text);
    CHECK_EQ_INT(length, 23);
    CHECK_EQ_STR(text, "01-01-2080 - 15-01-2080");
}

TEST(equals_identical) {
    CHECK(ranges_equal(make_range(bs(2080, 1, 1), bs(2080, 1, 15)),
                       make_range(bs(2080, 1, 1), bs(2080, 1, 15))));
}

TEST(equals_different_start) {
    CHECK(!ranges_equal(make_range(bs(2080, 1, 1), bs(2080, 1, 15)),
                        make_range(bs(2080, 1, 2), bs(2080, 1, 15))));
}

TEST(equals_different_end) {
    CHECK(!ranges_equal(make_range(bs(2080, 1, 1), bs(2080, 1, 15)),
                        make_range(bs(2080, 1, 1), bs(2080, 1, 16))));
}

TEST(equals_both_empty) {
    nepdate_range first = {{0}, {0}};
    nepdate_range second = {{0}, {0}};
    const nepdate_date start = bs(2080, 5, 10);
    CHECK_EQ_INT(nepdate_range_make(start, add_days(start, -1), &first), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(nepdate_range_make(start, add_days(start, -1), &second), NEPDATE_E_OUT_OF_RANGE);
    CHECK(ranges_equal(first, second));
}

TEST(equals_object_overload) {
    CHECK(ranges_equal(make_range(bs(2080, 1, 1), bs(2080, 1, 15)),
                       make_range(bs(2080, 1, 1), bs(2080, 1, 15))));
}

/* cs-n/a: NepaliDateRangeExtendedTests.Equals_ObjectOverload_WrongType_ReturnsFalse C has no object overload */
/* cs-n/a: NepaliDateRangeExtendedTests.GetHashCode_EqualRanges_SameHash C has no hash code */

TEST(operator_equals) {
    CHECK(ranges_equal(make_range(bs(2080, 1, 1), bs(2080, 1, 15)),
                       make_range(bs(2080, 1, 1), bs(2080, 1, 15))));
}

TEST(operator_not_equals) {
    CHECK(!ranges_equal(make_range(bs(2080, 1, 1), bs(2080, 1, 15)),
                        make_range(bs(2080, 1, 1), bs(2080, 1, 16))));
}

/* ---------- Core/NepaliDateRangeTests.cs ---------- */

TEST(range_constructor) {
    nepdate_range range = make_range(bs(2080, 1, 1), bs(2080, 1, 15));
    expect(range.start, 2080, 1, 1);
    expect(range.end, 2080, 1, 15);
    int length = 0;
    CHECK_EQ_INT(nepdate_range_length(range, &length), NEPDATE_OK);
    CHECK_EQ_INT(length, 15);
}

TEST(range_constructor_reversed) {
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_make(bs(2080, 1, 15), bs(2080, 1, 1), &range),
                 NEPDATE_E_OUT_OF_RANGE);
}

TEST(range_single_day) {
    const nepdate_date date = bs(2080, 1, 1);
    nepdate_range range = make_range(date, date);
    int length = 0;
    CHECK_EQ_INT(nepdate_range_length(range, &length), NEPDATE_OK);
    CHECK_EQ_INT(length, 1);
}

TEST(range_from_day_count) {
    const nepdate_date start = bs(2080, 1, 1);
    nepdate_range range = make_range(start, add_days(start, 9));
    int length = 0;
    CHECK_EQ_INT(nepdate_range_length(range, &length), NEPDATE_OK);
    CHECK_EQ_INT(length, 10);
}

TEST(range_for_month) {
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_month(2080, 1, &range), NEPDATE_OK);
    expect(range.start, 2080, 1, 1);
    expect(range.end, 2080, 1, 31);
}

TEST(range_for_fiscal_year) {
    nepdate_range range = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_of_fiscal_year(2080, &range), NEPDATE_OK);
    expect(range.start, 2080, 4, 1);
    expect(range.end, 2081, 3, 31);
}

TEST(range_contains_inside) {
    bool contains = false;
    CHECK_EQ_INT(nepdate_range_contains(make_range(bs(2080, 1, 1), bs(2080, 1, 15)),
                                        bs(2080, 1, 10), &contains),
                 NEPDATE_OK);
    CHECK(contains);
}

TEST(range_contains_outside) {
    bool contains = true;
    CHECK_EQ_INT(nepdate_range_contains(make_range(bs(2080, 1, 1), bs(2080, 1, 15)),
                                        bs(2080, 1, 20), &contains),
                 NEPDATE_OK);
    CHECK(!contains);
}

TEST(contains_range_inside) {
    const nepdate_range outer = make_range(bs(2080, 1, 1), bs(2080, 1, 31));
    const nepdate_range inner = make_range(bs(2080, 1, 5), bs(2080, 1, 20));
    bool contains = false;
    /* The outer range is deliberately the first argument; tidy would guess the two range
     * arguments might be swapped. */
    // NOLINTNEXTLINE(readability-suspicious-call-argument)
    CHECK_EQ_INT(nepdate_range_contains_range(outer, inner, &contains), NEPDATE_OK);
    CHECK(contains);
    CHECK_EQ_INT(nepdate_range_contains_range(inner, outer, &contains), NEPDATE_OK);
    CHECK(!contains);
}

TEST(overlaps_yes) {
    const nepdate_range first = make_range(bs(2080, 1, 1), bs(2080, 1, 15));
    const nepdate_range second = make_range(bs(2080, 1, 10), bs(2080, 1, 20));
    bool overlaps = false;
    CHECK_EQ_INT(nepdate_range_overlaps(first, second, &overlaps), NEPDATE_OK);
    CHECK(overlaps);
}

TEST(overlaps_no) {
    const nepdate_range first = make_range(bs(2080, 1, 1), bs(2080, 1, 15));
    const nepdate_range second = make_range(bs(2080, 1, 16), bs(2080, 1, 20));
    bool overlaps = true;
    CHECK_EQ_INT(nepdate_range_overlaps(first, second, &overlaps), NEPDATE_OK);
    CHECK(!overlaps);
}

TEST(adjacent_yes) {
    const nepdate_range first = make_range(bs(2080, 1, 1), bs(2080, 1, 15));
    const nepdate_range second = make_range(bs(2080, 1, 16), bs(2080, 1, 20));
    bool adjacent = false;
    CHECK_EQ_INT(nepdate_range_is_adjacent_to(first, second, &adjacent), NEPDATE_OK);
    CHECK(adjacent);
    CHECK_EQ_INT(nepdate_range_is_adjacent_to(second, first, &adjacent), NEPDATE_OK);
    CHECK(adjacent);
}

TEST(intersect_overlapping) {
    const nepdate_range first = make_range(bs(2080, 1, 1), bs(2080, 1, 15));
    const nepdate_range second = make_range(bs(2080, 1, 10), bs(2080, 1, 20));
    nepdate_range intersection = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_intersect(first, second, &intersection), NEPDATE_OK);
    expect(intersection.start, 2080, 1, 10);
    expect(intersection.end, 2080, 1, 15);
}

TEST(union_overlapping) {
    const nepdate_range first = make_range(bs(2080, 1, 1), bs(2080, 1, 15));
    const nepdate_range second = make_range(bs(2080, 1, 10), bs(2080, 1, 20));
    nepdate_range combined = {{0}, {0}};
    CHECK_EQ_INT(nepdate_range_union(first, second, &combined), NEPDATE_OK);
    expect(combined.start, 2080, 1, 1);
    expect(combined.end, 2080, 1, 20);
}

TEST(except_from_middle) {
    const nepdate_range range = make_range(bs(2080, 1, 1), bs(2080, 1, 15));
    const nepdate_range other = make_range(bs(2080, 1, 6), bs(2080, 1, 10));
    nepdate_range pieces[2] = {{{0}, {0}}, {{0}, {0}}};
    size_t written = 0;
    CHECK_EQ_INT(nepdate_range_except(range, other, pieces, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 2);
    expect(pieces[0].start, 2080, 1, 1);
    expect(pieces[0].end, 2080, 1, 5);
    expect(pieces[1].start, 2080, 1, 11);
    expect(pieces[1].end, 2080, 1, 15);
}

TEST(split_month_across_months) {
    /* Parts of Baishakh, Jestha and Ashad. */
    const nepdate_range range = make_range(bs(2080, 1, 20), bs(2080, 3, 10));
    nepdate_range parts[8] = {{{0}, {0}}};
    size_t written = 0;
    CHECK_EQ_INT(nepdate_range_split_by_month(range, parts, 8, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 3);
    expect(parts[0].start, 2080, 1, 20);
    expect(parts[0].end, 2080, 1, 31);
    expect(parts[1].start, 2080, 2, 1);
    expect(parts[1].end, 2080, 2, 32);
    expect(parts[2].start, 2080, 3, 1);
    expect(parts[2].end, 2080, 3, 10);
}

TEST(working_days_in_fourteen) {
    const nepdate_range range = make_range(bs(2081, 4, 13), bs(2081, 4, 26));
    int weekend_days = 0;
    nepdate_range_iterator iterator = {0, 0};
    CHECK_EQ_INT(nepdate_range_begin(range, &iterator), NEPDATE_OK);
    nepdate_date date = {0};
    int total = 0;
    while (nepdate_range_next(&iterator, &date)) {
        nepdate_weekday weekday = NEPDATE_SUNDAY;
        CHECK_EQ_INT(nepdate_weekday_of(date, &weekday), NEPDATE_OK);
        if (weekday == NEPDATE_SATURDAY) {
            weekend_days++;
        }
        total++;
    }
    CHECK_EQ_INT(total, 14);
    nepdate_range_day_cursor cursor = {0, 0, 0, 0};
    CHECK_EQ_INT(nepdate_range_working_days(range, false, &cursor), NEPDATE_OK);
    int working_days = 0;
    while (nepdate_range_day_next(&cursor, &date)) {
        working_days++;
    }
    CHECK_EQ_INT(working_days, total - weekend_days);
}

TEST(range_enumeration) {
    nepdate_range range = make_range(bs(2080, 1, 1), bs(2080, 1, 10));
    nepdate_range_iterator iterator = {0, 0};
    CHECK_EQ_INT(nepdate_range_begin(range, &iterator), NEPDATE_OK);
    nepdate_date date = {0};
    int count = 0;
    while (nepdate_range_next(&iterator, &date)) {
        expect(date, 2080, 1, 1 + count);
        count++;
    }
    CHECK_EQ_INT(count, 10);
}

TEST(demo_range_operations) {
    /* The operations work together: the month pieces cover the range without a gap or an overlap,
     * and subtracting a piece from the range leaves the rest in order. */
    const nepdate_range range = make_range(bs(2080, 4, 15), bs(2080, 6, 10));
    nepdate_range parts[16] = {{{0}, {0}}};
    size_t pieces_count = 0;
    CHECK_EQ_INT(nepdate_range_split_by_month(range, parts, 16, &pieces_count), NEPDATE_OK);
    CHECK_EQ_INT(pieces_count, 3);
    CHECK_EQ_INT(parts[0].start.value, range.start.value);
    CHECK_EQ_INT(parts[pieces_count - 1].end.value, range.end.value);
    for (size_t i = 0; i + 1 < pieces_count; i++) {
        CHECK_EQ_INT(parts[i].end.value + 1, parts[i + 1].start.value);
    }
    bool contained = false;
    // NOLINTNEXTLINE(readability-suspicious-call-argument)
    CHECK_EQ_INT(nepdate_range_contains_range(range, parts[1], &contained), NEPDATE_OK);
    CHECK(contained);

    nepdate_range rest[2] = {{{0}, {0}}, {{0}, {0}}};
    size_t written = 0;
    CHECK_EQ_INT(nepdate_range_except(range, parts[1], rest, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 2);
    CHECK_EQ_INT(rest[0].start.value, range.start.value);
    CHECK_EQ_INT(rest[0].end.value, parts[1].start.value - 1);
    CHECK_EQ_INT(rest[1].start.value, parts[1].end.value + 1);
    CHECK_EQ_INT(rest[1].end.value, range.end.value);

    const nepdate_range next = make_range(bs(2080, 6, 11), bs(2080, 6, 15));
    bool adjacent = true;
    CHECK_EQ_INT(nepdate_range_is_adjacent_to(parts[pieces_count - 1], next, &adjacent),
                 NEPDATE_OK);
    CHECK(adjacent);
}

/* ---------- Core/OptimizationVerificationTests.cs ---------- */

TEST(subtraction_same) {
    int32_t days = -1;
    const nepdate_date date = bs(2080, 5, 15);
    CHECK_EQ_INT(nepdate_diff_days(date, date, &days), NEPDATE_OK);
    CHECK_EQ_INT(days, 0);
}

TEST(subtraction_adjacent) {
    int32_t days = 0;
    CHECK_EQ_INT(nepdate_diff_days(bs(2080, 5, 15), bs(2080, 5, 16), &days), NEPDATE_OK);
    CHECK_EQ_INT(days, 1);
}

TEST(subtraction_cross_month) {
    int32_t days = 0;
    CHECK_EQ_INT(nepdate_diff_days(bs(2080, 4, 32), bs(2080, 5, 1), &days), NEPDATE_OK);
    CHECK_EQ_INT(days, 1);
}

TEST(subtraction_cross_year) {
    int32_t days = 0;
    CHECK_EQ_INT(nepdate_diff_days(bs(2080, 12, 30), bs(2081, 1, 1), &days), NEPDATE_OK);
    CHECK_EQ_INT(days, 1);
}

int main(void) {
    RUN_TEST_NAMED("NepaliDateArithmeticEdgeCaseTests."
                   "TryParse_AutoAdjust_InvalidDateAfterAdjustment_ReturnsFalse",
                   try_parse_auto_adjust_invalid_date);
    RUN_TEST_NAMED("NepaliDateArithmeticEdgeCaseTests."
                   "TryParse_AutoAdjust_ValidStandardFormat_ReturnsCorrectDate",
                   try_parse_auto_adjust_valid_standard);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearStartDate_DateInShrawan_ReturnsSameyearShrawan1",
        fiscal_start_shrawan);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearStartDate_DateInChaitra_ReturnsSameYearShrawan1",
        fiscal_start_chaitra);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearStartDate_DateInBasikhakh_"
                   "ReturnsPreviousYearShrawan1",
                   fiscal_start_baishakh);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearStartDate_DateInAshadh_"
                   "ReturnsPreviousYearShrawan1",
                   fiscal_start_ashad);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearStartDate_WithPositiveYearOffset_ShiftsForward",
        fiscal_start_offset_positive);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearStartDate_WithNegativeYearOffset_ShiftsBackward",
        fiscal_start_offset_negative);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearEndDate_DateInShrawan_ReturnsNextYearAshad",
        fiscal_end_shrawan);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearEndDate_DateInChaitra_ReturnsNextYearAshadh",
        fiscal_end_chaitra);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearEndDate_DateInBasikhakh_ReturnsSameYearAshadh",
        fiscal_end_baishakh);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearEndDate_WithPositiveYearOffset_ShiftsForward",
        fiscal_end_offset_positive);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearStartAndEndDate_DateInQ2_ReturnsBothBoundaries",
        fiscal_both_q2);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearStartAndEndDate_DateInQ4_"
                   "ReturnsPreviousFiscalYearBoundaries",
                   fiscal_both_q4);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_ExplicitFirst_ReturnsShrawan1",
        quarter_start_q1);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_ExplicitSecond_ReturnsKartik1",
        quarter_start_q2);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_ExplicitThird_ReturnsMagh1",
        quarter_start_q3);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_ExplicitFourth_"
                   "ReturnsNextYearBasikhakh1",
                   quarter_start_q4);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_ExplicitFirst_"
                   "ReturnsLastDayOfAshoj",
                   quarter_end_q1);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_ExplicitSecond_"
                   "ReturnsLastDayOfPoush",
                   quarter_end_q2);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_ExplicitThird_"
                   "ReturnsLastDayOfChaitra",
                   quarter_end_q3);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_ExplicitFourth_"
                   "ReturnsNextYearLastDayOfAshadh",
                   quarter_end_q4);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterStartAndEndDate_ExplicitSecond_"
                   "ReturnsBothBoundaries",
                   quarter_both_q2);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterStartAndEndDate_ExplicitThird_"
                   "ReturnsBothBoundaries",
                   quarter_both_q3);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_Current_DateInQ1_ReturnsShrawan1",
        quarter_current_q1_start);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_Current_DateInQ1_"
                   "ReturnsLastDayOfAshoj",
                   quarter_current_q1_end);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_Current_DateInQ2_ReturnsKartik1",
        quarter_current_q2_start);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_Current_DateInQ3_ReturnsMagh1",
        quarter_current_q3_start);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_Current_DateInQ3_"
                   "ReturnsLastDayOfChaitra",
                   quarter_current_q3_end);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_"
                   "ExplicitFirstWithOffset1_ReturnsNextYearShrawan1",
                   quarter_start_q1_offset_positive);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_"
                   "ExplicitFirstWithNegativeOffset_ReturnsPreviousYearShrawan1",
                   quarter_start_q1_offset_negative);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearStartDate_AllFiscalMonths_Return2080Shrawan1#1",
        fiscal_start_all_fiscal_months_1);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearStartDate_AllFiscalMonths_Return2080Shrawan1#2",
        fiscal_start_all_fiscal_months_2);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearStartDate_AllFiscalMonths_Return2080Shrawan1#3",
        fiscal_start_all_fiscal_months_3);
    RUN_TEST_NAMED(
        "FiscalYearInstanceMethodTests.FiscalYearStartDate_AllFiscalMonths_Return2080Shrawan1#4",
        fiscal_start_all_fiscal_months_4);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearStartDate_Q4CalendarMonths_"
                   "ReturnPreviousYearShrawan1#1",
                   fiscal_start_q4_months_1);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearStartDate_Q4CalendarMonths_"
                   "ReturnPreviousYearShrawan1#2",
                   fiscal_start_q4_months_2);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearStartDate_Q4CalendarMonths_"
                   "ReturnPreviousYearShrawan1#3",
                   fiscal_start_q4_months_3);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_FromQ4Date_"
                   "ExplicitFirst_ReturnsFY2080Q1Start",
                   quarter_start_from_q4_q1);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_FromQ4Date_"
                   "ExplicitSecond_ReturnsFY2080Q2Start",
                   quarter_start_from_q4_q2);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_FromQ4Date_"
                   "ExplicitThird_ReturnsFY2080Q3Start",
                   quarter_start_from_q4_q3);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_FromQ4Date_"
                   "ExplicitFourth_ReturnsFY2080Q4Start",
                   quarter_start_from_q4_q4);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_FromQ4Date_Current_"
                   "ReturnsFY2080Q4Start",
                   quarter_start_from_q4_current);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_FromQ4Date_"
                   "ExplicitFirst_ReturnsFY2080Q1End",
                   quarter_end_from_q4_q1);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_FromQ4Date_"
                   "ExplicitThird_ReturnsFY2080Q3End",
                   quarter_end_from_q4_q3);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_FromQ4Date_"
                   "ExplicitFourth_ReturnsFY2080Q4End",
                   quarter_end_from_q4_q4);
    RUN_TEST_NAMED("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_FromQ4Date_Current_"
                   "ReturnsFY2080Q4End",
                   quarter_end_from_q4_current);
    RUN_TEST_NAMED("FiscalYearTests.GetFiscalYearStartAndEndDate_ValidYear_ReturnsCorrectDates",
                   fiscal_tests_start_and_end_2080);
    RUN_TEST_NAMED("FiscalYearTests.GetFiscalYearStartAndEndDate_InvalidYear_ThrowsException#1",
                   fiscal_tests_invalid_1843);
    RUN_TEST_NAMED("FiscalYearTests.GetFiscalYearStartAndEndDate_InvalidYear_ThrowsException#2",
                   fiscal_tests_invalid_2200);
    RUN_TEST_NAMED("FiscalYearTests.GetFiscalYearStartDate_ValidYear_ReturnsCorrectDate",
                   fiscal_tests_start_2080);
    RUN_TEST_NAMED("FiscalYearTests.GetFiscalYearEndDate_ValidYear_ReturnsCorrectDate",
                   fiscal_tests_end_2080);
    RUN_TEST_NAMED(
        "FiscalYearTests.FiscalYearStartAndEndDate_OnNepaliDateInstance_ReturnsCorrectDates",
        fiscal_tests_instance_start_and_end);
    RUN_TEST_NAMED("FiscalYearTests.FiscalYearStartDate_OnNepaliDateInstance_ReturnsCorrectDate",
                   fiscal_tests_instance_start);
    RUN_TEST_NAMED("FiscalYearTests.FiscalYearEndDate_OnNepaliDateInstance_ReturnsCorrectDate",
                   fiscal_tests_instance_end);
    RUN_TEST_NAMED(
        "FiscalYearTests.GetFiscalYearQuarterStartAndEndDate_ValidYearAndMonth_ReturnsCorrectDates",
        fiscal_tests_quarter_start_and_end);
    RUN_TEST_NAMED(
        "FiscalYearTests.GetFiscalYearQuarterStartDate_ValidYearAndMonth_ReturnsCorrectDate",
        fiscal_tests_quarter_start);
    RUN_TEST_NAMED(
        "FiscalYearTests.GetFiscalYearQuarterEndDate_ValidYearAndMonth_ReturnsCorrectDate",
        fiscal_tests_quarter_end);
    RUN_TEST_NAMED("NepaliDateArithmeticEdgeCaseTests.AddDays_Zero_ReturnsSameDate", add_days_zero);
    RUN_TEST_NAMED(
        "NepaliDateArithmeticEdgeCaseTests.AddDays_PositiveAcrossMonthBoundary_IsCorrect",
        add_days_positive_month);
    RUN_TEST_NAMED(
        "NepaliDateArithmeticEdgeCaseTests.AddDays_NegativeAcrossMonthBoundary_IsCorrect",
        add_days_negative_month);
    RUN_TEST_NAMED("NepaliDateArithmeticEdgeCaseTests.AddDays_PositiveAcrossYearBoundary_IsCorrect",
                   add_days_positive_year);
    RUN_TEST_NAMED("NepaliDateArithmeticEdgeCaseTests.AddDays_NegativeAcrossYearBoundary_IsCorrect",
                   add_days_negative_year);
    RUN_TEST_NAMED("NepaliDateArithmeticEdgeCaseTests.AddDays_LargePositiveValue_IsCorrect",
                   add_days_large_positive);
    RUN_TEST_NAMED(
        "NepaliDateArithmeticEdgeCaseTests.AddMonths_PositiveCrossesYearBoundary_IsCorrect",
        add_months_positive_year);
    RUN_TEST_NAMED(
        "NepaliDateArithmeticEdgeCaseTests.AddMonths_NegativeCrossesYearBoundary_IsCorrect",
        add_months_negative_year);
    RUN_TEST_NAMED(
        "NepaliDateArithmeticEdgeCaseTests.AddMonths_PositiveMoreThan12_CrossesMultipleYears",
        add_months_more_than_12);
    RUN_TEST_NAMED("NepaliDateArithmeticEdgeCaseTests.AddMonths_Exactly12_SameMonthNextYear",
                   add_months_exactly_12);
    RUN_TEST_NAMED(
        "NepaliDateArithmeticEdgeCaseTests.AddMonths_DayCappedAtMonthEnd_WhenResultMonthIsShorter",
        add_months_capped);
    RUN_TEST_NAMED("NepaliDateArithmeticEdgeCaseTests.AddMonths_Negative_SameDayPreservedWhenFits",
                   add_months_negative_same_day);
    RUN_TEST_NAMED(
        "NepaliDateArithmeticEdgeCaseTests.Constructor_DateTimeWithMidnightTime_IgnoresTime",
        constructor_midnight_ignores_time);
    RUN_TEST_NAMED("NepaliDateArithmeticEdgeCaseTests.Constructor_DateTimeWithNoonTime_IgnoresTime",
                   constructor_noon_ignores_time);
    RUN_TEST_NAMED(
        "NepaliDateArithmeticEdgeCaseTests.Constructor_DateTimeWithEndOfDayTime_IgnoresTime",
        constructor_end_of_day_ignores_time);
    RUN_TEST_NAMED("NepaliDateArithmeticEdgeCaseTests.Constructor_"
                   "TwoDateTimesOnSameEnglishDateDifferentTimes_ProduceSameNepaliDate",
                   constructor_same_english_date);
    RUN_TEST_NAMED("NepaliDateManipulationTests.MonthEndDate_ReturnsCorrectDate", month_end_date);
    RUN_TEST_NAMED("NepaliDateManipulationTests.AddMonths_ReturnsCorrectDate#1",
                   manipulation_add_months_1);
    RUN_TEST_NAMED("NepaliDateManipulationTests.AddMonths_ReturnsCorrectDate#2",
                   manipulation_add_months_2);
    RUN_TEST_NAMED("NepaliDateManipulationTests.AddMonths_ReturnsCorrectDate#3",
                   manipulation_add_months_3);
    RUN_TEST_NAMED("NepaliDateManipulationTests.AddMonths_ReturnsCorrectDate#4",
                   manipulation_add_months_4);
    RUN_TEST_NAMED("NepaliDateManipulationTests.AddMonths_ReturnsCorrectDate#5",
                   manipulation_add_months_5);
    RUN_TEST_NAMED("NepaliDateManipulationTests.AddDays_ReturnsCorrectDate#1",
                   manipulation_add_days_1);
    RUN_TEST_NAMED("NepaliDateManipulationTests.AddDays_ReturnsCorrectDate#2",
                   manipulation_add_days_2);
    RUN_TEST_NAMED("NepaliDateManipulationTests.AddDays_ReturnsCorrectDate#3",
                   manipulation_add_days_3);
    RUN_TEST_NAMED("NepaliDateManipulationTests.AddDays_ReturnsCorrectDate#4",
                   manipulation_add_days_4);
    RUN_TEST_NAMED("NepaliDateManipulationTests.Subtract_TwoDates_ReturnsCorrectTimeSpan",
                   manipulation_subtract);
    RUN_TEST_NAMED("NepaliDateManipulationTests.Subtract_SameDate_ReturnsZero",
                   manipulation_subtract_same);
    RUN_TEST_NAMED(
        "NepaliDateRangeExtendedTests.ForCalendarYear_StartsAtBaishakhOne_EndsAtChaitraEnd",
        range_calendar_year);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.ForCalendarYear_LengthIsReasonable",
                   range_calendar_year_length);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.CurrentMonth_StartIsFirstDayOfCurrentMonth",
                   current_month_start_is_first_day);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.CurrentMonth_EndIsLastDayOfCurrentMonth",
                   current_month_end_is_last_day);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.CurrentMonth_ContainsToday",
                   current_month_contains_today);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.CurrentFiscalYear_ContainsToday",
                   current_fiscal_year_contains_today);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.CurrentFiscalYear_StartIsShrawan",
                   current_fiscal_year_start_is_shrawan);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.CurrentFiscalYear_EndIsAshadh",
                   current_fiscal_year_end_is_ashadh);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.CurrentCalendarYear_ContainsToday",
                   current_calendar_year_contains_today);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.CurrentCalendarYear_StartIsBaishakh1",
                   current_calendar_year_start_is_baishakh_1);
    RUN_TEST_NAMED(
        "NepaliDateRangeExtendedTests.FromDayCount_ZeroDays_ThrowsArgumentOutOfRangeException",
        from_day_count_zero);
    RUN_TEST_NAMED(
        "NepaliDateRangeExtendedTests.FromDayCount_NegativeDays_ThrowsArgumentOutOfRangeException",
        from_day_count_negative);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Contains_ExactStartDate_ReturnsTrue",
                   contains_exact_start);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Contains_ExactEndDate_ReturnsTrue",
                   contains_exact_end);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Contains_OneDayBeforeStart_ReturnsFalse",
                   contains_before_start);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Contains_OneDayAfterEnd_ReturnsFalse",
                   contains_after_end);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Contains_EmptyRange_ContainsNothing",
                   contains_empty_range);
    RUN_TEST_NAMED(
        "NepaliDateRangeExtendedTests.GetEnumerator_ThreeDayRange_YieldsExactlyThreeDates",
        enumerator_three_days);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.GetEnumerator_EmptyRange_YieldsNoDates",
                   enumerator_empty);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.GetEnumerator_SingleDayRange_YieldsOneDate",
                   enumerator_single);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Equals_IdenticalRanges_ReturnsTrue",
                   equals_identical);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Equals_DifferentStart_ReturnsFalse",
                   equals_different_start);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Equals_DifferentEnd_ReturnsFalse",
                   equals_different_end);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Equals_BothEmpty_ReturnsTrue", equals_both_empty);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Equals_ObjectOverload_SameRange_ReturnsTrue",
                   equals_object_overload);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.OperatorEquals_IdenticalRanges_ReturnsTrue",
                   operator_equals);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.OperatorNotEquals_DifferentRanges_ReturnsTrue",
                   operator_not_equals);
    RUN_TEST_NAMED("NepaliDateRangeTests.Constructor_ValidDates_CreatesRange", range_constructor);
    RUN_TEST_NAMED("NepaliDateRangeTests.Constructor_EndBeforeStart_CreatesEmptyRange",
                   range_constructor_reversed);
    RUN_TEST_NAMED("NepaliDateRangeTests.SingleDay_CreatesRangeWithOneDay", range_single_day);
    RUN_TEST_NAMED("NepaliDateRangeTests.FromDayCount_CreatesCorrectRange", range_from_day_count);
    RUN_TEST_NAMED("NepaliDateRangeTests.ForMonth_CreatesCorrectRange", range_for_month);
    RUN_TEST_NAMED("NepaliDateRangeTests.ForFiscalYear_CreatesCorrectRange", range_for_fiscal_year);
    RUN_TEST_NAMED("NepaliDateRangeTests.Contains_DateInRange_ReturnsTrue", range_contains_inside);
    RUN_TEST_NAMED("NepaliDateRangeTests.Contains_DateOutsideRange_ReturnsFalse",
                   range_contains_outside);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.SplitByFiscalQuarter_RangeWithinOneQuarter_"
                   "ReturnsSingleRange",
                   split_quarter_within_one);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.SplitByFiscalQuarter_RangeSpanningTwoQuarters_"
                   "ReturnsTwoRanges",
                   split_quarter_two);
    RUN_TEST_NAMED(
        "NepaliDateRangeExtendedTests.SplitByFiscalQuarter_FullFiscalYear_ReturnsFourRanges",
        split_quarter_full_fiscal_year);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.SplitByFiscalQuarter_RangeInQ4Months_SingleRange",
                   split_quarter_in_q4);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.DatesWithInterval_Zero_"
                   "ThrowsArgumentOutOfRangeException",
                   interval_zero);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.DatesWithInterval_NegativeInterval_"
                   "ThrowsArgumentOutOfRangeException",
                   interval_negative);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.DatesWithInterval_IntervalOfOne_YieldsEveryDay",
                   interval_one);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.DatesWithInterval_IntervalOfTwo_YieldsOddDays",
                   interval_two);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.DatesWithInterval_IntervalLargerThanRange_"
                   "YieldsOnlyStartDate",
                   interval_larger_than_range);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.WeekendDays_IncludeSundayTrue_"
                   "ReturnsBothSaturdayAndSunday",
                   weekends_both_days);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.WeekendDays_IncludeSundayFalse_"
                   "ReturnsOnlySaturday",
                   weekends_saturday_only);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.WeekendDays_RangeWithNoWeekend_ReturnsEmpty",
                   weekends_none_in_range);
    RUN_TEST_NAMED(
        "NepaliDateRangeExtendedTests.WorkingDays_ExcludeSundayFalse_ExcludesOnlySaturday",
        working_days_saturday_only);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.WorkingDays_ExcludeSundayTrue_"
                   "ExcludesBothSatAndSun",
                   working_days_both_weekend_days);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Intersect_NonOverlappingRanges_"
                   "ReturnsEmptyRange",
                   intersect_disjoint);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Intersect_AdjacentRanges_ReturnsEmptyRange",
                   intersect_adjacent);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Intersect_OneRangeFullyInsideOther_"
                   "ReturnsInnerRange",
                   intersect_inside);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Union_NonOverlappingRanges_SpansEntireGap",
                   union_spans_gap);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.IsAdjacentTo_GapBetweenRanges_ReturnsFalse",
                   adjacent_gap_between);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.IsAdjacentTo_OverlappingRanges_ReturnsFalse",
                   adjacent_overlapping);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Except_NonOverlappingExcludeRange_"
                   "ReturnsOriginalRange",
                   except_disjoint);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Except_ExcludeRangeFullyContainsThis_"
                   "ReturnsEmptyArray",
                   except_covered);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Except_ExcludeOverlapsStart_ReturnsRightPart",
                   except_overlaps_start);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.Except_ExcludeOverlapsEnd_ReturnsLeftPart",
                   except_overlaps_end);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.ToString_DefaultFormat_IsStartDashEnd",
                   range_to_string_default);
    RUN_TEST_NAMED("NepaliDateRangeExtendedTests.ToString_WithDateFormat_UsesSpecifiedFormat",
                   range_to_string_order_and_separator);
    RUN_TEST_NAMED("NepaliDateRangeTests.Contains_RangeFullyContained_ReturnsTrue",
                   contains_range_inside);
    RUN_TEST_NAMED("NepaliDateRangeTests.Overlaps_RangesOverlap_ReturnsTrue", overlaps_yes);
    RUN_TEST_NAMED("NepaliDateRangeTests.Overlaps_RangesDontOverlap_ReturnsFalse", overlaps_no);
    RUN_TEST_NAMED("NepaliDateRangeTests.IsAdjacentTo_AdjacentRanges_ReturnsTrue", adjacent_yes);
    RUN_TEST_NAMED("NepaliDateRangeTests.Intersect_OverlappingRanges_ReturnsIntersection",
                   intersect_overlapping);
    RUN_TEST_NAMED("NepaliDateRangeTests.Union_OverlappingRanges_ReturnsUnion", union_overlapping);
    RUN_TEST_NAMED("NepaliDateRangeTests.Except_RemovingRangeFromMiddle_ReturnsTwoRanges",
                   except_from_middle);
    RUN_TEST_NAMED("NepaliDateRangeTests.SplitByMonth_RangeAcrossMonths_SplitsCorrectly",
                   split_month_across_months);
    RUN_TEST_NAMED("NepaliDateRangeTests.WorkingDays_ReturnsCorrectDays", working_days_in_fourteen);
    RUN_TEST_NAMED("NepaliDateRangeTests.Demo_RangeOperations", demo_range_operations);
    RUN_TEST_NAMED("NepaliDateRangeTests.Enumeration_EnumeratesAllDaysInRange", range_enumeration);
    RUN_TEST_NAMED("OptimizationVerificationTests.Subtraction_SameDate_ReturnsZeroDays",
                   subtraction_same);
    RUN_TEST_NAMED("OptimizationVerificationTests.Subtraction_AdjacentDays_ReturnsOneDay",
                   subtraction_adjacent);
    RUN_TEST_NAMED("OptimizationVerificationTests.Subtraction_CrossMonth_CorrectDays",
                   subtraction_cross_month);
    RUN_TEST_NAMED("OptimizationVerificationTests.Subtraction_CrossYear_CorrectDays",
                   subtraction_cross_year);
    return harness_finish();
}
