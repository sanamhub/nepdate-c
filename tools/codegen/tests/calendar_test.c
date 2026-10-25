#include "calendar.h"
#include "harness.h"
#include "json.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_shared_calendar(size_t *length) {
    FILE *file = fopen(NEPDATE_C_SHARED_DIR "/data/calendar/bs-calendar.json", "rb");
    if (file == NULL) {
        return NULL;
    }
    char *text = calloc(1, 65536);
    if (text != NULL) {
        *length = fread(text, 1, 65535, file);
    }
    (void)fclose(file);
    return text;
}

/* Replaces the month-th number after the key "year" with replacement. */
static bool set_month_length(char *text, const char *year, int month, const char *replacement) {
    char key[16];
    (void)snprintf(key, sizeof key, "\"%s\": [", year);
    char *at = strstr(text, key);
    if (at == NULL) {
        return false;
    }
    at += strlen(key);
    for (int m = 1; m < month; m++) {
        at = strchr(at, ',');
        if (at == NULL) {
            return false;
        }
        at += 2; /* ", " */
    }
    /* Every length in the file has two digits, so the replacement must too. */
    if (strlen(replacement) != 2) {
        return false;
    }
    memcpy(at, replacement, 2);
    return true;
}

TEST(shared_calendar_parses) {
    size_t length = 0;
    char *text = read_shared_calendar(&length);
    CHECK(text != NULL);
    if (text == NULL) {
        return;
    }
    codegen_calendar calendar;
    char error[256] = "";
    CHECK(codegen_parse_calendar(text, length, &calendar, error, sizeof error));
    CHECK_EQ_STR(error, "");
    CHECK_EQ_INT(calendar.min_year, 1901);
    CHECK_EQ_INT(calendar.max_year, 2199);
    /* 2081: 31, 32, 31, 32, 31, 30, 30, 30, 29, 30, 29, 31 */
    CHECK_EQ_INT(calendar.month_lengths[2081 - 1901][1], 32);
    CHECK_EQ_INT(calendar.month_lengths[2081 - 1901][8], 29);
    free(text);
}

TEST(month_length_33_names_year_and_month) {
    size_t length = 0;
    char *text = read_shared_calendar(&length);
    CHECK(text != NULL);
    if (text == NULL) {
        return;
    }
    CHECK(set_month_length(text, "2050", 7, "33"));
    codegen_calendar calendar;
    char error[256] = "";
    CHECK(!codegen_parse_calendar(text, length, &calendar, error, sizeof error));
    CHECK_EQ_STR(error, "bs-calendar.json: year 2050 month 7: length 33 is outside 29..32");
    free(text);
}

TEST(month_length_28_is_rejected) {
    size_t length = 0;
    char *text = read_shared_calendar(&length);
    CHECK(text != NULL);
    if (text == NULL) {
        return;
    }
    CHECK(set_month_length(text, "1901", 1, "28"));
    codegen_calendar calendar;
    char error[256] = "";
    CHECK(!codegen_parse_calendar(text, length, &calendar, error, sizeof error));
    CHECK_EQ_STR(error, "bs-calendar.json: year 1901 month 1: length 28 is outside 29..32");
    free(text);
}

TEST(wrong_bounds_are_rejected) {
    static const char min_year[] = "{\"min_year\": 1900, \"max_year\": 2199}";
    static const char max_year[] = "{\"min_year\": 1901, \"max_year\": 2200}";
    static const char epoch[] = "{\"min_year\": 1901, \"max_year\": 2199,"
                                " \"epoch\": {\"bs\": \"1901-01-01\", \"ad\": \"1844-04-12\"}}";
    static const char missing[] = "{\"min_year\": 1901, \"max_year\": 2199,"
                                  " \"epoch\": {\"bs\": \"1901-01-01\", \"ad\": \"1844-04-11\"},"
                                  " \"month_lengths\": {}}";
    codegen_calendar calendar;
    char error[256] = "";
    CHECK(!codegen_parse_calendar(min_year, strlen(min_year), &calendar, error, sizeof error));
    CHECK_EQ_STR(error, "bs-calendar.json: min_year must be 1901");
    CHECK(!codegen_parse_calendar(max_year, strlen(max_year), &calendar, error, sizeof error));
    CHECK_EQ_STR(error, "bs-calendar.json: max_year must be 2199");
    CHECK(!codegen_parse_calendar(epoch, strlen(epoch), &calendar, error, sizeof error));
    CHECK_EQ_STR(error, "bs-calendar.json: epoch must be bs 1901-01-01, ad 1844-04-11");
    CHECK(!codegen_parse_calendar(missing, strlen(missing), &calendar, error, sizeof error));
    CHECK_EQ_STR(error, "bs-calendar.json: month_lengths must hold the 299 years 1901 to 2199");
}

