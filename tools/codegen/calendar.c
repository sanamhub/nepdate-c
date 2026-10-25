#include "calendar.h"

#include "json.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MIN_MONTH_LENGTH = 29, MAX_MONTH_LENGTH = 32 };

static bool fail(char *error, size_t error_size, const char *format, ...) {
    if (error_size > 0) {
        const int prefix = snprintf(error, error_size, "bs-calendar.json: ");
        if (prefix > 0 && (size_t)prefix < error_size) {
            va_list args;
            va_start(args, format);
            (void)vsnprintf(error + prefix, error_size - (size_t)prefix, format, args);
            va_end(args);
        }
    }
    return false;
}

static bool has_integer(const json_value *object, const char *key, long long expected) {
    const json_value *value = json_get(object, key);
    return value != NULL && value->type == JSON_NUMBER && value->is_integer &&
           value->integer == expected;
}

static bool has_string(const json_value *object, const char *key, const char *expected) {
    const json_value *value = json_get(object, key);
    return value != NULL && value->type == JSON_STRING && strcmp(value->string, expected) == 0;
}

static bool check_document(const json_value *doc, codegen_calendar *out, char *error,
                           size_t error_size) {
    if (doc->type != JSON_OBJECT) {
        return fail(error, error_size, "not a JSON object");
    }
    if (!has_integer(doc, "min_year", CODEGEN_MIN_YEAR)) {
        return fail(error, error_size, "min_year must be %d", CODEGEN_MIN_YEAR);
    }
    if (!has_integer(doc, "max_year", CODEGEN_MAX_YEAR)) {
        return fail(error, error_size, "max_year must be %d", CODEGEN_MAX_YEAR);
    }
    const json_value *epoch = json_get(doc, "epoch");
    if (!has_string(epoch, "bs", "1901-01-01") || !has_string(epoch, "ad", "1844-04-11")) {
        return fail(error, error_size, "epoch must be bs 1901-01-01, ad 1844-04-11");
    }
    const json_value *lengths = json_get(doc, "month_lengths");
    if (lengths == NULL || lengths->type != JSON_OBJECT || lengths->count != CODEGEN_YEARS) {
        return fail(error, error_size, "month_lengths must hold the %d years %d to %d",
                    CODEGEN_YEARS, CODEGEN_MIN_YEAR, CODEGEN_MAX_YEAR);
    }
    /* 299 members and every year found means no year is missing or repeated. */
    for (int year = CODEGEN_MIN_YEAR; year <= CODEGEN_MAX_YEAR; year++) {
        char key[8];
        (void)snprintf(key, sizeof key, "%d", year);
        const json_value *months = json_get(lengths, key);
        if (months == NULL || months->type != JSON_ARRAY || months->count != CODEGEN_MONTHS) {
            return fail(error, error_size, "year %d: expected %d month lengths", year,
                        CODEGEN_MONTHS);
        }
        for (size_t m = 0; m < CODEGEN_MONTHS; m++) {
            const json_value *length = &months->items[m];
            const int month = (int)m + 1;
            if (length->type != JSON_NUMBER || !length->is_integer) {
                return fail(error, error_size, "year %d month %d: length is not an integer", year,
                            month);
            }
            if (length->integer < MIN_MONTH_LENGTH || length->integer > MAX_MONTH_LENGTH) {
                return fail(error, error_size, "year %d month %d: length %lld is outside %d..%d",
                            year, month, length->integer, MIN_MONTH_LENGTH, MAX_MONTH_LENGTH);
            }
            out->month_lengths[year - CODEGEN_MIN_YEAR][m] = (unsigned char)length->integer;
        }
    }
    out->min_year = CODEGEN_MIN_YEAR;
    out->max_year = CODEGEN_MAX_YEAR;
    return true;
}

bool codegen_parse_calendar(const char *json_text, size_t length, codegen_calendar *out,
                            char *error, size_t error_size) {
    char json_error[160];
    json_value *doc = json_parse(json_text, length, json_error, sizeof json_error);
    if (doc == NULL) {
        return fail(error, error_size, "%s", json_error);
    }
    const bool ok = check_document(doc, out, error, error_size);
    json_free(doc);
    return ok && codegen_derive_tables(out, error, error_size);
}

/* Floor division, so the function is right for years before 1 too. */
static long floor_div(long a, long b) { return (a >= 0 ? a : a - (b - 1)) / b; }

