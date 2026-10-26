/* The phase-3 rows of the C# test catalogue. Rows whose C# API is a form or a property test the
 * equivalent public function here; rows a PARITY deviation covers carry a cs-deviation marker. */
#include "harness.h"

#include <nepdate.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

static nepdate_date date_of(int year, int month, int day) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_bs(year, month, day, &date), NEPDATE_OK);
    return date;
}

static void expect_date(nepdate_date date, int year, int month, int day) {
    nepdate_ymd ymd = {0, 0, 0};
    CHECK_EQ_INT(nepdate_to_bs(date, &ymd), NEPDATE_OK);
    CHECK_EQ_INT(ymd.year, year);
    CHECK_EQ_INT(ymd.month, month);
    CHECK_EQ_INT(ymd.day, day);
}

static size_t format(nepdate_date date, const char *pattern, nepdate_lang lang, char *buffer,
                     size_t size) {
    return nepdate_format(date, pattern, lang, buffer, size);
}

static void expect_format(nepdate_date date, const char *pattern, nepdate_lang lang,
                          const char *expected) {
    char buffer[512];
    const size_t length = format(date, pattern, lang, buffer, sizeof buffer);
    CHECK_EQ_INT(length, strlen(expected));
    CHECK_EQ_STR(buffer, expected);
}

static void check_strict(const char *text, int year, int month, int day) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_parse(text, strlen(text), &date), NEPDATE_OK);
    expect_date(date, year, month, day);
}

static void check_lenient(const char *text, int year, int month, int day) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_parse_lenient(text, strlen(text), &date), NEPDATE_OK);
    expect_date(date, year, month, day);
}

static void check_lenient_error(const char *text, nepdate_status status) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_parse_lenient(text, strlen(text), &date), status);
}

/* Counts UTF-8 code points, the C# string length of the same text. */
static size_t code_points(const char *text) {
    size_t count = 0;
    for (size_t i = 0; text[i] != '\0'; i++) {
        if (((unsigned char)text[i] & 0xC0U) != 0x80U) {
            count++;
        }
    }
    return count;
}

/* ---------- NepaliDateFormattableTests ---------- */

TEST(formattable_default) {
    const nepdate_date date = date_of(2081, 4, 15);
    expect_format(date, NULL, NEPDATE_ENGLISH, "2081/04/15");
    expect_format(date, "G", NEPDATE_ENGLISH, "2081/04/15");
    expect_format(date, "g", NEPDATE_ENGLISH, "2081/04/15");
    expect_format(date, "d", NEPDATE_ENGLISH, "2081/04/15");
}

TEST(formattable_long) {
    char buffer[256];
    const size_t length = format(date_of(2081, 4, 15), "D", NEPDATE_ENGLISH, buffer, sizeof buffer);
    CHECK_EQ_INT(length, strlen("Shrawan 15, 2081"));
    CHECK(strstr(buffer, "2081") != NULL);
}

TEST(formattable_sortable) {
    expect_format(date_of(2081, 4, 15), "s", NEPDATE_ENGLISH, "2081-04-15");
    expect_format(date_of(2081, 4, 15), "yyyy-MM-dd", NEPDATE_ENGLISH, "2081-04-15");
}

TEST(formattable_custom) {
    const nepdate_date date = date_of(2081, 4, 15);
    expect_format(date, "yyyy-MM-dd", NEPDATE_ENGLISH, "2081-04-15");
    expect_format(date, "dd.MM.yyyy", NEPDATE_ENGLISH, "15.04.2081");
    expect_format(date, "MM/dd/yyyy", NEPDATE_ENGLISH, "04/15/2081");
    expect_format(date, "M/d/yyyy", NEPDATE_ENGLISH, "4/15/2081");
    expect_format(date, "yyyy'BS'", NEPDATE_ENGLISH, "2081BS");
    expect_format(date, "yyyy\\M\\M-\\d\\d", NEPDATE_ENGLISH, "2081MM-dd");
    expect_format(date, "yyyy\\-MM\\-dd", NEPDATE_ENGLISH, "2081-04-15");
}

