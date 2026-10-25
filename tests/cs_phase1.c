/* The phase-1 rows of C# NepDate's own tests (shared/spec/vectors/csharp-tests/catalog.tsv).
 * Each row is a test named after its id, or a cs-deviation / cs-n/a / cs-pending line. Rows that
 * need the parser or today() are skipped with a comment naming the later phase. The CTest test
 * catalog_coverage (tools/check_catalog.cmake) checks that every phase-1 row is here. */
#include "harness.h"
#include "hinnant_reference.h"

#include <nepdate.h>

#include <stddef.h>
#include <stdlib.h>

static nepdate_date date_of(int year, int month, int day) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_bs(year, month, day, &date), NEPDATE_OK);
    return date;
}

static nepdate_status bs_status(int year, int month, int day) {
    nepdate_date date = {0};
    return nepdate_from_bs(year, month, day, &date);
}

static nepdate_status ad_status(int year, int month, int day) {
    nepdate_date date = {0};
    return nepdate_from_ad(year, month, day, &date);
}

static int compare(nepdate_date a, nepdate_date b) {
    int result = 0;
    CHECK_EQ_INT(nepdate_compare(a, b, &result), NEPDATE_OK);
    return result;
}

static nepdate_ymd to_ad(nepdate_date date) {
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_ad(date, &ymd), NEPDATE_OK);
    return ymd;
}

static nepdate_ymd to_bs(nepdate_date date) {
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(date, &ymd), NEPDATE_OK);
    return ymd;
}

static int month_length(nepdate_date date) {
    int length = 0;
    CHECK_EQ_INT(nepdate_month_length(date, &length), NEPDATE_OK);
    return length;
}

static int day_of_year(nepdate_date date) {
    int day = 0;
    CHECK_EQ_INT(nepdate_day_of_year(date, &day), NEPDATE_OK);
    return day;
}

static nepdate_weekday weekday(nepdate_date date) {
    nepdate_weekday day = NEPDATE_SUNDAY;
    CHECK_EQ_INT(nepdate_weekday_of(date, &day), NEPDATE_OK);
    return day;
}

/* The four fixed dates of NepaliDateComparableTests. */
static nepdate_date d1(void) { return date_of(2081, 1, 1); }
static nepdate_date d2(void) { return date_of(2081, 6, 15); }
static nepdate_date d3(void) { return date_of(2082, 1, 1); }

static int order(const void *a, const void *b) {
    const nepdate_date *left = a;
    const nepdate_date *right = b;
    int result = 0;
    (void)nepdate_compare(*left, *right, &result);
    return result;
}

static void check_sorts(void) {
    nepdate_date dates[3] = {{0}, {0}, {0}};
    dates[0] = d3();
    dates[1] = d1();
    dates[2] = d2();
    qsort(dates, 3, sizeof dates[0], order);
    CHECK_EQ_INT(dates[0].value, d1().value);
    CHECK_EQ_INT(dates[1].value, d2().value);
    CHECK_EQ_INT(dates[2].value, d3().value);
}

TEST(compare_object_earlier) { CHECK(compare(d1(), d2()) < 0); }
TEST(compare_object_same) { CHECK_EQ_INT(compare(d1(), date_of(2081, 1, 1)), 0); }
TEST(compare_object_later) { CHECK(compare(d3(), d1()) > 0); }
/* cs-n/a: NepaliDateComparableTests.CompareTo_Object_Null_ReturnsPositive a C date has no null
 * value; a zero-initialised date is invalid (D-01) */
/* cs-n/a: NepaliDateComparableTests.CompareTo_WrongType_ThrowsArgumentException C dates compare
 * with dates only, so the call doesn't compile */
TEST(compare_generic_earlier) { CHECK(compare(d1(), d2()) < 0); }
TEST(compare_generic_same) { CHECK_EQ_INT(compare(d1(), date_of(2081, 1, 1)), 0); }
TEST(compare_generic_later) { CHECK(compare(d3(), d1()) > 0); }
TEST(sorted_set_orders) { check_sorts(); }
TEST(array_sort_orders) { check_sorts(); }
TEST(list_sort_orders) { check_sorts(); }

TEST(all_nepali_months_roundtrip) {
    int failures = 0;
    for (int year = 1901; year <= 2199; year++) {
        for (int month = 1; month <= 12; month++) {
            const nepdate_date date = date_of(year, month, 1);
            const nepdate_ymd ad = to_ad(date);
            nepdate_date back = {0};
            failures += nepdate_from_ad(ad.year, ad.month, ad.day, &back) == NEPDATE_OK &&
                                back.value == date.value
                            ? 0
                            : 1;
        }
    }
    CHECK_EQ_INT(failures, 0);
}

