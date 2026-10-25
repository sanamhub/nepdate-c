/* Parsing: every shared vector, with an exact heap block so a read past the length trips ASan. */
#include "harness.h"
#include "vectors.h"

#include <nepdate.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static nepdate_status expected_status(const char *text) {
    static const struct {
        const char *name;
        nepdate_status status;
    } kinds[] = {
        {"InvalidCharacter", NEPDATE_E_INVALID_CHARACTER},
        {"WrongGroupCount", NEPDATE_E_WRONG_GROUP_COUNT},
        {"NumberTooLong", NEPDATE_E_NUMBER_TOO_LONG},
        {"Ambiguous", NEPDATE_E_AMBIGUOUS},
        {"Unrecognized", NEPDATE_E_UNRECOGNIZED},
        {"OutOfRange", NEPDATE_E_OUT_OF_RANGE},
        {"InvalidMonth", NEPDATE_E_INVALID_MONTH},
        {"InvalidDay", NEPDATE_E_INVALID_DAY},
    };
    for (size_t i = 0; i < sizeof kinds / sizeof kinds[0]; i++) {
        if (strcmp(text, kinds[i].name) == 0) {
            return kinds[i].status;
        }
    }
    return NEPDATE_E_INVALID_DATE;
}

/* A parse call on exactly length bytes, and on the same bytes followed by more text. */
static void check_parse(const char *text, size_t length, nepdate_status status,
                        const nepdate_date *expected, bool lenient) {
    char *exact = malloc(length == 0 ? 1 : length);
    char *padded = malloc(length + 16);
    CHECK(exact != NULL && padded != NULL);
    if (exact == NULL || padded == NULL) {
        free(exact);
        free(padded);
        return;
    }
    if (length > 0) {
        memcpy(exact, text, length);
    }
    memcpy(padded, text, length);
    memcpy(padded + length, "2080/05/15", 10);
    padded[length + 13] = '\0';

    nepdate_date out = {0x12345678};
    const nepdate_status exact_status =
        lenient ? nepdate_parse_lenient(exact, length, &out) : nepdate_parse(exact, length, &out);
    if (exact_status != status) {
        (void)printf("parse mismatch on \"%.*s\" (len %zu): got %d, want %d\n", (int)length, text,
                     length, (int)exact_status, (int)status);
    }
    CHECK_EQ_INT(exact_status, status);
    if (status == NEPDATE_OK) {
        CHECK_EQ_INT(out.value, expected->value);
    } else {
        CHECK_EQ_INT(out.value, 0x12345678);
    }

    out.value = 0x12345678;
    const nepdate_status padded_status =
        lenient ? nepdate_parse_lenient(padded, length, &out) : nepdate_parse(padded, length, &out);
    CHECK_EQ_INT(padded_status, status);
    if (status == NEPDATE_OK) {
        CHECK_EQ_INT(out.value, expected->value);
    } else {
        CHECK_EQ_INT(out.value, 0x12345678);
    }
    free(exact);
    free(padded);
}