TEST(formattable_names) {
    char buffer[256];
    CHECK_EQ_INT(format(date_of(2081, 4, 15), "MMM yyyy", NEPDATE_ENGLISH, buffer, sizeof buffer),
                 strlen("Shr 2081"));
    CHECK(strncmp(buffer, "Shr", 3) == 0);
    CHECK_EQ_INT(
        format(date_of(2081, 4, 15), "MMMM d, yyyy", NEPDATE_ENGLISH, buffer, sizeof buffer),
        strlen("Shrawan 15, 2081"));
    CHECK(strncmp(buffer, "Shrawan", 7) == 0);
    CHECK_EQ_INT(format(date_of(2081, 4, 15), "yy/MM/dd", NEPDATE_ENGLISH, buffer, sizeof buffer),
                 strlen("81/04/15"));
    CHECK(strncmp(buffer, "81/", 3) == 0);
}

TEST(formattable_single_digit_month) {
    expect_format(date_of(2081, 1, 5), "M/d/yyyy", NEPDATE_ENGLISH, "1/5/2081");
}

TEST(sortable_format_orders_dates) {
    char first[32];
    char second[32];
    CHECK_EQ_INT(format(date_of(2080, 1, 1), "s", NEPDATE_ENGLISH, first, sizeof first), 10);
    CHECK_EQ_INT(format(date_of(2081, 1, 1), "s", NEPDATE_ENGLISH, second, sizeof second), 10);
    CHECK(strcmp(first, second) < 0);
}

/* ---------- NepaliDateParsableTests ---------- */

TEST(parsable_generic) {
    check_strict("2080/05/15", 2080, 5, 15);
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_parse("2080/05/15", 10, &date), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_parse("not a date", 10, &date), NEPDATE_E_INVALID_CHARACTER);
}

/* ---------- NepaliDateMonthNameTests ---------- */

TEST(month_name_all_months) {
    for (int month = 1; month <= 12; month++) {
        nepdate_month result = NEPDATE_BAISHAKH;
        CHECK_EQ_INT(nepdate_month_of(date_of(2080, month, 1), &result), NEPDATE_OK);
        CHECK_EQ_INT(result, month);
    }
}

TEST(month_name_same_month) {
    nepdate_month first = NEPDATE_BAISHAKH;
    nepdate_month second = NEPDATE_BAISHAKH;
    CHECK_EQ_INT(nepdate_month_of(date_of(2080, 1, 1), &first), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_month_of(date_of(2080, 1, 15), &second), NEPDATE_OK);
    CHECK_EQ_INT(first, NEPDATE_BAISHAKH);
    CHECK_EQ_INT(second, NEPDATE_BAISHAKH);
}

TEST(month_name_same_month_different_years) {
    for (int year = 2070; year <= 2100; year += 10) {
        nepdate_month month = NEPDATE_BAISHAKH;
        CHECK_EQ_INT(nepdate_month_of(date_of(year, 9, 1), &month), NEPDATE_OK);
        CHECK_EQ_INT(month, NEPDATE_POUSH);
    }
}

TEST(month_name_adds_one_month) {
    nepdate_date next = {0};
    CHECK_EQ_INT(nepdate_add_months(date_of(2080, 10, 15), 1, NEPDATE_CLAMP, &next), NEPDATE_OK);
    nepdate_month month = NEPDATE_BAISHAKH;
    CHECK_EQ_INT(nepdate_month_of(next, &month), NEPDATE_OK);
    CHECK_EQ_INT(month, NEPDATE_FALGUN);
}

TEST(month_name_year_boundary) {
    nepdate_month month = NEPDATE_BAISHAKH;
    CHECK_EQ_INT(nepdate_month_of(date_of(2080, 12, 30), &month), NEPDATE_OK);
    CHECK_EQ_INT(month, NEPDATE_CHAITRA);
}

TEST(month_enum_values) {
    CHECK_EQ_INT(NEPDATE_BAISHAKH, 1);
    CHECK_EQ_INT(NEPDATE_JESTHA, 2);
    CHECK_EQ_INT(NEPDATE_ASHAD, 3);
    CHECK_EQ_INT(NEPDATE_SHRAWAN, 4);
    CHECK_EQ_INT(NEPDATE_BHADRA, 5);
    CHECK_EQ_INT(NEPDATE_ASHOJ, 6);
    CHECK_EQ_INT(NEPDATE_KARTIK, 7);
    CHECK_EQ_INT(NEPDATE_MANGSIR, 8);
    CHECK_EQ_INT(NEPDATE_POUSH, 9);
    CHECK_EQ_INT(NEPDATE_MAGH, 10);
    CHECK_EQ_INT(NEPDATE_FALGUN, 11);
    CHECK_EQ_INT(NEPDATE_CHAITRA, 12);
}

/* ---------- OptimizationVerificationTests ---------- */

