/* libFuzzer target for nepdate_from_unix and nepdate_today_nepal. The input is 16 bytes: eight
 * little-endian seconds, then four little-endian offset minutes. A successful conversion must
 * round-trip through nepdate_to_bs, and the only error is out of range. */
#include <nepdate.h>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* assert() vanishes under NDEBUG, so the checks call abort() directly. */
static void fuzz_check(bool condition) {
    if (!condition) {
        abort();
    }
}

static int64_t read_i64(const uint8_t *data) {
    uint64_t value = 0;
    for (size_t i = 0; i < 8; i++) {
        value |= (uint64_t)data[i] << (8U * i);
    }
    int64_t signed_value = 0;
    memcpy(&signed_value, &value, sizeof signed_value);
    return signed_value;
}

static int32_t read_i32(const uint8_t *data) {
    uint32_t value = 0;
    for (size_t i = 0; i < 4; i++) {
        value |= (uint32_t)data[i] << (8U * i);
    }
    int32_t signed_value = 0;
    memcpy(&signed_value, &value, sizeof signed_value);
    return signed_value;
}

static void check_status(nepdate_status status) {
    fuzz_check(status == NEPDATE_OK || status == NEPDATE_E_OUT_OF_RANGE);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    uint8_t input[16] = {0};
    memcpy(input, data, size < sizeof input ? size : sizeof input);
    const int64_t seconds = read_i64(input);
    const int32_t offset = read_i32(input + 8);

    nepdate_date date = {0};
    const nepdate_status status = nepdate_from_unix(seconds, offset, &date);
    check_status(status);
    if (status == NEPDATE_OK) {
        nepdate_ymd ymd = {0, 0, 0};
        fuzz_check(nepdate_to_bs(date, &ymd) == NEPDATE_OK);
        nepdate_date back = {0};
        fuzz_check(nepdate_from_bs(ymd.year, ymd.month, ymd.day, &back) == NEPDATE_OK);
        fuzz_check(back.value == date.value);
    }

    nepdate_date today = {0};
    const nepdate_status today_status = nepdate_today_nepal(seconds, &today);
    check_status(today_status);
    if (today_status == NEPDATE_OK) {
        nepdate_ymd ymd = {0, 0, 0};
        fuzz_check(nepdate_to_bs(today, &ymd) == NEPDATE_OK);
        nepdate_date back = {0};
        fuzz_check(nepdate_from_bs(ymd.year, ymd.month, ymd.day, &back) == NEPDATE_OK);
        fuzz_check(back.value == today.value);
    }
    return 0;
}