TEST(all_nepali_days_roundtrip) {
    int failures = 0;
    int total = 0;
    for (int year = 1901; year <= 2199; year++) {
        for (int month = 1; month <= 12; month++) {
            const int length = month_length(date_of(year, month, 1));
            for (int day = 1; day <= length; day++) {
                const nepdate_date date = date_of(year, month, day);
                const nepdate_ymd ad = to_ad(date);
                nepdate_date back = {0};
                failures += nepdate_from_ad(ad.year, ad.month, ad.day, &back) == NEPDATE_OK &&
                                    back.value == date.value
                                ? 0
                                : 1;
                total++;
            }
        }
    }
    CHECK_EQ_INT(failures, 0);
    CHECK_EQ_INT(total, 109212);
}

TEST(all_english_days_roundtrip) {
    int failures = 0;
    int total = 0;
    const int first = reference_days_from_civil(1844, 4, 11);
    const int last = reference_days_from_civil(2143, 4, 15);
    for (int days = first; days <= last; days++) {
        const reference_civil english = reference_civil_from_days(days);
        nepdate_date date = {0};
        const nepdate_status status =
            nepdate_from_ad(english.year, english.month, english.day, &date);
        const nepdate_ymd ad = status == NEPDATE_OK ? to_ad(date) : (nepdate_ymd){0, 0, 0};
        failures += status == NEPDATE_OK && ad.year == english.year && ad.month == english.month &&
                            ad.day == english.day
                        ? 0
                        : 1;
        total++;
    }
    CHECK_EQ_INT(failures, 0);
    CHECK_EQ_INT(total, 109212);
}

TEST(all_months_month_end) {
    for (int year = 1901; year <= 2199; year++) {
        for (int month = 1; month <= 12; month++) {
            const int length = month_length(date_of(year, month, 1));
            CHECK(length >= 29 && length <= 32);
        }
    }
}

TEST(consecutive_nepali_days) {
    int violations = 0;
    int previous = reference_days_from_civil(1844, 4, 11) - 1;
    for (int s = 0; s <= 109211; s++) {
        const nepdate_date date = {s + 1};
        const nepdate_ymd ad = to_ad(date);
        const int days = reference_days_from_civil(ad.year, ad.month, ad.day);
        violations += days == previous + 1 ? 0 : 1;
        previous = days;
    }
    CHECK_EQ_INT(violations, 0);
}

TEST(known_reference_1) {
    const nepdate_ymd ad = to_ad(date_of(2080, 1, 1));
    CHECK(ad.year == 2023 && ad.month == 4 && ad.day == 14);
}
TEST(known_reference_2) {
    const nepdate_ymd ad = to_ad(date_of(2000, 1, 1));
    CHECK(ad.year == 1943 && ad.month == 4 && ad.day == 14);
}
TEST(known_reference_3) {
    const nepdate_ymd ad = to_ad(date_of(2081, 1, 1));
    CHECK(ad.year == 2024 && ad.month == 4 && ad.day == 13);
}
TEST(known_reference_4) {
    const nepdate_ymd ad = to_ad(date_of(1901, 1, 1));
    CHECK(ad.year == 1844 && ad.month == 4 && ad.day == 11);
}

TEST(boundary_dates) {
    const nepdate_ymd min = to_ad(date_of(1901, 1, 1));
    const nepdate_ymd max = to_ad(date_of(2199, 12, 30));
    CHECK(min.year == 1844 && min.month == 4 && min.day == 11);
    CHECK(max.year == 2143 && max.month == 4 && max.day == 15);
    CHECK_EQ_INT(to_bs(date_of(1901, 1, 1)).year, 1901);
    CHECK_EQ_INT(to_bs(date_of(2199, 12, 30)).year, 2199);
}

TEST(total_days_in_range) {
    int total = 0;
    for (int year = 1901; year <= 2199; year++) {
        for (int month = 1; month <= 12; month++) {
            total += month_length(date_of(year, month, 1));
        }
    }
    const int span =
        reference_days_from_civil(2143, 4, 15) - reference_days_from_civil(1844, 4, 11) + 1;
    CHECK_EQ_INT(total, span);
    CHECK_EQ_INT(total, 109212);
}