TEST(optimization_to_string) {
    expect_format(date_of(2080, 5, 15), NULL, NEPDATE_ENGLISH, "2080/05/15");
    expect_format(date_of(2080, 1, 1), NULL, NEPDATE_ENGLISH, "2080/01/01");
    expect_format(date_of(2080, 12, 30), NULL, NEPDATE_ENGLISH, "2080/12/30");
    expect_format(date_of(1901, 1, 1), NULL, NEPDATE_ENGLISH, "1901/01/01");
    expect_format(date_of(2199, 12, 30), NULL, NEPDATE_ENGLISH, "2199/12/30");
    expect_format(date_of(2000, 6, 9), NULL, NEPDATE_ENGLISH, "2000/06/09");
    expect_format(date_of(2100, 10, 29), NULL, NEPDATE_ENGLISH, "2100/10/29");
}

TEST(optimization_to_string_length) {
    char buffer[64];
    CHECK_EQ_INT(format(date_of(2080, 5, 15), NULL, NEPDATE_ENGLISH, buffer, sizeof buffer), 10);
    CHECK_EQ_INT(strlen(buffer), 10);
}

TEST(optimization_zero_padding) {
    for (int month = 1; month <= 12; month++) {
        char buffer[8];
        char expected[4];
        CHECK_EQ_INT(snprintf(expected, sizeof expected, "%02d", month), 2);
        CHECK_EQ_INT(format(date_of(2080, month, 1), "MM", NEPDATE_ENGLISH, buffer, sizeof buffer),
                     2);
        CHECK_EQ_STR(buffer, expected);
    }
    for (int day = 1; day <= 28; day++) {
        char buffer[8];
        char expected[4];
        CHECK_EQ_INT(snprintf(expected, sizeof expected, "%02d", day), 2);
        CHECK_EQ_INT(format(date_of(2080, 1, day), "dd", NEPDATE_ENGLISH, buffer, sizeof buffer),
                     2);
        CHECK_EQ_STR(buffer, expected);
    }
}

TEST(optimization_to_string_roundtrip) {
    const nepdate_date date = date_of(2080, 5, 15);
    char buffer[32];
    CHECK_EQ_INT(format(date, NULL, NEPDATE_ENGLISH, buffer, sizeof buffer), 10);
    nepdate_date parsed = {0};
    CHECK_EQ_INT(nepdate_parse(buffer, strlen(buffer), &parsed), NEPDATE_OK);
    CHECK_EQ_INT(parsed.value, date.value);
}

TEST(short_format_orders) {
    const nepdate_date date = date_of(2080, 5, 15);
    char buffer[64];
    const struct {
        nepdate_order order;
        nepdate_separator separator;
        const char *expected;
    } rows[] = {
        {NEPDATE_YMD, NEPDATE_SEP_SLASH, "2080/05/15"},
        {NEPDATE_YMD, NEPDATE_SEP_DASH, "2080-05-15"},
        {NEPDATE_YMD, NEPDATE_SEP_DOT, "2080.05.15"},
        {NEPDATE_YMD, NEPDATE_SEP_UNDERSCORE, "2080_05_15"},
        {NEPDATE_YMD, NEPDATE_SEP_SPACE, "2080 05 15"},
        {NEPDATE_YMD, NEPDATE_SEP_BACKSLASH, "2080\\05\\15"},
        {NEPDATE_DMY, NEPDATE_SEP_SLASH, "15/05/2080"},
        {NEPDATE_DMY, NEPDATE_SEP_DASH, "15-05-2080"},
        {NEPDATE_MDY, NEPDATE_SEP_SLASH, "05/15/2080"},
        {NEPDATE_YDM, NEPDATE_SEP_SLASH, "2080/15/05"},
        {NEPDATE_MYD, NEPDATE_SEP_SLASH, "05/2080/15"},
        {NEPDATE_DYM, NEPDATE_SEP_SLASH, "15/2080/05"},
    };
    for (size_t i = 0; i < sizeof rows / sizeof rows[0]; i++) {
        const size_t length = nepdate_format_short(date, rows[i].order, rows[i].separator, true,
                                                   NEPDATE_ENGLISH, buffer, sizeof buffer);
        CHECK_EQ_INT(length, strlen(rows[i].expected));
        CHECK_EQ_STR(buffer, rows[i].expected);
    }
}

