# The benchmark harness of BENCHMARK section 3. It is built only from the top-level project with
# NEPDATE_C_BUILD_BENCH=ON and is never installed. The competitors are selected in
# docs/competitors.md. This language has no mature library to compare, so the harness measures
# this library only.
cmake_minimum_required(VERSION 3.25...4.1)

set(NEPDATE_C_BENCH_RESULT "i5-12500h" CACHE STRING
    "File name (without .json) of the result under bench/results")

add_executable(nepdate_bench bench_main.c golden.c)
target_link_libraries(nepdate_bench PRIVATE nepdate-c::nepdate nepdate-c::patro)
target_include_directories(nepdate_bench PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
nepdate_c_set_warnings(nepdate_bench)
nepdate_c_set_instrumentation(nepdate_bench)
if(MSVC)
    # The harness writes the JSON with fopen; the secure-CRT deprecation is a toolchain preference,
    # not a portability requirement.
    target_compile_definitions(nepdate_bench PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# The inlined form: dist/nepdate.c and dist/nepdate_patro.c are included into the harness's
# translation unit, so the calls are inlined as they are for a project that builds the
# amalgamation. The out-of-line target above links the libraries instead.
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/dist/nepdate.c")
    message(FATAL_ERROR "NEPDATE_C_BUILD_BENCH needs dist/; run tools/amalgamate.cmake")
endif()
add_executable(nepdate_bench_inlined include_amalgamation.c golden.c)
target_include_directories(nepdate_bench_inlined PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/dist")
target_compile_definitions(nepdate_bench_inlined PRIVATE
    NEPDATE_STATIC NEPDATE_PATRO_STATIC)
nepdate_c_set_warnings(nepdate_bench_inlined)
nepdate_c_set_instrumentation(nepdate_bench_inlined)
if(MSVC)
    target_compile_definitions(nepdate_bench_inlined PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

add_custom_target(bench-run
    COMMAND $<TARGET_FILE:nepdate_bench> --json
            "${CMAKE_SOURCE_DIR}/bench/results/${NEPDATE_C_BENCH_RESULT}.json"
    DEPENDS nepdate_bench
    USES_TERMINAL)

add_custom_target(bench-run-inlined
    COMMAND $<TARGET_FILE:nepdate_bench_inlined> --json
            "${CMAKE_SOURCE_DIR}/bench/results/${NEPDATE_C_BENCH_RESULT}-amalgamation.json"
    DEPENDS nepdate_bench_inlined
    USES_TERMINAL)