TEST(constructor_valid) {
    const nepdate_date date = date_of(2080, 5, 15);
    const nepdate_ymd ymd = to_bs(date);
    CHECK(ymd.year == 2080 && ymd.month == 5 && ymd.day == 15);
}
TEST(constructor_min) { CHECK_EQ_INT(to_bs(date_of(1901, 1, 1)).year, 1901); }
TEST(constructor_max) {
    const nepdate_ymd ymd = to_bs(date_of(2199, 12, 30));
    CHECK_EQ_INT(ymd.year, 2199);
    CHECK_EQ_INT(ymd.month, 12);
}
TEST(constructor_invalid_month_zero) {
    CHECK_EQ_INT(bs_status(2080, 0, 1), NEPDATE_E_INVALID_MONTH);
}
TEST(constructor_invalid_month_thirteen) {
    CHECK_EQ_INT(bs_status(2080, 13, 1), NEPDATE_E_INVALID_MONTH);
}
TEST(constructor_invalid_day_zero) { CHECK_EQ_INT(bs_status(2080, 1, 0), NEPDATE_E_INVALID_DAY); }
TEST(constructor_invalid_day_thirty_three) {
    CHECK_EQ_INT(bs_status(2080, 1, 33), NEPDATE_E_INVALID_DAY);
}

/* The rows below parse a string. The date they expect is built with the constructor until the
 * parser exists; the check that the string is accepted belongs to the formatting phase. */
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(valid_string_slash) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(valid_string_dash) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(valid_string_dot) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(valid_string_underscore) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(valid_string_backslash) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(valid_string_space) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* cs-deviation: NepaliDateConstructionTests.Constructor_AutoAdjustedFormats_CreatesInstance#1 D-06
 * (autoAdjust guesses the field order) */
/* cs-deviation: NepaliDateConstructionTests.Constructor_AutoAdjustedFormats_CreatesInstance#2 D-06
 * (autoAdjust guesses the field order) */
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(auto_adjusted_plain) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* cs-deviation: NepaliDateConstructionTests.Constructor_AutoAdjustedFormats_CreatesInstance#4 D-06
 * (2-digit years are not expanded) */
/* cs-deviation: NepaliDateConstructionTests.Constructor_AutoAdjust_MonthOverflowWithBoundaryDay_SwapsAsDocumented#1 D-06 (autoAdjust swaps month and day) */
/* cs-deviation: NepaliDateConstructionTests.Constructor_AutoAdjust_MonthOverflowWithBoundaryDay_SwapsAsDocumented#2 D-06 (autoAdjust swaps month and day) */
/* cs-deviation: NepaliDateConstructionTests.TryParse_AutoAdjust_MonthOverflowWithBoundaryDay_SwapsAsDocumented D-06 (autoAdjust swaps month and day) */

/* cs-pending: NepaliDateConstructionTests.Constructor_InvalidStringFormats_ThrowsException#1 phase 3: needs parse */
TEST(invalid_string_empty) { SKIP("phase 3: needs parse"); }
/* cs-pending: NepaliDateConstructionTests.Constructor_InvalidStringFormats_ThrowsException#2 phase 3: needs parse */
TEST(invalid_string_text) { SKIP("phase 3: needs parse"); }
/* cs-pending: NepaliDateConstructionTests.Constructor_InvalidStringFormats_ThrowsException#3 phase 3: needs parse */
TEST(invalid_string_two_groups) { SKIP("phase 3: needs parse"); }
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(valid_format_invalid_date) { CHECK_EQ_INT(bs_status(2080, 5, 40), NEPDATE_E_INVALID_DAY); }
TEST(english_date_converts) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_ad(2023, 8, 30, &date), NEPDATE_OK);
    const nepdate_ymd ymd = to_bs(date);
    CHECK(ymd.year == 2080 && ymd.month == 5 && ymd.day == 13);
}
/* cs-pending: NepaliDateConstructionTests.TryParse_ValidString_ReturnsTrueWithCorrectDate phase 3: needs parse */
TEST(try_parse_valid_string) { SKIP("phase 3: needs parse"); }
/* cs-pending: NepaliDateConstructionTests.TryParse_InvalidString_ReturnsFalseWithDefault#1 phase 3: needs parse */
TEST(try_parse_invalid_empty) { SKIP("phase 3: needs parse"); }
/* cs-pending: NepaliDateConstructionTests.TryParse_InvalidString_ReturnsFalseWithDefault#2 phase 3: needs parse */
TEST(try_parse_invalid_text) { SKIP("phase 3: needs parse"); }
/* cs-pending: NepaliDateConstructionTests.TryParse_InvalidString_ReturnsFalseWithDefault#3 phase 3: needs parse */
TEST(try_parse_invalid_two_groups) { SKIP("phase 3: needs parse"); }