TEST(short_format_no_padding) {
    const nepdate_date date = date_of(2080, 5, 15);
    static const struct {
        nepdate_order order;
        nepdate_separator separator;
        const char *expected;
    } rows[] = {
        {NEPDATE_YMD, NEPDATE_SEP_SLASH, "2080/5/15"},
        {NEPDATE_DMY, NEPDATE_SEP_DASH, "15-5-2080"},
        {NEPDATE_MDY, NEPDATE_SEP_DOT, "5.15.2080"},
    };
    for (size_t i = 0; i < sizeof rows / sizeof rows[0]; i++) {
        char buffer[64];
        const size_t length = nepdate_format_short(date, rows[i].order, rows[i].separator, false,
                                                   NEPDATE_ENGLISH, buffer, sizeof buffer);
        CHECK_EQ_INT(length, strlen(rows[i].expected));
        CHECK_EQ_STR(buffer, rows[i].expected);
    }
}

TEST(optimization_unicode) {
    char buffer[128];
    const nepdate_date date = date_of(2080, 5, 15);
    CHECK_EQ_INT(nepdate_format_short(date, NEPDATE_YMD, NEPDATE_SEP_SLASH, true, NEPDATE_NEPALI,
                                      buffer, sizeof buffer),
                 26);
    CHECK_EQ_STR(buffer, "२०८०/०५/१५");
    CHECK(strstr(buffer, "१") != NULL);
    char dash[128];
    CHECK_EQ_INT(nepdate_format_short(date, NEPDATE_YMD, NEPDATE_SEP_DASH, true, NEPDATE_NEPALI,
                                      dash, sizeof dash),
                 26);
    CHECK(strstr(dash, "-") != NULL);
    CHECK_EQ_INT(code_points(buffer), 10);
    CHECK_EQ_INT(code_points("2080/05/15"), 10);
}

TEST(optimization_nepali_digits) {
    check_lenient("२०८०/०५/१५", 2080, 5, 15);
    check_lenient("२०८०-०५-१५", 2080, 5, 15);
    check_lenient("2080/05/15", 2080, 5, 15);
    check_lenient("२०८०/05/१५", 2080, 5, 15);
}

TEST(optimization_indicators) {
    check_lenient("2080/05/15 B.S.", 2080, 5, 15);
    check_lenient("2080/05/15 BS", 2080, 5, 15);
    check_lenient("2080/05/15 V.S.", 2080, 5, 15);
    check_lenient("2080/05/15 VS", 2080, 5, 15);
    check_lenient("2080/05/15 bs", 2080, 5, 15);
    check_lenient("2080/05/15 v.s.", 2080, 5, 15);
    check_lenient("2080/05/15 गते", 2080, 5, 15);
    check_lenient("  2080 / 05 / 15  ", 2080, 5, 15);
}

TEST(optimization_month_names) {
    static const char *const names[] = {
        "Baisakh", "Jestha",  "Asar",  "Shrawan", "Bhadra", "Ashwin",
        "Kartik",  "Mangsir", "Poush", "Magh",    "Falgun", "Chaitra",
    };
    for (int month = 1; month <= 12; month++) {
        char text[64];
        CHECK_EQ_INT(snprintf(text, sizeof text, "15 %s 2080", names[month - 1]) > 0, 1);
        check_lenient(text, 2080, month, 15);
    }
}

TEST(optimization_month_name_positions) {
    check_lenient("Baisakh 15, 2080", 2080, 1, 15);
    check_lenient("2080 Baisakh 15", 2080, 1, 15);
}

TEST(optimization_nepali_month_names) {
    check_lenient("15 वैशाख 2080", 2080, 1, 15);
    check_lenient("15 जेष्ठ 2080", 2080, 2, 15);
    check_lenient("15 असार 2080", 2080, 3, 15);
    check_lenient("15 श्रावण 2080", 2080, 4, 15);
}

TEST(optimization_invalid_input) {
    check_lenient_error("not a date", NEPDATE_E_UNRECOGNIZED);
    check_lenient_error("", NEPDATE_E_UNRECOGNIZED);
    check_lenient_error(" ", NEPDATE_E_UNRECOGNIZED);
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_parse_lenient(NULL, 0, &date), NEPDATE_E_NULL);
}

TEST(optimization_try_parse) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_parse_lenient("2080/05/15", 10, &date), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_parse_lenient("garbage", 7, &date), NEPDATE_E_UNRECOGNIZED);
    CHECK_EQ_INT(nepdate_parse_lenient(NULL, 0, &date), NEPDATE_E_NULL);
}

