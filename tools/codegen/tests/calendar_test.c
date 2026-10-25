#include "calendar.h"
#include "harness.h"
#include "json.h"

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
        "",         "{",          "[1,]",     "{\"a\" 1}",    "01",      "1.",
        "\"\\x\"",  "\"\\ud800\"", "tru",      "{} x",        "[1e]",    "\"a",
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

TEST(rendered_header_is_lf_only_with_the_bounds) {
    codegen_calendar calendar;
    memset(&calendar, 0, sizeof calendar);
    calendar.min_year = 1901;
    calendar.max_year = 2199;
    char *text = codegen_render_calendar_tables(&calendar);
    CHECK(text != NULL);
    if (text == NULL) {
        return;
    }
    static const char first_line[] =
        "/* Generated by tools/codegen from shared/data. DO NOT EDIT. */\n";
    CHECK(strncmp(text, first_line, sizeof first_line - 1) == 0);
    CHECK(strchr(text, '\r') == NULL);
    CHECK(strstr(text, "#define NEPDATE_MIN_YEAR 1901\n") != NULL);
    CHECK(strstr(text, "#define NEPDATE_MAX_YEAR 2199\n") != NULL);
    free(text);
}

int main(void) {
    RUN_TEST(shared_calendar_parses);
    RUN_TEST(month_length_33_names_year_and_month);
    RUN_TEST(month_length_28_is_rejected);
    RUN_TEST(wrong_bounds_are_rejected);
    RUN_TEST(invalid_json_is_rejected);
    RUN_TEST(json_values_decode);
    RUN_TEST(rendered_header_is_lf_only_with_the_bounds);
    return harness_finish();
}
