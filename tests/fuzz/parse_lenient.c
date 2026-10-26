/* libFuzzer target for nepdate_parse_lenient. Every input must give a date this library made or one
 * of the documented errors, and a successful parse must round-trip through nepdate_to_bs. */
#include <nepdate.h>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/* assert() vanishes under NDEBUG, so the checks call abort() directly. */
static void fuzz_check(bool condition) {
    if (!condition) {
        abort();
    }
}

static bool known_error(nepdate_status status) {
    switch (status) {
    case NEPDATE_E_OUT_OF_RANGE:
    case NEPDATE_E_INVALID_MONTH:
    case NEPDATE_E_INVALID_DAY:
    case NEPDATE_E_INVALID_CHARACTER:
    case NEPDATE_E_WRONG_GROUP_COUNT:
    case NEPDATE_E_NUMBER_TOO_LONG:
    case NEPDATE_E_AMBIGUOUS:
    case NEPDATE_E_UNRECOGNIZED:
        return true;
    default:
        return false;
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    nepdate_date date = {0x12345678};
    const nepdate_status status = nepdate_parse_lenient((const char *)data, size, &date);
    if (status == NEPDATE_OK) {
        nepdate_ymd ymd = {0, 0, 0};
        fuzz_check(nepdate_to_bs(date, &ymd) == NEPDATE_OK);
        nepdate_date back = {0};
        fuzz_check(nepdate_from_bs(ymd.year, ymd.month, ymd.day, &back) == NEPDATE_OK);
        fuzz_check(back.value == date.value);
    } else {
        fuzz_check(known_error(status));
        fuzz_check(date.value == 0x12345678);
    }
    return 0;
}
