# libFuzzer targets, built only with the fuzz preset: Clang, ASan and UBSan, with
# -fsanitize=fuzzer-no-link on the library so the core is instrumented too.
if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "NEPDATE_C_BUILD_FUZZERS needs Clang")
endif()
if(NOT NEPDATE_C_SANITIZE MATCHES "fuzzer")
    message(FATAL_ERROR "NEPDATE_C_BUILD_FUZZERS needs the fuzz preset (NEPDATE_C_SANITIZE)")
endif()

set(seed_dir "${CMAKE_BINARY_DIR}/fuzz/seeds")
execute_process(
    COMMAND "${CMAKE_COMMAND}"
        "-DSHARED_DIR=${PROJECT_SOURCE_DIR}/shared"
        "-DOUT_DIR=${seed_dir}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/make_corpus.cmake"
    RESULT_VARIABLE corpus_result
)
if(NOT corpus_result EQUAL 0)
    message(FATAL_ERROR "the fuzz seed corpus could not be written")
endif()

foreach(target IN ITEMS parse_strict parse_lenient format_pattern)
    add_executable(fuzz_${target} ${target}.c)
    target_link_libraries(fuzz_${target} PRIVATE nepdate-c::nepdate)
    nepdate_c_set_warnings(fuzz_${target})
    nepdate_c_set_instrumentation(fuzz_${target})
    target_link_options(fuzz_${target} PRIVATE -fsanitize=fuzzer)
    set_target_properties(fuzz_${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/fuzz/bin")
endforeach()