TEST(english_date_conversion) {
    const nepdate_ymd ad = to_ad(date_of(2080, 5, 15));
    CHECK(ad.year == 2023 && ad.month == 9 && ad.day == 1);
}
TEST(day_of_week) { CHECK_EQ_INT(weekday(date_of(2080, 5, 15)), NEPDATE_FRIDAY); }
TEST(month_end_day) { CHECK_EQ_INT(month_length(date_of(2080, 5, 15)), 31); }
TEST(month_name) {
    nepdate_month month = NEPDATE_BAISHAKH;
    CHECK_EQ_INT(nepdate_month_of(date_of(2080, 5, 15), &month), NEPDATE_OK);
    CHECK_EQ_INT(month, NEPDATE_BHADRA);
}
/* cs-pending: NepaliDatePropertiesTests.Today_ReturnsCurrentNepaliDate phase 4: needs today */
TEST(today) { SKIP("phase 4: needs today"); }
TEST(day_of_year_first) { CHECK_EQ_INT(day_of_year(date_of(2080, 1, 1)), 1); }
TEST(day_of_year_shrawan) {
    const int expected = month_length(date_of(2080, 1, 1)) + month_length(date_of(2080, 2, 1)) +
                         month_length(date_of(2080, 3, 1)) + 1;
    CHECK_EQ_INT(day_of_year(date_of(2080, 4, 1)), expected);
}
/* cs-n/a: NepaliDatePropertiesTests.Equals_NullObject_ReturnsFalse a C date has no null value;
 * a zero-initialised date is invalid (D-01) */
/* cs-n/a: NepaliDatePropertiesTests.Equals_DifferentType_ReturnsFalse C dates compare with dates
 * only, so the call doesn't compile */

/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(all_separators_slash) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(all_separators_dash) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(all_separators_dot) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(all_separators_underscore) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(all_separators_backslash) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(all_separators_space) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(all_separators_danda) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(all_separators_pipe) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* cs-pending: OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#1 phase 3: needs parse */
TEST(invalid_strings_empty) { SKIP("phase 3: needs parse"); }
/* cs-pending: OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#2 phase 3: needs parse */
TEST(invalid_strings_text) { SKIP("phase 3: needs parse"); }
/* cs-pending: OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#3 phase 3: needs parse */
TEST(invalid_strings_two_groups) { SKIP("phase 3: needs parse"); }
/* cs-pending: OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#4 phase 3: needs parse */
TEST(invalid_strings_one_group) { SKIP("phase 3: needs parse"); }
/* cs-pending: OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#5 phase 3: needs parse */
TEST(invalid_strings_extra_group) { SKIP("phase 3: needs parse"); }
/* cs-pending: OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#6 phase 3: needs parse */
TEST(invalid_strings_words) { SKIP("phase 3: needs parse"); }
/* cs-n/a: OptimizationVerificationTests.Constructor_NullString_ThrowsException C has no null
 * string; a missing string is a NULL pointer, which the parser will reject */
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(without_leading_zeros_1) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 15)).year, 2080); }
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(without_leading_zeros_2) { CHECK_EQ_INT(to_bs(date_of(2080, 5, 5)).day, 5); }
/* pending phase 3: needs parse; the constructor checks the date until then. */
TEST(without_leading_zeros_3) {
    const nepdate_ymd ymd = to_bs(date_of(2080, 1, 1));
    CHECK(ymd.year == 2080 && ymd.month == 1 && ymd.day == 1);
}

