add_executable(codegen_calendar_test calendar_test.c)
target_link_libraries(codegen_calendar_test PRIVATE nepdate_c_codegen_lib)
target_include_directories(codegen_calendar_test PRIVATE "${PROJECT_SOURCE_DIR}/tests")
target_compile_definitions(codegen_calendar_test PRIVATE
    _CRT_SECURE_NO_WARNINGS
    NEPDATE_C_SHARED_DIR="${PROJECT_SOURCE_DIR}/shared")
nepdate_c_set_warnings(codegen_calendar_test)
nepdate_c_set_instrumentation(codegen_calendar_test)
nepdate_c_add_test(codegen_calendar_test codegen_calendar_test)

add_executable(codegen_patro_test patro_test.c)
target_link_libraries(codegen_patro_test PRIVATE nepdate_c_codegen_lib)
target_include_directories(codegen_patro_test PRIVATE "${PROJECT_SOURCE_DIR}/tests")
target_compile_definitions(codegen_patro_test PRIVATE
    _CRT_SECURE_NO_WARNINGS
    NEPDATE_C_SHARED_DIR="${PROJECT_SOURCE_DIR}/shared")
nepdate_c_set_warnings(codegen_patro_test)
nepdate_c_set_instrumentation(codegen_patro_test)
nepdate_c_add_test(codegen_patro_test codegen_patro_test)

# The command-line checks: identical bytes on two runs, LF only, --check exit codes.
add_test(NAME codegen_cli
    COMMAND "${CMAKE_COMMAND}"
        -DCODEGEN=$<TARGET_FILE:nepdate_c_codegen>
        -DSHARED=${PROJECT_SOURCE_DIR}/shared
        -DCOMMITTED=${PROJECT_SOURCE_DIR}/src/generated/calendar_tables.h
        -DCOMMITTED_PATRO=${PROJECT_SOURCE_DIR}/src/generated/patro_tables.h
        -DWORK=${CMAKE_CURRENT_BINARY_DIR}/codegen_cli
        -P "${CMAKE_CURRENT_SOURCE_DIR}/codegen_cli_test.cmake"
)
