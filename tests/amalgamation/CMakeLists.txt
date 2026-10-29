cmake_minimum_required(VERSION 3.25)
project(nepdate_c_amalgamation LANGUAGES C)

# ROOT is the repository root. Only dist/ is used here, so the build proves the two-file pairs
# carry the whole library.
add_executable(amalgamation_test
    amalgamation_test.c
    "${ROOT}/dist/nepdate.c"
    "${ROOT}/dist/nepdate_patro.c")
set_target_properties(amalgamation_test PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
target_include_directories(amalgamation_test PRIVATE "${ROOT}/dist")
target_compile_definitions(amalgamation_test PRIVATE NEPDATE_STATIC NEPDATE_PATRO_STATIC)

enable_testing()
add_test(NAME amalgamation_test COMMAND amalgamation_test)
set_tests_properties(amalgamation_test PROPERTIES PASS_REGULAR_EXPRESSION "BS 2081-04-15")