TEST(optimization_roundtrip_all_first_days) {
    for (int month = 1; month <= 12; month++) {
        const nepdate_date date = date_of(2080, month, 1);
        char text[32];
        CHECK_EQ_INT(format(date, NULL, NEPDATE_ENGLISH, text, sizeof text), 10);
        nepdate_date parsed = {0};
        CHECK_EQ_INT(nepdate_parse(text, strlen(text), &parsed), NEPDATE_OK);
        CHECK_EQ_INT(parsed.value, date.value);
    }
}

TEST(optimization_unicode_roundtrip) {
    for (int month = 1; month <= 12; month++) {
        const nepdate_date date = date_of(2080, month, 1);
        char text[64];
        CHECK_EQ_INT(nepdate_format_short(date, NEPDATE_YMD, NEPDATE_SEP_SLASH, true,
                                          NEPDATE_NEPALI, text, sizeof text),
                     26);
        nepdate_date parsed = {0};
        CHECK_EQ_INT(nepdate_parse(text, strlen(text), &parsed), NEPDATE_OK);
        CHECK_EQ_INT(parsed.value, date.value);
    }
}

TEST(long_date_string) {
    const nepdate_long_options plain = {false, true, true};
    const nepdate_long_options weekday = {true, true, true};
    const nepdate_long_options no_year = {false, false, true};
    char text[256];
    CHECK_EQ_INT(
        nepdate_format_long(date_of(2081, 4, 15), plain, NEPDATE_ENGLISH, text, sizeof text), 16);
    CHECK(strstr(text, "15") != NULL);
    CHECK(strstr(text, "Shrawan") != NULL);
    CHECK_EQ_INT(
        nepdate_format_long(date_of(2081, 4, 15), weekday, NEPDATE_ENGLISH, text, sizeof text), 25);
    CHECK(strstr(text, "Tuesday") != NULL);
    CHECK_EQ_INT(
        nepdate_format_long(date_of(2081, 4, 15), no_year, NEPDATE_ENGLISH, text, sizeof text), 10);
    CHECK(strstr(text, "2080") == NULL);
}

/* ---------- SmartDateParserTests and StringExtensionsTests ---------- */

TEST(smart_standard) {
    check_lenient("2080/04/15", 2080, 4, 15);
    check_lenient("15/04/2080", 2080, 4, 15);
    check_lenient("15 Shrawan 2080", 2080, 4, 15);
    check_lenient("२०८०/०४/१५", 2080, 4, 15);
    check_lenient("15 साउन 2080", 2080, 4, 15);
    check_lenient("15 Shrawan 2080 B.S.", 2080, 4, 15);
    check_lenient("15 Srawan 2080", 2080, 4, 15);
}

TEST(smart_invalid) {
    check_lenient_error("not a date", NEPDATE_E_UNRECOGNIZED);
    check_lenient_error("04/15/2080", NEPDATE_E_INVALID_MONTH);
    check_lenient_error("15/04/80", NEPDATE_E_AMBIGUOUS);
}

TEST(smart_try_parse) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_parse_lenient("2080/04/15", 10, &date), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_parse_lenient("not a date", 10, &date), NEPDATE_E_UNRECOGNIZED);
}

TEST(extension_to_nepali_date) {
    check_strict("2080/04/15", 2080, 4, 15);
    check_strict("२०८०/०४/१५", 2080, 4, 15);
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_parse("invalid", 7, &date), NEPDATE_E_INVALID_CHARACTER);
    CHECK_EQ_INT(nepdate_parse("2080/5/15", 9, &date), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_parse("2080/05/15/99", 13, &date), NEPDATE_E_WRONG_GROUP_COUNT);
}