TEST(operator_equality) { CHECK_EQ_INT(compare(date_of(2080, 5, 15), date_of(2080, 5, 15)), 0); }
TEST(operator_inequality) { CHECK(compare(date_of(2080, 5, 15), date_of(2080, 5, 16)) != 0); }
TEST(operator_less_than) {
    const nepdate_date earlier = date_of(2080, 5, 15);
    const nepdate_date later = date_of(2080, 5, 16);
    CHECK(compare(earlier, later) < 0);
    CHECK(!(compare(later, earlier) < 0));
    CHECK(!(compare(earlier, earlier) < 0));
}
TEST(operator_less_than_or_equal) {
    const nepdate_date earlier = date_of(2080, 5, 15);
    const nepdate_date later = date_of(2080, 5, 16);
    CHECK(compare(earlier, later) <= 0);
    CHECK(compare(earlier, earlier) <= 0);
    CHECK(!(compare(later, earlier) <= 0));
}
TEST(operator_greater_than) {
    const nepdate_date earlier = date_of(2080, 5, 15);
    const nepdate_date later = date_of(2080, 5, 16);
    CHECK(compare(later, earlier) > 0);
    CHECK(!(compare(earlier, later) > 0));
    CHECK(!(compare(later, later) > 0));
}
TEST(operator_greater_than_or_equal) {
    const nepdate_date earlier = date_of(2080, 5, 15);
    const nepdate_date later = date_of(2080, 5, 16);
    CHECK(compare(later, earlier) >= 0);
    CHECK(compare(later, later) >= 0);
    CHECK(!(compare(earlier, later) >= 0));
}
TEST(operator_min_max) {
    CHECK(compare(date_of(1901, 1, 1), date_of(2199, 12, 30)) < 0);
    CHECK(compare(date_of(2199, 12, 30), date_of(1901, 1, 1)) > 0);
}
TEST(compare_to_earlier) { CHECK(compare(date_of(2080, 5, 15), date_of(2080, 5, 16)) < 0); }
TEST(compare_to_same) { CHECK_EQ_INT(compare(date_of(2080, 5, 15), date_of(2080, 5, 15)), 0); }
TEST(compare_to_later) { CHECK(compare(date_of(2080, 5, 16), date_of(2080, 5, 15)) > 0); }
TEST(compare_to_different_years) { CHECK(compare(date_of(2080, 1, 1), date_of(2081, 1, 1)) < 0); }
TEST(compare_to_different_months) { CHECK(compare(date_of(2080, 1, 1), date_of(2080, 2, 1)) < 0); }
TEST(equals_same) { CHECK_EQ_INT(compare(date_of(2080, 5, 15), date_of(2080, 5, 15)), 0); }
TEST(equals_different) { CHECK(compare(date_of(2080, 5, 15), date_of(2080, 5, 16)) != 0); }
/* cs-n/a: OptimizationVerificationTests.GetHashCode_EqualDates_SameHashCode C has no GetHashCode;
 * two dates are equal when nepdate_compare returns 0 */
/* cs-n/a: OptimizationVerificationTests.GetHashCode_StableAcrossMultipleCalls C has no
 * GetHashCode; a date is a plain value */
/* cs-deviation: OptimizationVerificationTests.IsLeapYear_ChecksEnglishYearCorrectly#1 D-12
 * (IsLeapYear is not ported; year length instead) */
/* cs-deviation: OptimizationVerificationTests.IsLeapYear_ChecksEnglishYearCorrectly#2 D-12
 * (IsLeapYear is not ported; year length instead) */
/* cs-deviation: OptimizationVerificationTests.IsLeapYear_ChecksEnglishYearCorrectly#3 D-12
 * (IsLeapYear is not ported; year length instead) */

TEST(invalid_ymd_month_zero) { CHECK_EQ_INT(bs_status(2080, 0, 15), NEPDATE_E_INVALID_MONTH); }
TEST(invalid_ymd_month_thirteen) { CHECK_EQ_INT(bs_status(2080, 13, 15), NEPDATE_E_INVALID_MONTH); }
TEST(invalid_ymd_day_zero) { CHECK_EQ_INT(bs_status(2080, 5, 0), NEPDATE_E_INVALID_DAY); }
TEST(invalid_ymd_day_thirty_three) { CHECK_EQ_INT(bs_status(2080, 5, 33), NEPDATE_E_INVALID_DAY); }
TEST(invalid_ymd_year_low) { CHECK_EQ_INT(bs_status(1900, 1, 1), NEPDATE_E_OUT_OF_RANGE); }
TEST(invalid_ymd_year_high) { CHECK_EQ_INT(bs_status(2200, 1, 1), NEPDATE_E_OUT_OF_RANGE); }
TEST(constructor_min_valid) { CHECK_EQ_INT(to_bs(date_of(1901, 1, 1)).year, 1901); }
TEST(constructor_max_valid) { CHECK_EQ_INT(to_bs(date_of(2199, 12, 30)).year, 2199); }
TEST(english_min_roundtrip) {
    const nepdate_date min = date_of(1901, 1, 1);
    const nepdate_ymd ad = to_ad(min);
    nepdate_date back = {0};
    CHECK_EQ_INT(nepdate_from_ad(ad.year, ad.month, ad.day, &back), NEPDATE_OK);
    CHECK_EQ_INT(back.value, min.value);
}
TEST(english_max_roundtrip) {
    const nepdate_date max = date_of(2199, 12, 30);
    const nepdate_ymd ad = to_ad(max);
    nepdate_date back = {0};
    CHECK_EQ_INT(nepdate_from_ad(ad.year, ad.month, ad.day, &back), NEPDATE_OK);
    CHECK_EQ_INT(back.value, max.value);
}
TEST(english_just_before_min) { CHECK_EQ_INT(ad_status(1844, 4, 10), NEPDATE_E_OUT_OF_RANGE); }
TEST(english_just_after_max) { CHECK_EQ_INT(ad_status(2143, 4, 16), NEPDATE_E_OUT_OF_RANGE); }

