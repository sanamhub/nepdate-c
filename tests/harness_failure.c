/* Fails on purpose. tests/CMakeLists.txt expects a non-zero exit, and output that names this file,
 * the line and both values, so a broken harness can't report success. */
#include "harness.h"

TEST(reports_both_values) {
    CHECK_EQ_INT(3, 4);
    CHECK_EQ_STR("left", "right");
}

int main(void) {
    RUN_TEST(reports_both_values);
    return harness_finish();
}
