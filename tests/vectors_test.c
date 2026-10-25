/* The shared vectors (month boundaries, C# golden dates) and the exhaustive round trip. */
#include "harness.h"
#include "hinnant_reference.h"
#include "vectors.h"

#include <nepdate.h>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { MAX_COLUMNS = 32, EPOCH_UNIX_DAYS = -45920 };

static int column(char **header, int count, const char *name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(header[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

static nepdate_date bs_date(int year, int month, int day) {
    nepdate_date date = {0};
    CHECK_EQ_INT(nepdate_from_bs(year, month, day, &date), NEPDATE_OK);
    return date;
}

static int number(const char *text) { return (int)strtol(text, NULL, 10); }

TEST(month_boundaries) {
    vector_reader reader;
    CHECK(vector_open(&reader, "month-boundaries.csv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    if (line == NULL) {
        vector_close(&reader);
        return;
    }
    char *header[MAX_COLUMNS];
    const int header_count = vector_split(line, ',', header, MAX_COLUMNS);
    const int bs_column = column(header, header_count, "bs");
    const int ad_column = column(header, header_count, "ad");
    const int weekday_column = column(header, header_count, "weekday");
    const int length_column = column(header, header_count, "month_length");
    CHECK(bs_column >= 0 && ad_column >= 0 && weekday_column >= 0 && length_column >= 0);

    int rows = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[MAX_COLUMNS];
        const int count = vector_split(line, ',', fields, MAX_COLUMNS);
        if (count != 4) {
            continue;
        }
        int by = 0;
        int bm = 0;
        int bd = 0;
        int ay = 0;
        int am = 0;
        int ad = 0;
        CHECK(vector_parse_ymd(fields[bs_column], &by, &bm, &bd));
        CHECK(vector_parse_ymd(fields[ad_column], &ay, &am, &ad));
        const nepdate_date date = bs_date(by, bm, bd);
        nepdate_ymd got_ad = {0, 0, 0};
        CHECK_EQ_INT(nepdate_to_ad(date, &got_ad), NEPDATE_OK);
        CHECK(got_ad.year == ay && got_ad.month == am && got_ad.day == ad);
        nepdate_date back = {0};
        CHECK_EQ_INT(nepdate_from_ad(ay, am, ad, &back), NEPDATE_OK);
        CHECK_EQ_INT(back.value, date.value);
        nepdate_weekday weekday = NEPDATE_SUNDAY;
        CHECK_EQ_INT(nepdate_weekday_of(date, &weekday), NEPDATE_OK);
        CHECK_EQ_INT(weekday, vector_weekday(fields[weekday_column]));
        int length = 0;
        CHECK_EQ_INT(nepdate_month_length(date, &length), NEPDATE_OK);
        CHECK_EQ_INT(length, number(fields[length_column]));
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 7176);
}

TEST(csharp_golden_dates) {
    vector_reader reader;
    CHECK(vector_open(&reader, "csharp-golden/dates.tsv"));
    char *line = vector_next(&reader);
    CHECK(line != NULL);
    if (line == NULL) {
        vector_close(&reader);
        return;
    }
    char *header[MAX_COLUMNS];
    const int header_count = vector_split(line, '\t', header, MAX_COLUMNS);
    const int bs_column = column(header, header_count, "bs");
    const int ad_column = column(header, header_count, "ad");
    const int weekday_column = column(header, header_count, "weekday");
    const int day_of_year_column = column(header, header_count, "day_of_year");
    const int length_column = column(header, header_count, "month_length");
    CHECK(bs_column >= 0 && ad_column >= 0 && weekday_column >= 0 && day_of_year_column >= 0 &&
          length_column >= 0);

    int rows = 0;
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[MAX_COLUMNS];
        const int count = vector_split(line, '\t', fields, MAX_COLUMNS);
        if (count != header_count) {
            continue;
        }
        int by = 0;
        int bm = 0;
        int bd = 0;
        int ay = 0;
        int am = 0;
        int ad = 0;
        CHECK(vector_parse_ymd(fields[bs_column], &by, &bm, &bd));
        CHECK(vector_parse_ymd(fields[ad_column], &ay, &am, &ad));
        const nepdate_date date = bs_date(by, bm, bd);
        nepdate_ymd got_ad = {0, 0, 0};
        CHECK_EQ_INT(nepdate_to_ad(date, &got_ad), NEPDATE_OK);
        CHECK(got_ad.year == ay && got_ad.month == am && got_ad.day == ad);
        nepdate_weekday weekday = NEPDATE_SUNDAY;
        CHECK_EQ_INT(nepdate_weekday_of(date, &weekday), NEPDATE_OK);
        CHECK_EQ_INT(weekday, vector_weekday(fields[weekday_column]));
        int day_of_year = 0;
        CHECK_EQ_INT(nepdate_day_of_year(date, &day_of_year), NEPDATE_OK);
        CHECK_EQ_INT(day_of_year, number(fields[day_of_year_column]));
        int length = 0;
        CHECK_EQ_INT(nepdate_month_length(date, &length), NEPDATE_OK);
        CHECK_EQ_INT(length, number(fields[length_column]));
        rows++;
    }
    vector_close(&reader);
    CHECK_EQ_INT(rows, 1018);
}

/* ALGORITHM section 2: build LEN, PACKED and YEAR_START from the vector's month_length column. */
static bool read_month_lengths(int len[299][12]) {
    vector_reader reader;
    if (!vector_open(&reader, "month-boundaries.csv")) {
        return false;
    }
    char *line = vector_next(&reader);
    if (line == NULL) {
        vector_close(&reader);
        return false;
    }
    char *header[MAX_COLUMNS];
    const int header_count = vector_split(line, ',', header, MAX_COLUMNS);
    const int bs_column = column(header, header_count, "bs");
    const int length_column = column(header, header_count, "month_length");
    if (bs_column < 0 || length_column < 0) {
        vector_close(&reader);
        return false;
    }
    while ((line = vector_next(&reader)) != NULL) {
        char *fields[MAX_COLUMNS];
        if (vector_split(line, ',', fields, MAX_COLUMNS) != 4) {
            continue;
        }
        int y = 0;
        int m = 0;
        int d = 0;
        if (vector_parse_ymd(fields[bs_column], &y, &m, &d) && y >= 1901 && y <= 2199 && m >= 1 &&
            m <= 12) {
            len[y - 1901][m - 1] = number(fields[length_column]);
        }
    }
    vector_close(&reader);
    return true;
}

static bool packed_tables(int len[299][12], uint32_t packed[299], uint32_t year_start[300]) {
    for (int i = 0; i < 299; i++) {
        uint32_t bits = 0;
        uint32_t sum = 0;
        for (int m = 0; m < 12; m++) {
            const int value = len[i][m];
            if (value < 29 || value > 32) {
                return false;
            }
            bits |= (uint32_t)(value - 29) << (2 * m);
            sum += (uint32_t)value;
        }
        packed[i] = bits;
        year_start[i + 1] = year_start[i] + sum;
    }
    return true;
}

static void check_month_lengths(const uint32_t packed[299]) {
    for (int i = 0; i < 299; i++) {
        for (int m = 1; m <= 12; m++) {
            const nepdate_date date = bs_date(1901 + i, m, 1);
            int length = 0;
            CHECK_EQ_INT(nepdate_month_length(date, &length), NEPDATE_OK);
            CHECK_EQ_INT(length, 29 + (int)((packed[i] >> (2 * (m - 1))) & 3U));
        }
    }
}

static void check_day_of_year(int len[299][12], const uint32_t year_start[300]) {
    for (int i = 0; i < 299; i++) {
        for (int m = 0; m < 12; m++) {
            for (int d = 1; d <= len[i][m]; d++) {
                const nepdate_date date = bs_date(1901 + i, m + 1, d);
                int day_of_year = 0;
                CHECK_EQ_INT(nepdate_day_of_year(date, &day_of_year), NEPDATE_OK);
                CHECK_EQ_INT(day_of_year, (int)((uint32_t)(date.value - 1) - year_start[i] + 1U));
            }
        }
    }
}

TEST(tables_agree_with_the_spec) {
    static int len[299][12];
    static uint32_t packed[299];
    static uint32_t year_start[300];
    const bool read = read_month_lengths(len);
    CHECK(read);
    if (!read) {
        return;
    }
    const bool built = packed_tables(len, packed, year_start);
    CHECK(built);
    if (!built) {
        return;
    }
    CHECK_EQ_INT(year_start[299], 109212);
    check_month_lengths(packed);
    check_day_of_year(len, year_start);
}

TEST(every_serial_round_trips) {
    /* AD 1844-04-11 is Unix day -45920 (ALGORITHM section 2). */
    const int epoch_days = EPOCH_UNIX_DAYS;
    for (int s = 0; s <= 109211; s++) {
        const nepdate_date date = {s + 1};
        nepdate_ymd bs = {0, 0, 0};
        CHECK_EQ_INT(nepdate_to_bs(date, &bs), NEPDATE_OK);
        nepdate_date back = {0};
        CHECK_EQ_INT(nepdate_from_bs(bs.year, bs.month, bs.day, &back), NEPDATE_OK);
        CHECK_EQ_INT(back.value, date.value);

        nepdate_ymd ad = {0, 0, 0};
        CHECK_EQ_INT(nepdate_to_ad(date, &ad), NEPDATE_OK);
        const reference_civil reference = reference_civil_from_days(s + epoch_days);
        CHECK(ad.year == reference.year && ad.month == reference.month && ad.day == reference.day);
        CHECK_EQ_INT(reference_days_from_civil(reference.year, reference.month, reference.day),
                     s + epoch_days);
        nepdate_date from_ad = {0};
        CHECK_EQ_INT(nepdate_from_ad(ad.year, ad.month, ad.day, &from_ad), NEPDATE_OK);
        CHECK_EQ_INT(from_ad.value, date.value);

        nepdate_weekday weekday = NEPDATE_SUNDAY;
        CHECK_EQ_INT(nepdate_weekday_of(date, &weekday), NEPDATE_OK);
        CHECK_EQ_INT(weekday, ((s + epoch_days) % 7 + 7 + 4) % 7);
    }
}

int main(void) {
    RUN_TEST(month_boundaries);
    RUN_TEST(csharp_golden_dates);
    RUN_TEST(tables_agree_with_the_spec);
    RUN_TEST(every_serial_round_trips);
    return harness_finish();
}