TEST(month_end_exact) {
    for (int year = 2070; year <= 2090; year++) {
        for (int month = 1; month <= 12; month++) {
            const int length = month_length(date_of(year, month, 1));
            const nepdate_ymd last = to_bs(date_of(year, month, length));
            CHECK_EQ_INT(last.day, length);
            CHECK_EQ_INT(bs_status(year, month, length + 1), NEPDATE_E_INVALID_DAY);
        }
    }
}

TEST(day_of_week_matches_english) {
    for (int year = 1910; year <= 2190; year += 30) {
        for (int month = 1; month <= 12; month += 3) {
            const nepdate_date date = date_of(year, month, 15);
            const nepdate_ymd ad = to_ad(date);
            const int days = reference_days_from_civil(ad.year, ad.month, ad.day);
            const int expected = (days % 7 + 7 + 4) % 7;
            CHECK_EQ_INT(weekday(date), expected);
        }
    }
}

/* cs-deviation: OptimizationVerificationTests.IsDefault_DefaultStruct_ReturnsTrue D-01 (C has no
 * default date; a zero-initialised nepdate_date is invalid) */
/* cs-deviation: OptimizationVerificationTests.IsDefault_ValidDate_ReturnsFalse D-01 (C has no
 * default date; a zero-initialised nepdate_date is invalid) */

