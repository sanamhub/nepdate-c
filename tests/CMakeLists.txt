# A test program: one C file using harness.h, linked with the library, run by CTest.
function(nepdate_c_add_test_program name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE nepdate-c::nepdate)
    nepdate_c_set_warnings(${name})
    nepdate_c_set_instrumentation(${name})
    nepdate_c_add_test(${name} ${name})
endfunction()

add_subdirectory(readme)

nepdate_c_add_test_program(version_test version_test.c)
nepdate_c_add_test_program(validation_test validation_test.c)
nepdate_c_add_test_program(conversion_test conversion_test.c)
nepdate_c_add_test_program(vectors_test vectors_test.c vectors.c)
target_compile_definitions(vectors_test PRIVATE
    NEPDATE_C_SHARED_DIR="${PROJECT_SOURCE_DIR}/shared")
nepdate_c_add_test_program(arithmetic_test arithmetic_test.c vectors.c)
target_compile_definitions(arithmetic_test PRIVATE
    NEPDATE_C_SHARED_DIR="${PROJECT_SOURCE_DIR}/shared")
nepdate_c_add_test_program(fiscal_test fiscal_test.c vectors.c)
target_compile_definitions(fiscal_test PRIVATE
    NEPDATE_C_SHARED_DIR="${PROJECT_SOURCE_DIR}/shared")
nepdate_c_add_test_program(range_test range_test.c)
nepdate_c_add_test_program(format_test format_test.c vectors.c)
target_compile_definitions(format_test PRIVATE
    NEPDATE_C_SHARED_DIR="${PROJECT_SOURCE_DIR}/shared")
nepdate_c_add_test_program(parse_test parse_test.c vectors.c)
target_compile_definitions(parse_test PRIVATE
    NEPDATE_C_SHARED_DIR="${PROJECT_SOURCE_DIR}/shared")
nepdate_c_add_test_program(cs_phase1 cs_phase1.c)
nepdate_c_add_test_program(cs_phase2 cs_phase2.c)
nepdate_c_add_test_program(cs_phase3 cs_phase3.c)
nepdate_c_add_test_program(time_test time_test.c)
target_include_directories(time_test PRIVATE "${PROJECT_SOURCE_DIR}/src")

if(TARGET nepdate-c::patro)
    nepdate_c_add_test_program(patro_test patro_test.c)
    target_link_libraries(patro_test PRIVATE nepdate-c::patro)
    nepdate_c_add_test_program(cs_phase4 cs_phase4.c)
    target_link_libraries(cs_phase4 PRIVATE nepdate-c::patro)
    target_include_directories(cs_phase4 PRIVATE "${PROJECT_SOURCE_DIR}/src")
endif()

# The sample of the 2^32 date values the property test checks on a normal CI job. Valgrind and
# MemorySanitizer are far slower than native code, so they check smaller samples (the msan preset
# sets 20 bits); the full 2^32 sweep is an option and runs nightly.
set(NEPDATE_C_BIT_PATTERN_BITS 24 CACHE STRING
    "log2 of the number of date bit patterns bit_patterns_test checks")
set(bit_pattern_bits ${NEPDATE_C_BIT_PATTERN_BITS})
if(NEPDATE_C_TEST_LAUNCHER MATCHES "valgrind")
    set(bit_pattern_bits 16)
endif()
nepdate_c_add_test_program(bit_patterns_test bit_patterns_test.c)
target_compile_definitions(bit_patterns_test PRIVATE
    NEPDATE_C_BIT_PATTERN_BITS=${bit_pattern_bits})
if(TARGET nepdate-c::patro)
    target_link_libraries(bit_patterns_test PRIVATE nepdate-c::patro)
    target_compile_definitions(bit_patterns_test PRIVATE NEPDATE_C_TEST_PATRO)
endif()
option(NEPDATE_C_FULL_SWEEP "Check all 2^32 bit patterns in bit_patterns_test" OFF)
if(NEPDATE_C_FULL_SWEEP)
    target_compile_definitions(bit_patterns_test PRIVATE NEPDATE_C_FULL_SWEEP)
    set_tests_properties(bit_patterns_test PROPERTIES TIMEOUT 3600)
endif()

# Every phase-1 to phase-4 row of the C# catalogue, or a comment naming the deviation or the later
# phase. Each phase has its own test source, so each test checks its own rows. The phase-4 rows
# need the patro library, so a core-only build checks phases 1 to 3.
set(catalog_phases 1 2 3)
if(TARGET nepdate-c::patro)
    list(APPEND catalog_phases 4)
