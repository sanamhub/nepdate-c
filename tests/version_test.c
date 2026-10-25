#include "harness.h"

#include <nepdate.h>

#include <stddef.h>
#include <stdio.h>

TEST(library_version_matches_header) { CHECK_EQ_STR(nepdate_version(), NEPDATE_VERSION_STRING); }

TEST(version_string_is_built_from_the_parts) {
    char expected[64];
    const char *prerelease = NEPDATE_VERSION_PRERELEASE;
    int length = 0;
    if (prerelease[0] == '\0') {
        length = snprintf(expected, sizeof expected, "%d.%d.%d", NEPDATE_VERSION_MAJOR,
                          NEPDATE_VERSION_MINOR, NEPDATE_VERSION_PATCH);
    } else {
        length = snprintf(expected, sizeof expected, "%d.%d.%d-%s", NEPDATE_VERSION_MAJOR,
                          NEPDATE_VERSION_MINOR, NEPDATE_VERSION_PATCH, prerelease);
    }
    CHECK(length > 0 && (size_t)length < sizeof expected);
    CHECK_EQ_STR(NEPDATE_VERSION_STRING, expected);
}

int main(void) {
    RUN_TEST(library_version_matches_header);
    RUN_TEST(version_string_is_built_from_the_parts);
    return harness_finish();
}