TEST(invalid_json_is_rejected) {
    static const char *const inputs[] = {
        "",        "{",           "[1,]", "{\"a\" 1}", "01",   "1.",
        "\"\\x\"", "\"\\ud800\"", "tru",  "{} x",      "[1e]", "\"a",
    };
    for (size_t i = 0; i < sizeof inputs / sizeof inputs[0]; i++) {
        char error[128] = "";
        json_value *value = json_parse(inputs[i], strlen(inputs[i]), error, sizeof error);
        CHECK(value == NULL);
        CHECK(strncmp(error, "JSON error at byte ", 19) == 0);
        json_free(value);
    }
}

TEST(json_values_decode) {
    static const char text[] = "{\"s\": \"a\\u00e9\\ud83d\\ude00\\n\", \"n\": -9223372036854775808,"
                               " \"big\": 9223372036854775808, \"f\": 1.5e3, \"t\": true,"
                               " \"z\": null, \"a\": [[], {}]}";
    char error[128] = "";
    json_value *value = json_parse(text, strlen(text), error, sizeof error);
    CHECK(value != NULL);
    if (value == NULL) {
        return;
    }
    const json_value *s = json_get(value, "s");
    CHECK(s != NULL && s->type == JSON_STRING);
    CHECK_EQ_STR(s == NULL ? NULL : s->string, "a\303\251\360\237\230\200\n");
    const json_value *n = json_get(value, "n");
    CHECK(n != NULL && n->is_integer);
    CHECK(n != NULL && n->integer == -9223372036854775807LL - 1);
    const json_value *big = json_get(value, "big");
    CHECK(big != NULL && big->type == JSON_NUMBER && !big->is_integer);
    const json_value *f = json_get(value, "f");
    CHECK(f != NULL && f->type == JSON_NUMBER && !f->is_integer);
    const json_value *t = json_get(value, "t");
    CHECK(t != NULL && t->type == JSON_BOOL && t->boolean);
    const json_value *z = json_get(value, "z");
    CHECK(z != NULL && z->type == JSON_NULL);
    const json_value *a = json_get(value, "a");
    CHECK(a != NULL && a->type == JSON_ARRAY && a->count == 2);
    CHECK(json_get(value, "missing") == NULL);
    json_free(value);
}

/* Parses the shared calendar into a heap copy (the struct is too large for some stacks). */
static codegen_calendar *shared_calendar(void) {
    size_t length = 0;
    char *text = read_shared_calendar(&length);
    codegen_calendar *calendar = malloc(sizeof *calendar);
    char error[256] = "";
    const bool ok = text != NULL && calendar != NULL &&
                    codegen_parse_calendar(text, length, calendar, error, sizeof error);
    free(text);
    if (!ok) {
        free(calendar);
        return NULL;
    }
    return calendar;
}

TEST(days_from_civil_known_days) {
    CHECK_EQ_INT(codegen_days_from_civil(1970, 1, 1), 0);
    CHECK_EQ_INT(codegen_days_from_civil(2000, 3, 1), 11017);
    CHECK_EQ_INT(codegen_days_from_civil(1844, 4, 11), -45920);
    CHECK_EQ_INT(codegen_days_from_civil(-1, 1, 1), -719893);
}

/* The derived tables against ALGORITHM sections 2 and 9 and the data checks in
 * shared/data/README.md. */