endif()
foreach(phase IN LISTS catalog_phases)
    add_test(NAME catalog_coverage_${phase}
        COMMAND "${CMAKE_COMMAND}"
            -DPHASE=${phase}
            -DCATALOG=${PROJECT_SOURCE_DIR}/shared/spec/vectors/csharp-tests/catalog.tsv
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/cs_phase${phase}.c
            -P "${PROJECT_SOURCE_DIR}/tools/check_catalog.cmake"
    )
endforeach()

# The harness must turn a failed check into a failed test and say where and why.
nepdate_c_add_test_program(harness_failure harness_failure.c)
set_tests_properties(harness_failure PROPERTIES WILL_FAIL TRUE)
nepdate_c_add_test(harness_failure_output harness_failure)
set_tests_properties(harness_failure_output PROPERTIES PASS_REGULAR_EXPRESSION
    "harness_failure\\.c:6: CHECK_EQ_INT\\(3, 4\\) failed: 3 != 4\n[^\n]*harness_failure\\.c:7: CHECK_EQ_STR\\(\"left\", \"right\"\\) failed: \"left\" != \"right\"")

# install_consumer: installs this build tree to a scratch prefix and builds tests/consumer against
# it. An instrumented tree can't be linked by a plain consumer, and a tree whose programs need an
# emulator can't run one, so those builds skip it.
if(NOT NEPDATE_C_INSTRUMENTED AND NOT CMAKE_CROSSCOMPILING_EMULATOR)
    if(WIN32)
        # Windows images put Strawberry Perl's pkg-config, a Perl script CMake can't run, on PATH;
        # only a real pkgconf executable is worth testing there.
        find_program(NEPDATE_C_PKG_CONFIG NAMES pkgconf)
    else()
        find_program(NEPDATE_C_PKG_CONFIG NAMES pkg-config pkgconf)
    endif()
    add_test(NAME install_consumer
        COMMAND "${CMAKE_COMMAND}"
            -DBUILD_DIR=${PROJECT_BINARY_DIR}
            -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/consumer
            -DWORK=${CMAKE_CURRENT_BINARY_DIR}/install_consumer
            -DGENERATOR=${CMAKE_GENERATOR}
            -DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
            -DC_COMPILER=${CMAKE_C_COMPILER}
            -DTOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DBUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DLIBDIR=${CMAKE_INSTALL_LIBDIR}
            -DINCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
            -DBINDIR=${CMAKE_INSTALL_BINDIR}
            -DPKG_CONFIG=${NEPDATE_C_PKG_CONFIG}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/install_consumer.cmake"
    )
    # fetch_content: builds tests/fetch_content, which pulls this checkout in with FetchContent and
    # has its own targets named nepdate and patro. It needs the patro library, which a core-only
    # build leaves out.
    if(TARGET nepdate-c::patro)
        add_test(NAME fetch_content
            COMMAND "${CMAKE_COMMAND}"
                -DROOT=${PROJECT_SOURCE_DIR}
                -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/fetch_content
                -DWORK=${CMAKE_CURRENT_BINARY_DIR}/fetch_content
                -DGENERATOR=${CMAKE_GENERATOR}
                -DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                -DC_COMPILER=${CMAKE_C_COMPILER}
                -DTOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                -DBUILD_TYPE=${CMAKE_BUILD_TYPE}
                -P "${CMAKE_CURRENT_SOURCE_DIR}/fetch_content.cmake"
        )
    endif()
endif()

if(NEPDATE_C_CXX_HEADERS)
    foreach(standard IN ITEMS 11 20)
        add_executable(cxx_headers_${standard} cxx_headers.cpp)
        set_target_properties(cxx_headers_${standard} PROPERTIES
            CXX_STANDARD ${standard} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
        target_link_libraries(cxx_headers_${standard} PRIVATE nepdate-c::nepdate)
        if(TARGET nepdate-c::patro)
            target_link_libraries(cxx_headers_${standard} PRIVATE nepdate-c::patro)
        endif()
        nepdate_c_set_cxx_warnings(cxx_headers_${standard})
        nepdate_c_set_instrumentation(cxx_headers_${standard})
        nepdate_c_add_test(cxx_headers_${standard} cxx_headers_${standard})
    endforeach()
endif()

# The release check must refuse a version the files don't carry.
add_test(NAME release_check_rejects_other_version
    COMMAND "${CMAKE_COMMAND}" -DVERSION=9.9.9 -P "${PROJECT_SOURCE_DIR}/tools/release_check.cmake")
set_tests_properties(release_check_rejects_other_version PROPERTIES PASS_REGULAR_EXPRESSION
    "release_check: 9\.9\.9 doesn't match:.*CMakeLists\.txt: project version 0\.1\.0, want 9\.9\.9")
