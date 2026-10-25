#include "harness.h"

#include <nepdate.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

TEST(library_version_matches_header) { CHECK_EQ_STR(nepdate_version(), NEPDATE_VERSION_STRING); }

TEST(version_string_is_built_from_the_parts) {
    char core[32];
    const int length = snprintf(core, sizeof core, "%d.%d.%d", NEPDATE_VERSION_MAJOR,
                                NEPDATE_VERSION_MINOR, NEPDATE_VERSION_PATCH);
    CHECK(length > 0 && (size_t)length < sizeof core);
    const char *version = NEPDATE_VERSION_STRING;
    const size_t core_length = strlen(core);
    CHECK(strncmp(version, core, core_length) == 0);
    if (strlen(version) < core_length) {
        return;
    }
    /* After the core comes "-PRERELEASE", or nothing for a stable version, where the "-" is
     * skipped because the pre-release is empty. */
    char tail[64];
    (void)snprintf(tail, sizeof tail, "-%s", NEPDATE_VERSION_PRERELEASE);
    CHECK_EQ_STR(version + core_length, tail + (sizeof NEPDATE_VERSION_PRERELEASE == 1 ? 1 : 0));
}

int main(void) {
    RUN_TEST(library_version_matches_header);
    RUN_TEST(version_string_is_built_from_the_parts);
    return harness_finish();
}