int main(void) {
    RUN_TEST_NAMED("NepaliDateFormattableTests.ToString_NullFormat_ReturnsDefault",
                   formattable_default);
    RUN_TEST_NAMED("NepaliDateFormattableTests.ToString_G_SameAsDefault", formattable_default);
    RUN_TEST_NAMED("NepaliDateFormattableTests.ToString_g_SameAsDefault", formattable_default);
    RUN_TEST_NAMED("NepaliDateFormattableTests.ToString_d_SameAsDefault", formattable_default);
    RUN_TEST_NAMED("NepaliDateFormattableTests.ToString_D_ReturnsLongDateString", formattable_long);
    RUN_TEST_NAMED("NepaliDateFormattableTests.ToString_s_ReturnsSortableIsoFormat",
                   formattable_sortable);
    RUN_TEST_NAMED("NepaliDateFormattableTests.CustomFormat_yyyy_MM_dd_WithDash",
                   formattable_custom);
    RUN_TEST_NAMED("NepaliDateFormattableTests.CustomFormat_dd_MM_yyyy_WithDot",
                   formattable_custom);
    RUN_TEST_NAMED("NepaliDateFormattableTests.CustomFormat_MM_slash_dd_slash_yyyy",
                   formattable_custom);
    RUN_TEST_NAMED(
        "NepaliDateFormattableTests.CustomFormat_MMMxxx_ReturnsThreeCharMonthAbbreviation",
        formattable_names);
    RUN_TEST_NAMED("NepaliDateFormattableTests.CustomFormat_MMMM_ReturnsFullMonthName",
                   formattable_names);
    RUN_TEST_NAMED("NepaliDateFormattableTests.CustomFormat_yy_ReturnsTwoDigitYear",
                   formattable_names);
    RUN_TEST_NAMED("NepaliDateFormattableTests.CustomFormat_M_WithoutLeadingZero_ReturnsUnpadded",
                   formattable_custom);
    RUN_TEST_NAMED("NepaliDateFormattableTests.CustomFormat_SingleDigitMonth_WithoutLeadingZero",
                   formattable_single_digit_month);
    RUN_TEST_NAMED("NepaliDateFormattableTests.CustomFormat_LiteralInSingleQuotes",
                   formattable_custom);
    RUN_TEST_NAMED("NepaliDateFormattableTests.CustomFormat_BackslashEscapedTokens_ProduceLiterals",
                   formattable_custom);
    RUN_TEST_NAMED(
        "NepaliDateFormattableTests.CustomFormat_BackslashBeforeLiteralChar_IsPassedThrough",
        formattable_custom);
    RUN_TEST_NAMED(
        "NepaliDateFormattableTests.StringInterpolation_WithFormatSpecifier_UsesIFormattable",
        formattable_custom);
    RUN_TEST_NAMED("NepaliDateFormattableTests.StringFormatMethod_WithSpecifier",
                   formattable_custom);
    RUN_TEST_NAMED(
        "NepaliDateFormattableTests.SortableFormat_LexicographicOrderEqualsChronologicalOrder",
        sortable_format_orders_dates);

    RUN_TEST_NAMED("NepaliDateParsableTests.Parse_Via_GenericConstraint_Returns_CorrectDate",
                   parsable_generic);
    RUN_TEST_NAMED("NepaliDateParsableTests.TryParse_Via_GenericConstraint_ValidDate_Returns_True",
                   parsable_generic);
    RUN_TEST_NAMED(
        "NepaliDateParsableTests.TryParse_Via_GenericConstraint_InvalidDate_Returns_False",
        parsable_generic);
    RUN_TEST_NAMED("NepaliDateParsableTests.SpanParse_Via_GenericConstraint_Returns_CorrectDate",
                   parsable_generic);
    RUN_TEST_NAMED(
        "NepaliDateParsableTests.SpanTryParse_Via_GenericConstraint_ValidDate_Returns_True",
        parsable_generic);

    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#1",
                   month_name_all_months);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#2",
                   month_name_all_months);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#3",
                   month_name_all_months);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#4",
                   month_name_all_months);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#5",
                   month_name_all_months);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#6",
                   month_name_all_months);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#7",
                   month_name_all_months);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#8",
                   month_name_all_months);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#9",
                   month_name_all_months);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#10",
                   month_name_all_months);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#11",
                   month_name_all_months);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#12",
                   month_name_all_months);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_SameMonthDifferentDays_AllReturnSameEnum",
                   month_name_same_month);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_SameMonthDifferentYears_AllReturnSameEnum#1",
                   month_name_same_month_different_years);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_SameMonthDifferentYears_AllReturnSameEnum#2",
                   month_name_same_month_different_years);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_SameMonthDifferentYears_AllReturnSameEnum#3",
                   month_name_same_month_different_years);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_SameMonthDifferentYears_AllReturnSameEnum#4",
                   month_name_same_month_different_years);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_AddingOneMonth_AdvancesEnum",
                   month_name_adds_one_month);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.MonthName_YearBoundary_WrapsFromChaitraToBasikhakh",
                   month_name_year_boundary);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#1",
                   month_enum_values);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#2",
                   month_enum_values);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#3",
                   month_enum_values);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#4",
                   month_enum_values);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#5",
                   month_enum_values);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#6",
                   month_enum_values);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#7",
                   month_enum_values);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#8",
                   month_enum_values);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#9",
                   month_enum_values);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#10",
                   month_enum_values);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#11",
                   month_enum_values);
    RUN_TEST_NAMED("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#12",
                   month_enum_values);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToString_ProducesExactFormat#1",
                   optimization_to_string);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToString_ProducesExactFormat#2",
                   optimization_to_string);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToString_ProducesExactFormat#3",
                   optimization_to_string);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToString_ProducesExactFormat#4",
                   optimization_to_string);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToString_ProducesExactFormat#5",
                   optimization_to_string);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToString_ProducesExactFormat#6",
                   optimization_to_string);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToString_ProducesExactFormat#7",
                   optimization_to_string);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToString_OutputIsExactly10Characters",
                   optimization_to_string_length);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToString_AllMonthsZeroPadded",
                   optimization_zero_padding);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToString_AllDaysZeroPadded",
                   optimization_zero_padding);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToString_RoundtripsWithConstructor",
                   optimization_to_string_roundtrip);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#1",
        short_format_orders);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#2",
        short_format_orders);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#3",
        short_format_orders);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#4",
        short_format_orders);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#5",
        short_format_orders);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#6",
        short_format_orders);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#7",
        short_format_orders);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#8",
        short_format_orders);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#9",
        short_format_orders);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#10",
        short_format_orders);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#11",
        short_format_orders);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#12",
        short_format_orders);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_WithoutLeadingZeros_ProducesCorrectOutput#1",
        short_format_no_padding);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_WithoutLeadingZeros_ProducesCorrectOutput#2",
        short_format_no_padding);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_WithoutLeadingZeros_ProducesCorrectOutput#3",
        short_format_no_padding);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToUnicodeString_DefaultFormat_ConvertsAllDigits",
                   optimization_unicode);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToUnicodeString_EachDigitConvertsCorrectly",
                   optimization_unicode);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToUnicodeString_SeparatorsPreserved",
                   optimization_unicode);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToUnicodeString_SameLengthAsToString",
                   optimization_unicode);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_NepaliDigits_ParseCorrectly#1",
                   optimization_nepali_digits);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_NepaliDigits_ParseCorrectly#2",
                   optimization_nepali_digits);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.SmartDateParser_PureEnglishDigits_NoConversionNeeded",
        optimization_nepali_digits);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.SmartDateParser_MixedNepaliEnglishDigits_ParseCorrectly",
        optimization_nepali_digits);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.SmartDateParser_WithIndicators_RemovesAndParsesCorrectly#1",
        optimization_indicators);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.SmartDateParser_WithIndicators_RemovesAndParsesCorrectly#2",
        optimization_indicators);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.SmartDateParser_WithIndicators_RemovesAndParsesCorrectly#3",
        optimization_indicators);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.SmartDateParser_WithIndicators_RemovesAndParsesCorrectly#4",
        optimization_indicators);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.SmartDateParser_WithIndicators_RemovesAndParsesCorrectly#5",
        optimization_indicators);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.SmartDateParser_WithIndicators_RemovesAndParsesCorrectly#6",
        optimization_indicators);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.SmartDateParser_WithNepaliKeywords_RemovesAndParses",
        optimization_indicators);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_WithExtraSpaces_ParsesCorrectly",
                   optimization_indicators);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#1",
                   optimization_month_names);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#2",
                   optimization_month_names);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#3",
                   optimization_month_names);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#4",
                   optimization_month_names);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#5",
                   optimization_month_names);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#6",
                   optimization_month_names);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#7",
                   optimization_month_names);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#8",
                   optimization_month_names);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#9",
                   optimization_month_names);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#10",
                   optimization_month_names);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#11",
                   optimization_month_names);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#12",
                   optimization_month_names);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_MonthNameDifferentPositions_"
                   "ParsesCorrectly#1",
                   optimization_month_name_positions);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_MonthNameDifferentPositions_"
                   "ParsesCorrectly#2",
                   optimization_month_name_positions);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.SmartDateParser_NepaliUnicodeMonthNames_ParseCorrectly#1",
        optimization_nepali_month_names);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.SmartDateParser_NepaliUnicodeMonthNames_ParseCorrectly#2",
        optimization_nepali_month_names);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.SmartDateParser_NepaliUnicodeMonthNames_ParseCorrectly#3",
        optimization_nepali_month_names);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.SmartDateParser_NepaliUnicodeMonthNames_ParseCorrectly#4",
        optimization_nepali_month_names);
    /* cs-pending: OptimizationVerificationTests.SmartDateParser_TwoDigitYear_AdjustedTo2000s phase 4: needs today; this port returns Ambiguous (D-06) */
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.SmartDateParser_InvalidInput_ThrowsFormatException",
        optimization_invalid_input);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_NullInput_ThrowsException",
                   optimization_invalid_input);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_EmptyInput_ThrowsException",
                   optimization_invalid_input);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_WhitespaceInput_ThrowsException",
                   optimization_invalid_input);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_TryParse_ValidInput_ReturnsTrue",
                   optimization_try_parse);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.SmartDateParser_TryParse_InvalidInput_ReturnsFalse",
        optimization_try_parse);
    RUN_TEST_NAMED("OptimizationVerificationTests.SmartDateParser_TryParse_NullInput_ReturnsFalse",
                   optimization_try_parse);
    RUN_TEST_NAMED(
        "OptimizationVerificationTests.ToString_EveryMonthFirstDay_RoundtripsViaConstructor",
        optimization_roundtrip_all_first_days);
    RUN_TEST_NAMED("OptimizationVerificationTests.UnicodeConversion_AllDigits_RoundtripCorrectly",
                   optimization_unicode_roundtrip);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToLongDateString_Default_ContainsMonthNameAndDay",
                   long_date_string);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToLongDateString_WithDayName_ContainsDayOfWeek",
                   long_date_string);
    RUN_TEST_NAMED("OptimizationVerificationTests.ToLongDateString_WithoutYear_DoesNotContainYear",
                   long_date_string);

    RUN_TEST_NAMED("SmartDateParserTests.Parse_StandardFormat_ReturnsCorrectDate", smart_standard);
    RUN_TEST_NAMED("SmartDateParserTests.Parse_InvertedFormat_ReturnsCorrectDate", smart_standard);
    /* cs-deviation: SmartDateParserTests.Parse_MonthDayYearFormat_ReturnsCorrectDate D-06 (no
     * month-day-year guess) */
    RUN_TEST_NAMED("SmartDateParserTests.Parse_WithMonthNames_ReturnsCorrectDate", smart_standard);
    RUN_TEST_NAMED("SmartDateParserTests.Parse_WithNepaliUnicode_ReturnsCorrectDate",
                   smart_standard);
    RUN_TEST_NAMED("SmartDateParserTests.Parse_MixedFormats_ReturnsCorrectDate", smart_standard);
    RUN_TEST_NAMED("SmartDateParserTests.Parse_WithSuffixes_ReturnsCorrectDate", smart_standard);
    RUN_TEST_NAMED("SmartDateParserTests.Parse_WithTypos_ReturnsCorrectDate", smart_standard);
    /* cs-deviation: SmartDateParserTests.Parse_ShorterYearFormats_ReturnsCorrectDate D-06 (a
     * 2-digit year is Ambiguous) */
    RUN_TEST_NAMED("SmartDateParserTests.Parse_InvalidFormat_ThrowsFormatException", smart_invalid);
    RUN_TEST_NAMED("SmartDateParserTests.TryParse_ValidFormat_ReturnsTrue", smart_try_parse);
    RUN_TEST_NAMED("SmartDateParserTests.TryParse_InvalidFormat_ReturnsFalse", smart_try_parse);
    RUN_TEST_NAMED("SmartDateParserTests.ExtensionMethod_ToNepaliDate_ParsesCorrectly",
                   extension_to_nepali_date);
    RUN_TEST_NAMED("SmartDateParserTests.ExtensionMethod_TryToNepaliDate_ParsesCorrectly",
                   extension_to_nepali_date);
    RUN_TEST_NAMED("StringExtensionsTests.ToNepaliDate_ValidDateString_ReturnsNepaliDate",
                   extension_to_nepali_date);
    RUN_TEST_NAMED("StringExtensionsTests.ToNepaliDate_NepaliDigits_ReturnsNepaliDate",
                   extension_to_nepali_date);
    RUN_TEST_NAMED("StringExtensionsTests.ToNepaliDate_InvalidDateString_ThrowsFormatException",
                   extension_to_nepali_date);
    RUN_TEST_NAMED("StringExtensionsTests.TryToNepaliDate_ValidDateString_ReturnsTrue",
                   extension_to_nepali_date);
    RUN_TEST_NAMED("StringExtensionsTests.TryToNepaliDate_InvalidDateString_ReturnsFalse",
                   extension_to_nepali_date);
    return harness_finish();
}
