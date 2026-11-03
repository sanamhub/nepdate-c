/* Ordered batch conversion with caller buffers: every row of the bulk vector, then the argument,
 * capacity and order rules around it. */
#include "harness.h"
#include "vectors.h"

#include <nepdate.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAX_BATCH = 8 };

static void check_bs_date(nepdate_date actual, const char *expected_text) {
    nepdate_date expected = {0};
    CHECK(vector_parse_date(expected_text, &expected));
    CHECK_EQ_INT(actual.value, expected.value);
}

static void check_ad_date(nepdate_ymd actual, const char *expected_text) {
    int year = 0;
    int month = 0;
    int day = 0;
    CHECK(vector_parse_ymd(expected_text, &year, &month, &day));
    CHECK_EQ_INT(actual.year, year);
    CHECK_EQ_INT(actual.month, month);
    CHECK_EQ_INT(actual.day, day);
}

/* The AD dates of an input field, "1844-04-11;2024-07-31", as the array a bulk call takes. */
static int parse_ad_list(char *field, nepdate_ymd *out) {
    char *items[MAX_BATCH];
    const int count = vector_split(field, ';', items, MAX_BATCH);
    CHECK(count <= MAX_BATCH);
    for (int i = 0; i < count; i++) {
        int year = 0;
        int month = 0;
        int day = 0;
        CHECK(vector_parse_ymd(items[i], &year, &month, &day));
        out[i].year = year;
        out[i].month = month;
        out[i].day = day;
    }
    return count;
}

/* The BS dates of an input field, as the array a bulk call takes. */
static int parse_bs_list(char *field, nepdate_date *out) {
    char *items[MAX_BATCH];
    const int count = vector_split(field, ';', items, MAX_BATCH);
    CHECK(count <= MAX_BATCH);
    for (int i = 0; i < count; i++) {
        CHECK(vector_parse_date(items[i], &out[i]));
    }
    return count;
}

/* The texts of an input field, as the array a bulk parse takes. The bytes stay in the reader. */
static int parse_text_list(char *field, nepdate_text *out) {
    char *items[MAX_BATCH];
    const int count = vector_split(field, ';', items, MAX_BATCH);
    CHECK(count <= MAX_BATCH);
    for (int i = 0; i < count; i++) {
        out[i].text = items[i];
        out[i].length = strlen(items[i]);
    }
    return count;
}

/* One row whose output is a list of dates, checked element by element in input order. */
static void check_bulk_dates(const char *operation, char *input, char *expected) {
    size_t written = 0;
    char *expected_items[MAX_BATCH];
    if (strcmp(operation, "to_nepali_dates") == 0 ||
        strcmp(operation, "batch_to_nepali_dates") == 0) {
        nepdate_ymd ad[MAX_BATCH] = {{0, 0, 0}};
        const int count = parse_ad_list(input, ad);
        CHECK_EQ_INT(vector_split(expected, ';', expected_items, MAX_BATCH), count);
        nepdate_date out[MAX_BATCH] = {{0}};
        CHECK_EQ_INT(nepdate_bulk_to_bs(ad, (size_t)count, out, MAX_BATCH, &written), NEPDATE_OK);
        CHECK_EQ_INT(written, count);
        for (int i = 0; i < count; i++) {
            check_bs_date(out[i], expected_items[i]);
        }
        return;
    }

    int count = 0;
    nepdate_ymd out[MAX_BATCH] = {{0, 0, 0}};
    if (strcmp(operation, "to_english_dates_from_strings") == 0) {
        nepdate_text texts[MAX_BATCH] = {{NULL, 0}};
        count = parse_text_list(input, texts);
        CHECK_EQ_INT(nepdate_bulk_parse_to_ad(texts, (size_t)count, out, MAX_BATCH, &written),
                     NEPDATE_OK);
    } else if (strcmp(operation, "to_english_dates_from_dates") == 0 ||
               strcmp(operation, "batch_to_english_dates") == 0) {
        nepdate_date dates[MAX_BATCH] = {{0}};
        count = parse_bs_list(input, dates);
        CHECK_EQ_INT(nepdate_bulk_to_ad(dates, (size_t)count, out, MAX_BATCH, &written),
                     NEPDATE_OK);
    } else {
        FAIL("unknown bulk operation");
        return;
    }
    CHECK_EQ_INT(vector_split(expected, ';', expected_items, MAX_BATCH), count);
    CHECK_EQ_INT(written, count);
    for (int i = 0; i < count; i++) {
        check_ad_date(out[i], expected_items[i]);
    }
}

TEST(golden_bulk_rows) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/bulk.tsv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    int rows = 0;
    int rejected = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[5];
        CHECK_EQ_INT(vector_split(line, '\t', fields, 5), 4);
        if (strcmp(fields[2], "ERR") == 0) {
            /* One bad input fails the whole call and writes nothing. */
            size_t written = 99;
            nepdate_text texts[MAX_BATCH] = {{NULL, 0}};
            nepdate_ymd out[MAX_BATCH] = {{0, 0, 0}};
            const int count = parse_text_list(fields[1], texts);
            CHECK_EQ_INT(nepdate_bulk_parse_to_ad(texts, (size_t)count, out, MAX_BATCH, &written),
                         NEPDATE_E_INVALID_DAY);
            CHECK_EQ_INT(written, 99);
            rejected++;
        } else {
            check_bulk_dates(fields[0], fields[1], fields[2]);
        }
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 6);
    CHECK_EQ_INT(rejected, 1);
}

