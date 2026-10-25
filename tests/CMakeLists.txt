# A test program: one C file using harness.h, linked with the library, run by CTest.
function(nepdate_c_add_test_program name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE nepdate-c::nepdate)
    nepdate_c_set_warnings(${name})
    nepdate_c_set_instrumentation(${name})
    nepdate_c_add_test(${name} ${name})
endfunction()

nepdate_c_add_test_program(version_test version_test.c)
nepdate_c_add_test_program(validation_test validation_test.c)
nepdate_c_add_test_program(conversion_test conversion_test.c)
nepdate_c_add_test_program(vectors_test vectors_test.c vectors.c)
target_compile_definitions(vectors_test PRIVATE
    NEPDATE_C_SHARED_DIR="${PROJECT_SOURCE_DIR}/shared")
# The sample of the 2^32 date values the property test checks on a normal CI job. Valgrind is far
# slower than native code, so its preset checks a smaller sample; the full 2^32 sweep is an option.
set(NEPDATE_C_BIT_PATTERN_BITS 24 CACHE STRING
    "log2 of the number of date bit patterns bit_patterns_test checks")
set(bit_pattern_bits ${NEPDATE_C_BIT_PATTERN_BITS})
if(NEPDATE_C_TEST_LAUNCHER MATCHES "valgrind")
    set(bit_pattern_bits 16)
endif()
nepdate_c_add_test_program(bit_patterns_test bit_patterns_test.c)
target_compile_definitions(bit_patterns_test PRIVATE
    NEPDATE_C_BIT_PATTERN_BITS=${bit_pattern_bits})
option(NEPDATE_C_FULL_SWEEP "Check all 2^32 bit patterns in bit_patterns_test" OFF)
if(NEPDATE_C_FULL_SWEEP)
    target_compile_definitions(bit_patterns_test PRIVATE NEPDATE_C_FULL_SWEEP)
    set_tests_properties(bit_patterns_test PROPERTIES TIMEOUT 3600)
endif()

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
endif()

if(NEPDATE_C_CXX_HEADERS)
    foreach(standard IN ITEMS 11 20)
        add_executable(cxx_headers_${standard} cxx_headers.cpp)
        set_target_properties(cxx_headers_${standard} PROPERTIES
            CXX_STANDARD ${standard} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
        target_link_libraries(cxx_headers_${standard} PRIVATE nepdate-c::nepdate)
        nepdate_c_set_cxx_warnings(cxx_headers_${standard})
        nepdate_c_set_instrumentation(cxx_headers_${standard})
        nepdate_c_add_test(cxx_headers_${standard} cxx_headers_${standard})
    endforeach()
endif()

# The release check must refuse a version the files don't carry.
add_test(NAME release_check_rejects_other_version
    COMMAND "${CMAKE_COMMAND}" -DVERSION=9.9.9 -P "${PROJECT_SOURCE_DIR}/tools/release_check.cmake")
set_tests_properties(release_check_rejects_other_version PROPERTIES PASS_REGULAR_EXPRESSION
    "release_check: 9\.9\.9 doesn't match:.*CMakeLists\.txt: project version 0\.0\.0, want 9\.9\.9")