int main(void) {
    RUN_TEST_NAMED("NepaliDateComparableTests.CompareTo_Object_Earlier_ReturnsNegative",
                   compare_object_earlier);
    RUN_TEST_NAMED("NepaliDateComparableTests.CompareTo_Object_Same_ReturnsZero",
                   compare_object_same);
    RUN_TEST_NAMED("NepaliDateComparableTests.CompareTo_Object_Later_ReturnsPositive",
                   compare_object_later);
    RUN_TEST_NAMED("NepaliDateComparableTests.CompareTo_Generic_Earlier_ReturnsNegative",
                   compare_generic_earlier);
    RUN_TEST_NAMED("NepaliDateComparableTests.CompareTo_Generic_Same_ReturnsZero",
                   compare_generic_same);
    RUN_TEST_NAMED("NepaliDateComparableTests.CompareTo_Generic_Later_ReturnsPositive",
                   compare_generic_later);
    RUN_TEST_NAMED("NepaliDateComparableTests.SortedSet_OrdersCorrectly", sorted_set_orders);
    RUN_TEST_NAMED("NepaliDateComparableTests.ArraySort_ViaIComparable_OrdersCorrectly",
                   array_sort_orders);
    RUN_TEST_NAMED("NepaliDateComparableTests.List_Sort_OrdersCorrectly", list_sort_orders);
    RUN_TEST_NAMED(
        "DictionaryIntegrityTests.AllNepaliMonths_RoundtripToEnglishAndBack_ProducesOriginalDate",
        all_nepali_months_roundtrip);
    RUN_TEST_NAMED(
        "DictionaryIntegrityTests.AllNepaliDays_RoundtripToEnglishAndBack_ProducesOriginalDate",
        all_nepali_days_roundtrip);
    RUN_TEST_NAMED(
        "DictionaryIntegrityTests.AllEnglishDays_RoundtripToNepaliAndBack_ProducesOriginalDate",
        all_english_days_roundtrip);
    RUN_TEST_NAMED("DictionaryIntegrityTests.AllMonths_MonthEndDay_IsWithinValidRange",
                   all_months_month_end);
    RUN_TEST_NAMED("DictionaryIntegrityTests.ConsecutiveNepaliDays_ProduceConsecutiveEnglishDays",
                   consecutive_nepali_days);
    RUN_TEST_NAMED("DictionaryIntegrityTests.KnownReferenceDates_ConvertCorrectly#1",
                   known_reference_1);
    RUN_TEST_NAMED("DictionaryIntegrityTests.KnownReferenceDates_ConvertCorrectly#2",
                   known_reference_2);
    RUN_TEST_NAMED("DictionaryIntegrityTests.KnownReferenceDates_ConvertCorrectly#3",
                   known_reference_3);
    RUN_TEST_NAMED("DictionaryIntegrityTests.KnownReferenceDates_ConvertCorrectly#4",
                   known_reference_4);
    RUN_TEST_NAMED("DictionaryIntegrityTests.BoundaryDates_ConvertCorrectly", boundary_dates);
    RUN_TEST_NAMED("DictionaryIntegrityTests.TotalDaysInRange_MatchesEnglishDateSpan",
                   total_days_in_range);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_ValidNepaliDate_CreatesInstance",
                   constructor_valid);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_MinValue_CreatesInstance",
                   constructor_min);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_MaxValue_CreatesInstance",
                   constructor_max);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_InvalidNepaliDate_ThrowsException#1",
                   constructor_invalid_month_zero);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_InvalidNepaliDate_ThrowsException#2",
                   constructor_invalid_month_thirteen);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_InvalidNepaliDate_ThrowsException#3",
                   constructor_invalid_day_zero);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_InvalidNepaliDate_ThrowsException#4",
                   constructor_invalid_day_thirty_three);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_ValidStringFormats_CreatesInstance#1",
                   valid_string_slash);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_ValidStringFormats_CreatesInstance#2",
                   valid_string_dash);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_ValidStringFormats_CreatesInstance#3",
                   valid_string_dot);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_ValidStringFormats_CreatesInstance#4",
                   valid_string_underscore);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_ValidStringFormats_CreatesInstance#5",
                   valid_string_backslash);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_ValidStringFormats_CreatesInstance#6",
                   valid_string_space);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_AutoAdjustedFormats_CreatesInstance#3",
                   auto_adjusted_plain);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_InvalidStringFormats_ThrowsException#1",
                   invalid_string_empty);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_InvalidStringFormats_ThrowsException#2",
                   invalid_string_text);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_InvalidStringFormats_ThrowsException#3",
                   invalid_string_two_groups);
    RUN_TEST_NAMED(
        "NepaliDateConstructionTests.Constructor_ValidFormatInvalidDate_ThrowsException#1",
        valid_format_invalid_date);
    RUN_TEST_NAMED("NepaliDateConstructionTests.Constructor_EnglishDate_ConvertsCorrectly",
                   english_date_converts);
    RUN_TEST_NAMED("NepaliDateConstructionTests.TryParse_ValidString_ReturnsTrueWithCorrectDate",
                   try_parse_valid_string);
    RUN_TEST_NAMED("NepaliDateConstructionTests.TryParse_InvalidString_ReturnsFalseWithDefault#1",
                   try_parse_invalid_empty);
    RUN_TEST_NAMED("NepaliDateConstructionTests.TryParse_InvalidString_ReturnsFalseWithDefault#2",
                   try_parse_invalid_text);
    RUN_TEST_NAMED("NepaliDateConstructionTests.TryParse_InvalidString_ReturnsFalseWithDefault#3",
                   try_parse_invalid_two_groups);
    RUN_TEST_NAMED("NepaliDatePropertiesTests.EnglishDate_Conversion_ReturnsCorrectDate",
                   english_date_conversion);
    RUN_TEST_NAMED("NepaliDatePropertiesTests.DayOfWeek_ReturnsCorrectValue", day_of_week);
    RUN_TEST_NAMED("NepaliDatePropertiesTests.MonthEndDay_ReturnsCorrectValue", month_end_day);
    RUN_TEST_NAMED("NepaliDatePropertiesTests.MonthName_ReturnsCorrectEnum", month_name);
    RUN_TEST_NAMED("NepaliDatePropertiesTests.Today_ReturnsCurrentNepaliDate", today);
    RUN_TEST_NAMED("NepaliDatePropertiesTests.DayOfYear_FirstDayOfYear_Returns1",
                   day_of_year_first);
    RUN_TEST_NAMED(
        "NepaliDatePropertiesTests.DayOfYear_FirstDayOfMonth4_EqualsSum_Of_Months1To3_Plus1",
        day_of_year_shrawan);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#1",
                   all_separators_slash);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#2",
                   all_separators_dash);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#3",
                   all_separators_dot);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#4",
                   all_separators_underscore);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#5",
                   all_separators_backslash);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#6",
                   all_separators_space);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#7",
                   all_separators_danda);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#8",
                   all_separators_pipe);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#1",
                   invalid_strings_empty);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#2",
                   invalid_strings_text);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#3",
                   invalid_strings_two_groups);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#4",
                   invalid_strings_one_group);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#5",
                   invalid_strings_extra_group);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#6",
                   invalid_strings_words);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.Constructor_WithoutLeadingZeros_ParsesCorrectly#1",
        without_leading_zeros_1);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.Constructor_WithoutLeadingZeros_ParsesCorrectly#2",
        without_leading_zeros_2);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.Constructor_WithoutLeadingZeros_ParsesCorrectly#3",
        without_leading_zeros_3);
    RUN_TEST_NAMED("OptimizationVerificationTests.Operator_Equality_SameDates_ReturnsTrue",
                   operator_equality);
    RUN_TEST_NAMED("OptimizationVerificationTests.Operator_Inequality_DifferentDates_ReturnsTrue",
                   operator_inequality);
    RUN_TEST_NAMED("OptimizationVerificationTests.Operator_LessThan_EarlierDate_ReturnsTrue",
                   operator_less_than);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.Operator_LessThanOrEqual_SameAndEarlier_ReturnsTrue",
        operator_less_than_or_equal);
    RUN_TEST_NAMED("OptimizationVerificationTests.Operator_GreaterThan_LaterDate_ReturnsTrue",
                   operator_greater_than);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.Operator_GreaterThanOrEqual_SameAndLater_ReturnsTrue",
        operator_greater_than_or_equal);
    RUN_TEST_NAMED("OptimizationVerificationTests.Operator_MinMaxBoundary_CorrectOrdering",
                   operator_min_max);
    RUN_TEST_NAMED("OptimizationVerificationTests.CompareTo_EarlierDate_ReturnsNegative",
                   compare_to_earlier);
    RUN_TEST_NAMED("OptimizationVerificationTests.CompareTo_SameDate_ReturnsZero", compare_to_same);
    RUN_TEST_NAMED("OptimizationVerificationTests.CompareTo_LaterDate_ReturnsPositive",
                   compare_to_later);
    RUN_TEST_NAMED("OptimizationVerificationTests.CompareTo_DifferentYears_CorrectOrdering",
                   compare_to_different_years);
    RUN_TEST_NAMED("OptimizationVerificationTests.CompareTo_DifferentMonths_CorrectOrdering",
                   compare_to_different_months);
    RUN_TEST_NAMED("OptimizationVerificationTests.Equals_SameDates_ReturnsTrue", equals_same);
    RUN_TEST_NAMED("OptimizationVerificationTests.Equals_DifferentDates_ReturnsFalse",
                   equals_different);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.Constructor_InvalidYearMonthDay_ThrowsException#1",
        invalid_ymd_month_zero);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.Constructor_InvalidYearMonthDay_ThrowsException#2",
        invalid_ymd_month_thirteen);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.Constructor_InvalidYearMonthDay_ThrowsException#3",
        invalid_ymd_day_zero);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.Constructor_InvalidYearMonthDay_ThrowsException#4",
        invalid_ymd_day_thirty_three);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.Constructor_InvalidYearMonthDay_ThrowsException#5",
        invalid_ymd_year_low);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.Constructor_InvalidYearMonthDay_ThrowsException#6",
        invalid_ymd_year_high);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_MinValue_IsValid",
                   constructor_min_valid);
    RUN_TEST_NAMED("OptimizationVerificationTests.Constructor_MaxValue_IsValid",
                   constructor_max_valid);
    RUN_TEST_NAMED("OptimizationVerificationTests.EnglishDate_MinValue_ConvertsAndRoundtrips",
                   english_min_roundtrip);
    RUN_TEST_NAMED("OptimizationVerificationTests.EnglishDate_MaxValue_ConvertsAndRoundtrips",
                   english_max_roundtrip);
    RUN_TEST_NAMED("OptimizationVerificationTests.EnglishDate_JustBeforeMin_ThrowsException",
                   english_just_before_min);
    RUN_TEST_NAMED("OptimizationVerificationTests.EnglishDate_JustAfterMax_ThrowsException",
                   english_just_after_max);
    RUN_TEST_NAMED("OptimizationVerificationTests.MonthEndDay_CreatingDateWithExactEndDay_Succeeds",
                   month_end_exact);
    RUN_TEST_NAMED("OptimizationVerificationTests.DayOfWeek_MatchesEnglishDateDayOfWeek",
                   day_of_week_matches_english);
    return harness_finish();
}
