# The benchmark harness of BENCHMARK section 3. It is built only from the top-level project with
# NEPDATE_C_BUILD_BENCH=ON and is never installed. The competitor is downloaded here and compiled
# here; GPL code stays out of the tree and out of every released artefact.
cmake_minimum_required(VERSION 3.25...4.1)

include(FetchContent)

set(NEPDATE_C_BENCH_COMPETITORS ON CACHE BOOL
    "Download the C competitor of docs/competitors.md into the build tree")
set(NEPDATE_C_BENCH_RESULT "i5-12500h" CACHE STRING
    "File name (without .json) of the result under bench/results")

add_executable(nepdate_bench bench_main.c bench_competitors.c golden.c)
target_link_libraries(nepdate_bench PRIVATE nepdate-c::nepdate nepdate-c::patro)
target_include_directories(nepdate_bench PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(nepdate_bench PRIVATE NEPDATE_BENCH_COMPETITORS=${NEPDATE_C_BENCH_COMPETITORS})
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
add_executable(nepdate_bench_inlined include_amalgamation.c bench_competitors.c golden.c)
target_include_directories(nepdate_bench_inlined PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/dist")
target_compile_definitions(nepdate_bench_inlined PRIVATE
    NEPDATE_STATIC NEPDATE_PATRO_STATIC NEPDATE_BENCH_COMPETITORS=${NEPDATE_C_BENCH_COMPETITORS})
nepdate_c_set_warnings(nepdate_bench_inlined)
nepdate_c_set_instrumentation(nepdate_bench_inlined)
if(MSVC)
    target_compile_definitions(nepdate_bench_inlined PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(NEPDATE_C_BENCH_COMPETITORS)
    # Download only: aatishnn/nepdate is a C source tree with no CMake project, and its GPL code is
    # compiled here only, never linked into a released binary.
    if(POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD)
    endif()
    FetchContent_Declare(aatishnn_nepdate
        GIT_REPOSITORY https://github.com/aatishnn/nepdate.git
        GIT_TAG 18e5d3f9)
    FetchContent_GetProperties(aatishnn_nepdate)
    if(NOT aatishnn_nepdate_POPULATED)
        FetchContent_Populate(aatishnn_nepdate)
    endif()
    add_library(aatishnn_nepdate STATIC "${aatishnn_nepdate_SOURCE_DIR}/date.c")
    target_include_directories(aatishnn_nepdate PUBLIC "${aatishnn_nepdate_SOURCE_DIR}")
    target_compile_features(aatishnn_nepdate PRIVATE c_std_11)
    target_link_libraries(nepdate_bench PRIVATE aatishnn_nepdate)
    target_compile_definitions(nepdate_bench PRIVATE NEPDATE_BENCH_COMPETITOR_AATISHNN)
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