TEST(golden_parse_rows) {
    vector_reader reader;
    CHECK(vector_open(&reader, "parse.tsv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    int rows = 0;
    int strict_tests = 0;
    int lenient_tests = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[4];
        const int count = vector_split(line, '\t', fields, 4);
        CHECK_EQ_INT(count, 3);
        const char *input = fields[0];
        const size_t length = strlen(input);

        nepdate_status strict = NEPDATE_OK;
        nepdate_status lenient = NEPDATE_OK;
        if (strncmp(fields[1], "ERR:", 4) == 0) {
            strict = expected_status(fields[1] + 4);
        }
        if (strncmp(fields[2], "ERR:", 4) == 0) {
            lenient = expected_status(fields[2] + 4);
        }

        nepdate_date strict_date = {0};
        nepdate_date lenient_date = {0};
        if (strict == NEPDATE_OK) {
            int year = 0;
            int month = 0;
            int day = 0;
            CHECK(vector_parse_ymd(fields[1], &year, &month, &day));
            CHECK_EQ_INT(nepdate_from_bs(year, month, day, &strict_date), NEPDATE_OK);
        }
        if (lenient == NEPDATE_OK) {
            int year = 0;
            int month = 0;
            int day = 0;
            CHECK(vector_parse_ymd(fields[2], &year, &month, &day));
            CHECK_EQ_INT(nepdate_from_bs(year, month, day, &lenient_date), NEPDATE_OK);
        }

        check_parse(input, length, strict, &strict_date, false);
        check_parse(input, length, lenient, &lenient_date, true);
        if (strict == NEPDATE_OK) {
            strict_tests++;
        }
        if (lenient == NEPDATE_OK) {
            lenient_tests++;
        }
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 69);
    CHECK_EQ_INT(strict_tests, 20);
    CHECK_EQ_INT(lenient_tests, 39);
    (void)printf("parse rows: %d, strict dates %d, lenient dates %d\n", rows, strict_tests,
                 lenient_tests);
}

TEST(shorter_length_stops_at_the_length) {
    static const char text[] = "2080/05/15";
    nepdate_date out = {0};
    CHECK_EQ_INT(nepdate_parse(text, 0, &out), NEPDATE_E_WRONG_GROUP_COUNT);
    for (size_t length = 1; length <= 8; length++) {
        out.value = 0;
        CHECK_EQ_INT(nepdate_parse(text, length, &out), NEPDATE_E_WRONG_GROUP_COUNT);
    }
    CHECK_EQ_INT(nepdate_parse(text, 9, &out), NEPDATE_OK);
    nepdate_ymd ymd = {0};
    CHECK_EQ_INT(nepdate_to_bs(out, &ymd), NEPDATE_OK);
    CHECK(ymd.year == 2080 && ymd.month == 5 && ymd.day == 1);
    CHECK_EQ_INT(nepdate_parse(text, 10, &out), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_to_bs(out, &ymd), NEPDATE_OK);
    CHECK(ymd.year == 2080 && ymd.month == 5 && ymd.day == 15);
}

TEST(null_arguments) {
    nepdate_date out = {0};
    CHECK_EQ_INT(nepdate_parse(NULL, 0, &out), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_parse(NULL, 3, &out), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_parse("2080/05/15", 10, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_parse_lenient(NULL, 0, &out), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_parse_lenient("2080/05/15", 10, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_parse("", 0, &out), NEPDATE_E_WRONG_GROUP_COUNT);
    CHECK_EQ_INT(nepdate_parse_lenient("", 0, &out), NEPDATE_E_UNRECOGNIZED);
}

TEST(edge_cases) {
    nepdate_date out = {0};
    CHECK_EQ_INT(nepdate_parse("\xE0", 1, &out), NEPDATE_E_INVALID_CHARACTER);
    CHECK_EQ_INT(nepdate_parse("\xE0\xA5", 2, &out), NEPDATE_E_INVALID_CHARACTER);
    CHECK_EQ_INT(nepdate_parse("\xE0\xA5\xA5", 3, &out), NEPDATE_E_INVALID_CHARACTER);
    CHECK_EQ_INT(nepdate_parse("\xE0\xA5\xB0", 3, &out), NEPDATE_E_INVALID_CHARACTER);
    CHECK_EQ_INT(nepdate_parse("\xE2\x84", 2, &out), NEPDATE_E_INVALID_CHARACTER);
    CHECK_EQ_INT(nepdate_parse("\xE2\x84\xAB", 3, &out), NEPDATE_E_INVALID_CHARACTER);

    CHECK_EQ_INT(nepdate_parse_lenient("\xE0", 1, &out), NEPDATE_E_UNRECOGNIZED);
    CHECK_EQ_INT(nepdate_parse_lenient("\xE0\xA5\xA5", 3, &out), NEPDATE_E_UNRECOGNIZED);
    CHECK_EQ_INT(nepdate_parse_lenient("\xE0\xA5\xB0", 3, &out), NEPDATE_E_UNRECOGNIZED);
    CHECK_EQ_INT(nepdate_parse_lenient("\xE2\x84", 2, &out), NEPDATE_E_UNRECOGNIZED);
    CHECK_EQ_INT(nepdate_parse_lenient("\xE2\x84\xAB", 3, &out), NEPDATE_E_UNRECOGNIZED);
    CHECK_EQ_INT(nepdate_parse_lenient("\xE2\x84\xAA", 3, &out), NEPDATE_E_UNRECOGNIZED);

    const char *nepali = "15 साउन २०८०।";
    CHECK_EQ_INT(nepdate_parse_lenient(nepali, strlen(nepali), &out), NEPDATE_OK);
    nepdate_ymd ymd = {0};
    CHECK_EQ_INT(nepdate_to_bs(out, &ymd), NEPDATE_OK);
    CHECK(ymd.year == 2080 && ymd.month == 4 && ymd.day == 15);

    CHECK_EQ_INT(nepdate_parse_lenient("123 Shrawan 2080", strlen("123 Shrawan 2080"), &out),
                 NEPDATE_E_AMBIGUOUS);
    CHECK_EQ_INT(nepdate_parse_lenient("2080 Shrawan 2081", strlen("2080 Shrawan 2081"), &out),
                 NEPDATE_E_AMBIGUOUS);
    CHECK_EQ_INT(nepdate_parse_lenient("Shrawan 2080", strlen("Shrawan 2080"), &out),
                 NEPDATE_E_UNRECOGNIZED);
    CHECK_EQ_INT(nepdate_parse_lenient("2080..05..15 BS", strlen("2080..05..15 BS"), &out),
                 NEPDATE_OK);
    CHECK_EQ_INT(nepdate_parse_lenient("1 2 3 4", strlen("1 2 3 4"), &out), NEPDATE_E_UNRECOGNIZED);
    CHECK_EQ_INT(nepdate_parse_lenient("15 ! 2080", strlen("15 ! 2080"), &out),
                 NEPDATE_E_UNRECOGNIZED);
    CHECK_EQ_INT(nepdate_parse_lenient("2080.05.15.99 BS", strlen("2080.05.15.99 BS"), &out),
                 NEPDATE_E_UNRECOGNIZED);
}

int main(void) {
    RUN_TEST(golden_parse_rows);
    RUN_TEST(edge_cases);
    RUN_TEST(shorter_length_stops_at_the_length);
    RUN_TEST(null_arguments);
    return harness_finish();
}