TEST(derived_tables_match_the_spec) {
    codegen_calendar *calendar = shared_calendar();
    CHECK(calendar != NULL);
    if (calendar == NULL) {
        return;
    }
    CHECK_EQ_INT(calendar->month_start[CODEGEN_ALL_MONTHS], 109212);
    CHECK_EQ_INT(calendar->year_start[CODEGEN_YEARS], 109212);
    CHECK_EQ_INT(calendar->epoch_unix_days, -45920);
    /* AD 1844-04-01 is ten days before the epoch; AD 2143-04-15 is the last day, 109211. */
    CHECK_EQ_INT(calendar->ad_month_start[0], -10);
    CHECK_EQ_INT(calendar->ad_month_start[CODEGEN_AD_MONTHS - 1] + 14, 109211);
    /* BS 2081-01-01 is AD 2024-04-13. */
    const long bs_2081 = calendar->year_start[2081 - 1901];
    CHECK_EQ_INT(calendar->month_start[(2081 - 1901) * 12], bs_2081);
    CHECK_EQ_INT(bs_2081 + calendar->epoch_unix_days, codegen_days_from_civil(2024, 4, 13));
    CHECK_EQ_INT(calendar->ad_month_start[(2024 - 1844) * 12] + 12, bs_2081);

    /* Year lengths: 364 once, 365 221 times, 366 76 times, 367 once. */
    int histogram[4] = {0, 0, 0, 0};
    for (int i = 0; i < CODEGEN_YEARS; i++) {
        const long days = calendar->year_start[i + 1] - calendar->year_start[i];
        CHECK(days >= 364 && days <= 367);
        if (days >= 364 && days <= 367) {
            histogram[days - 364]++;
        }
    }
    CHECK_EQ_INT(histogram[0], 1);
    CHECK_EQ_INT(histogram[1], 221);
    CHECK_EQ_INT(histogram[2], 76);
    CHECK_EQ_INT(histogram[3], 1);

    /* FNV-1a 64 of the 3588 month lengths, one byte each: the fingerprint nepdate-cpp builds
     * from the same file, so both ports read the same data. */
    uint64_t hash = 0xcbf29ce484222325U;
    for (int y = 0; y < CODEGEN_YEARS; y++) {
        for (int m = 0; m < CODEGEN_MONTHS; m++) {
            hash = (hash ^ calendar->month_lengths[y][m]) * 0x100000001b3U;
        }
    }
    CHECK(hash == 0xc2cdd10a9b668dc3U);
    CHECK_EQ_INT(calendar->estimate_mul, 137797);
    free(calendar);
}

TEST(month_estimate_two_months_off_is_rejected) {
    codegen_calendar *calendar = shared_calendar();
    CHECK(calendar != NULL);
    if (calendar == NULL) {
        return;
    }
    /* Short months first and long months last move the middle of the calendar far from the
     * average month length. */
    for (int y = 0; y < CODEGEN_YEARS; y++) {
        for (int m = 0; m < CODEGEN_MONTHS; m++) {
            calendar->month_lengths[y][m] = y < CODEGEN_YEARS / 2 ? 29 : 32;
        }
    }
    char error[256] = "";
    CHECK(!codegen_derive_tables(calendar, error, sizeof error));
    CHECK(strstr(error, "the month estimate for day") != NULL);
    free(calendar);
}

TEST(rendered_header_is_lf_only_with_the_tables) {
    codegen_calendar *calendar = shared_calendar();
    CHECK(calendar != NULL);
    if (calendar == NULL) {
        return;
    }
    char *text = codegen_render_calendar_tables(calendar);
    free(calendar);
    CHECK(text != NULL);
    if (text == NULL) {
        return;
    }
    static const char first_line[] =
        "/* Generated by tools/codegen from shared/data. DO NOT EDIT. */\n";
    CHECK(strncmp(text, first_line, sizeof first_line - 1) == 0);
    CHECK(strchr(text, '\r') == NULL);
    CHECK(strstr(text, "#define NEPDATE_CAL_MIN_YEAR 1901\n") != NULL);
    CHECK(strstr(text, "#define NEPDATE_CAL_MAX_YEAR 2199\n") != NULL);
    CHECK(strstr(text, "#define NEPDATE_CAL_DAYS 109212\n") != NULL);
    CHECK(strstr(text, "#define NEPDATE_CAL_EPOCH_UNIX_DAYS (-45920)\n") != NULL);
    CHECK(strstr(text, "static const uint8_t nepdate_cal_month_length[3588] = {\n"
                       "    31, 31, 32, 31, 31, 31, 30, 29, 30, 29, 30, 30, /* 1901 */\n") != NULL);
    CHECK(strstr(text, "    109212, /* 2200 */\n};\n") != NULL);
    CHECK(strstr(text, "static const int32_t nepdate_cal_ad_month_start[3589] = {\n"
                       "    -10, 20, 51, 81, 112, 143, 173, 204, 234, /* 1844 */\n") != NULL);
    free(text);
}

int main(void) {
    RUN_TEST(shared_calendar_parses);
    RUN_TEST(month_length_33_names_year_and_month);
    RUN_TEST(month_length_28_is_rejected);
    RUN_TEST(wrong_bounds_are_rejected);
    RUN_TEST(invalid_json_is_rejected);
    RUN_TEST(json_values_decode);
    RUN_TEST(days_from_civil_known_days);
    RUN_TEST(derived_tables_match_the_spec);
    RUN_TEST(month_estimate_two_months_off_is_rejected);
    RUN_TEST(rendered_header_is_lf_only_with_the_tables);
    return harness_finish();
}
