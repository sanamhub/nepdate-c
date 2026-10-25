/* libFuzzer target for the formatters. The first byte picks the language, the next four pick a
 * valid date, and the rest is the pattern. Every write must return the full length, NUL-terminate
 * when it has room and leave the bytes past the terminator alone. */
#include <nepdate.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static nepdate_date make_date(uint32_t seed) {
    const int year = NEPDATE_MIN_YEAR + (int)(seed % 299U);
    const int month = 1 + (int)((seed / 299U) % 12U);
    const int day = 1 + (int)((seed / (299U * 12U)) % 28U);
    nepdate_date date = {0};
    assert(nepdate_from_bs(year, month, day, &date) == NEPDATE_OK);
    return date;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0) {
        return 0;
    }
    const nepdate_lang lang = (data[0] & 1U) != 0 ? NEPDATE_NEPALI : NEPDATE_ENGLISH;
    uint32_t seed = 1;
    for (size_t i = 1; i < size && i < 5; i++) {
        seed = seed * 31U + data[i];
    }
    const nepdate_date date = make_date(seed);

    const size_t pattern_length = size > 5 ? size - 5 : 0;
    char *pattern = malloc(pattern_length + 1);
    assert(pattern != NULL);
    if (pattern_length > 0) {
        memcpy(pattern, data + (size > 5 ? 5 : size), pattern_length);
    }
    pattern[pattern_length] = '\0';

    const size_t needed = nepdate_format(date, pattern, lang, NULL, 0);
    assert(needed <= (1U << 20));
    char *buffer = malloc(needed + 1);
    assert(buffer != NULL);
    memset(buffer, 0xA5, needed + 1);
    assert(nepdate_format(date, pattern, lang, buffer, needed + 1) == needed);
    assert(buffer[needed] == '\0');
    if (needed > 0) {
        assert(nepdate_format(date, pattern, lang, buffer, needed / 2U + 1U) == needed);
        assert(buffer[needed / 2U] == '\0');
    }
    free(buffer);

    const nepdate_long_options options = {(data[0] & 2U) != 0, (data[0] & 4U) != 0,
                                          (data[0] & 8U) != 0};
    char long_buffer[128];
    const size_t long_length =
        nepdate_format_long(date, options, lang, long_buffer, sizeof long_buffer);
    assert(long_length > 0 && long_length < sizeof long_buffer);
    assert(long_buffer[long_length] == '\0');

    const nepdate_order order = (nepdate_order)((seed >> 3) % 6U);
    static const char separators[6] = {'/', '\\', '.', '_', '-', ' '};
    const nepdate_separator separator = (nepdate_separator)separators[(seed >> 5) % 6U];
    char short_buffer[64];
    const size_t short_length = nepdate_format_short(date, order, separator, (seed & 1U) != 0, lang,
                                                     short_buffer, sizeof short_buffer);
    assert(short_length > 0 && short_length < sizeof short_buffer);
    assert(short_buffer[short_length] == '\0');

    free(pattern);
    return 0;
}