TEST(order_is_input_order) {
    /* The C# parallel path above 500 items does not promise an order; this port always keeps the
     * input order, so an unsorted input stays unsorted in the same way. */
    nepdate_date unsorted[3];
    CHECK_EQ_INT(nepdate_from_bs(2081, 4, 15, &unsorted[0]), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_from_bs(1901, 1, 1, &unsorted[1]), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_from_bs(2199, 12, 30, &unsorted[2]), NEPDATE_OK);
    nepdate_ymd ad[3] = {{0, 0, 0}};
    size_t written = 0;
    CHECK_EQ_INT(nepdate_bulk_to_ad(unsorted, 3, ad, 3, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 3);
    check_ad_date(ad[0], "2024-07-30");
    check_ad_date(ad[1], "1844-04-11");
    check_ad_date(ad[2], "2143-04-15");

    nepdate_ymd input[3] = {{2024, 7, 30}, {1844, 4, 11}, {2143, 4, 15}};
    nepdate_date bs[3] = {{0}};
    CHECK_EQ_INT(nepdate_bulk_to_bs(input, 3, bs, 3, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 3);
    check_bs_date(bs[0], "2081-04-15");
    check_bs_date(bs[1], "1901-01-01");
    check_bs_date(bs[2], "2199-12-30");
}

TEST(capacity_truncates_the_output) {
    nepdate_date input[3];
    CHECK_EQ_INT(nepdate_from_bs(2080, 1, 1, &input[0]), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_from_bs(2080, 1, 2, &input[1]), NEPDATE_OK);
    CHECK_EQ_INT(nepdate_from_bs(2080, 1, 3, &input[2]), NEPDATE_OK);

    nepdate_ymd ad[3] = {{7, 7, 7}, {7, 7, 7}, {7, 7, 7}};
    size_t written = 0;
    CHECK_EQ_INT(nepdate_bulk_to_ad(input, 3, ad, 2, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 3);
    check_ad_date(ad[0], "2023-04-14");
    check_ad_date(ad[1], "2023-04-15");
    CHECK_EQ_INT(ad[2].year, 7);
    CHECK_EQ_INT(ad[2].month, 7);
    CHECK_EQ_INT(ad[2].day, 7);

    /* capacity 0 with no buffer still reports how many results the call has. */
    written = 0;
    CHECK_EQ_INT(nepdate_bulk_to_ad(input, 3, NULL, 0, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 3);
}

TEST(empty_and_null_arguments) {
    size_t written = 99;
    CHECK_EQ_INT(nepdate_bulk_to_bs(NULL, 0, NULL, 0, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 0);
    CHECK_EQ_INT(nepdate_bulk_parse_to_ad(NULL, 0, NULL, 0, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 0);
    CHECK_EQ_INT(nepdate_bulk_to_ad(NULL, 0, NULL, 0, &written), NEPDATE_OK);
    CHECK_EQ_INT(written, 0);

    nepdate_date date = {0};
    nepdate_ymd ad = {2081, 4, 15};
    CHECK_EQ_INT(nepdate_bulk_to_bs(&ad, 1, &date, 1, NULL), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_bulk_to_bs(NULL, 1, &date, 1, &written), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_bulk_to_bs(&ad, 1, NULL, 1, &written), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_bulk_parse_to_ad(NULL, 1, NULL, 1, &written), NEPDATE_E_NULL);
    CHECK_EQ_INT(nepdate_bulk_to_ad(NULL, 1, NULL, 1, &written), NEPDATE_E_NULL);
}

TEST(a_failed_call_writes_nothing) {
    nepdate_ymd input[2] = {{2024, 2, 29}, {2023, 2, 29}};
    nepdate_date out[2] = {{7}, {8}};
    size_t written = 99;
    CHECK_EQ_INT(nepdate_bulk_to_bs(input, 2, out, 2, &written), NEPDATE_E_INVALID_GREGORIAN);
    CHECK_EQ_INT(out[0].value, 7);
    CHECK_EQ_INT(out[1].value, 8);
    CHECK_EQ_INT(written, 99);

    /* The first date converts, the second is outside the supported AD span. */
    input[1].year = 1800;
    input[1].month = 1;
    input[1].day = 1;
    CHECK_EQ_INT(nepdate_bulk_to_bs(input, 2, out, 2, &written), NEPDATE_E_OUT_OF_RANGE);
    CHECK_EQ_INT(out[0].value, 7);
    CHECK_EQ_INT(written, 99);

    const nepdate_text texts[2] = {{"2081-04-15", 10}, {"2081-01-32", 10}};
    nepdate_ymd ad[2] = {{7, 7, 7}, {8, 8, 8}};
    CHECK_EQ_INT(nepdate_bulk_parse_to_ad(texts, 2, ad, 2, &written), NEPDATE_E_INVALID_DAY);
    CHECK_EQ_INT(ad[0].year, 7);
    CHECK_EQ_INT(written, 99);

    const nepdate_date dates[2] = {{1}, {0}};
    CHECK_EQ_INT(nepdate_bulk_to_ad(dates, 2, ad, 2, &written), NEPDATE_E_INVALID_DATE);
    CHECK_EQ_INT(ad[0].year, 7);
    CHECK_EQ_INT(written, 99);
}

int main(void) {
    RUN_TEST(golden_bulk_rows);
    RUN_TEST(order_is_input_order);
    RUN_TEST(capacity_truncates_the_output);
    RUN_TEST(empty_and_null_arguments);
    RUN_TEST(a_failed_call_writes_nothing);
    return harness_finish();
}