long codegen_days_from_civil(long year, long month, long day) {
    const long y = month <= 2 ? year - 1 : year;
    const long era = floor_div(y, 400);
    const long yoe = y - era * 400;
    const long mp = (month + 9) % 12;
    const long doy = (153 * mp + 2) / 5 + day - 1;
    const long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

bool codegen_derive_tables(codegen_calendar *calendar, char *error, size_t error_size) {
    long start = 0;
    for (int i = 0; i < CODEGEN_ALL_MONTHS; i++) {
        if (i % CODEGEN_MONTHS == 0) {
            calendar->year_start[i / CODEGEN_MONTHS] = start;
        }
        calendar->month_start[i] = start;
        start += calendar->month_lengths[i / CODEGEN_MONTHS][i % CODEGEN_MONTHS];
    }
    calendar->month_start[CODEGEN_ALL_MONTHS] = start;
    calendar->year_start[CODEGEN_YEARS] = start;

    calendar->epoch_unix_days = codegen_days_from_civil(1844, 4, 11);

    uint64_t hash = UINT64_C(0xcbf29ce484222325);
    for (int i = 0; i < CODEGEN_ALL_MONTHS; i++) {
        hash = (hash ^ calendar->month_lengths[i / CODEGEN_MONTHS][i % CODEGEN_MONTHS]) *
               UINT64_C(0x100000001b3);
    }
    calendar->fingerprint = hash;

    for (int j = 0; j < CODEGEN_AD_MONTHS; j++) {
        const int k = j + CODEGEN_AD_FIRST_MONTH - 1;
        calendar->ad_month_start[j] =
            codegen_days_from_civil(CODEGEN_AD_FIRST_YEAR + k / 12, k % 12 + 1, 1) -
            calendar->epoch_unix_days;
    }

    /* The library finds the month of a day count with one multiplication and at most one step
     * either way (ADR-0006); every day of the calendar must keep to that. */
    /* 64-bit: the shifted month count does not fit in a 32-bit long. */
    calendar->estimate_mul =
        (unsigned long)(((uint64_t)CODEGEN_ALL_MONTHS << CODEGEN_ESTIMATE_SHIFT) / (uint64_t)start);
    int month = 0;
    for (long s = 0; s < start; s++) {
        while (calendar->month_start[month + 1] <= s) {
            month++;
        }
        const uint64_t product = (uint64_t)s * calendar->estimate_mul;
        const long estimate = (long)(product >> CODEGEN_ESTIMATE_SHIFT);
        if (estimate < month - 1 || estimate > month + 1 || estimate >= CODEGEN_ALL_MONTHS) {
            return fail(error, error_size,
                        "year %d month %d: the month estimate for day %ld is %ld months off",
                        CODEGEN_MIN_YEAR + month / CODEGEN_MONTHS, month % CODEGEN_MONTHS + 1, s,
                        estimate - month);
        }
    }
    return true;
}

/* A growing string. A failed allocation leaves text NULL, and every later append does nothing. */
typedef struct {
    char *text;
    size_t length;
    size_t capacity;
} builder;

static void builder_drop(builder *b) {
    free(b->text);
    b->text = NULL;
}

static void append(builder *b, const char *format, ...) {
    if (b->text == NULL) {
        return;
    }
    va_list args;
    va_start(args, format);
    va_list copy;
    va_copy(copy, args);
    const int n = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (n < 0) {
        va_end(args);
        builder_drop(b);
        return;
    }
    const size_t needed = b->length + (size_t)n + 1;
    if (needed > b->capacity) {
        size_t capacity = b->capacity * 2;
        while (capacity < needed) {
            capacity *= 2;
        }
        char *bigger = realloc(b->text, capacity);
        if (bigger == NULL) {
            va_end(args);
            builder_drop(b);
            return;
        }
        b->text = bigger;
        b->capacity = capacity;
    }
    (void)vsnprintf(b->text + b->length, b->capacity - b->length, format, args);
    va_end(args);
    b->length += (size_t)n;
}

/* One table: first_line values on the first line, then per_line a line, each line ending in a
 * comment with the year it starts at (first_year, plus years_per_line for each line). */
typedef struct {
    const char *type;
    const char *name;
    const long *values;
    int count;
    int first_line;
    int per_line;
    int first_year;
    int years_per_line;
} table;

static void append_table(builder *b, const table *t) {
    append(b, "static const %s %s[%d] = {\n", t->type, t->name, t->count);
    int i = 0;
    for (int line = 0; i < t->count; line++) {
        const int on_line = line == 0 ? t->first_line : t->per_line;
        append(b, "   ");
        for (int k = 0; k < on_line && i < t->count; k++) {
            append(b, " %ld,", t->values[i]);
            i++;
        }
        append(b, " /* %d */\n", t->first_year + line * t->years_per_line);
    }
    append(b, "};\n");
}

char *codegen_render_calendar_tables(const codegen_calendar *calendar) {
    enum { INITIAL_CAPACITY = 65536 };
    builder b = {malloc(INITIAL_CAPACITY), 0, INITIAL_CAPACITY};
    append(&b, "/* Generated by tools/codegen from shared/data. DO NOT EDIT. */\n"
               "/* Source: shared/data/calendar/bs-calendar.json. Layout and use: ADR-0006. */\n"
               "#ifndef NEPDATE_GENERATED_CALENDAR_TABLES_H\n"
               "#define NEPDATE_GENERATED_CALENDAR_TABLES_H\n"
               "\n"
               "#include <stdint.h>\n"
               "\n");
    append(&b, "#define NEPDATE_CAL_MIN_YEAR %d\n", calendar->min_year);
    append(&b, "#define NEPDATE_CAL_MAX_YEAR %d\n", calendar->max_year);
    append(&b,
           "/* Days in the calendar: day counts run from 0 (BS %d-01-01) to this minus 1. */\n"
           "#define NEPDATE_CAL_DAYS %ld\n",
           calendar->min_year, calendar->month_start[CODEGEN_ALL_MONTHS]);
    append(&b,
           "/* Unix day of the epoch, AD 1844-04-11. */\n"
           "#define NEPDATE_CAL_EPOCH_UNIX_DAYS (%ld)\n",
           calendar->epoch_unix_days);
    append(&b,
           "/* FNV-1a 64 of the 3588 month lengths, one byte each; the C++ port builds the same\n"
           " * value from the same file. */\n"
           "#define NEPDATE_CAL_FINGERPRINT 0x%016llxULL\n",
           (unsigned long long)calendar->fingerprint);
    append(&b,
           "/* Entry 0 of nepdate_cal_ad_month_start is AD %d-%02d. */\n"
           "#define NEPDATE_CAL_AD_FIRST_YEAR %d\n"
           "#define NEPDATE_CAL_AD_FIRST_MONTH %d\n"
           "#define NEPDATE_CAL_AD_MONTHS %d\n",
           CODEGEN_AD_FIRST_YEAR, CODEGEN_AD_FIRST_MONTH, CODEGEN_AD_FIRST_YEAR,
           CODEGEN_AD_FIRST_MONTH, CODEGEN_AD_MONTHS);
    append(&b,
           "/* (s * MUL) >> SHIFT is the month index of day count s, or one more or one less;\n"
           " * the generator checks every day. */\n"
           "#define NEPDATE_CAL_ESTIMATE_MUL %luU\n"
           "#define NEPDATE_CAL_ESTIMATE_SHIFT %d\n"
           "\n",
           calendar->estimate_mul, CODEGEN_ESTIMATE_SHIFT);

    long lengths[CODEGEN_ALL_MONTHS];
    for (int i = 0; i < CODEGEN_ALL_MONTHS; i++) {
        lengths[i] = calendar->month_lengths[i / CODEGEN_MONTHS][i % CODEGEN_MONTHS];
    }
    const table tables[] = {
        {"uint8_t", "nepdate_cal_month_length", lengths, CODEGEN_ALL_MONTHS, CODEGEN_MONTHS,
         CODEGEN_MONTHS, CODEGEN_MIN_YEAR, 1},
        {"uint32_t", "nepdate_cal_month_start", calendar->month_start, CODEGEN_ALL_MONTHS + 1,
         CODEGEN_MONTHS, CODEGEN_MONTHS, CODEGEN_MIN_YEAR, 1},
        {"uint32_t", "nepdate_cal_year_start", calendar->year_start, CODEGEN_YEARS + 1, 10, 10,
         CODEGEN_MIN_YEAR, 10},
        {"int32_t", "nepdate_cal_ad_month_start", calendar->ad_month_start, CODEGEN_AD_MONTHS,
         CODEGEN_MONTHS - CODEGEN_AD_FIRST_MONTH + 1, CODEGEN_MONTHS, CODEGEN_AD_FIRST_YEAR, 1},
    };
    static const char *const comments[] = {
        "Days in month index (year - 1901) * 12 + month - 1.",
        "Day count of the first day of each month index; the last entry is NEPDATE_CAL_DAYS.",
        "Day count of 1 Baishakh of year 1901 + i; the last entry is NEPDATE_CAL_DAYS.",
        "Day count of the first day of AD month index (year - 1844) * 12 + month - 4.",
    };
    for (size_t i = 0; i < sizeof tables / sizeof tables[0]; i++) {
        append(&b, "%s/* %s */\n", i == 0 ? "" : "\n", comments[i]);
        append_table(&b, &tables[i]);
    }
    append(&b, "\n#endif /* NEPDATE_GENERATED_CALENDAR_TABLES_H */\n");
    return b.text;
}
