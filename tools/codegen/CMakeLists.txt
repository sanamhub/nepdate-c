add_library(nepdate_c_codegen_lib STATIC calendar.c calendar.h json.c json.h)
target_include_directories(nepdate_c_codegen_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(nepdate_c_codegen main.c)
target_link_libraries(nepdate_c_codegen PRIVATE nepdate_c_codegen_lib)

foreach(target IN ITEMS nepdate_c_codegen_lib nepdate_c_codegen)
    # The tool uses fopen and friends, which MSVC asks to replace with its own _s functions.
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    nepdate_c_set_warnings(${target})
    nepdate_c_set_instrumentation(${target})
endforeach()

# $<TARGET_FILE> instead of the bare target name: CMake substitutes the name only when it isn't
# cross-compiling, and a toolchain file that sets CMAKE_SYSTEM_NAME counts as cross-compiling.
set(codegen_args --shared "${PROJECT_SOURCE_DIR}/shared" --out "${PROJECT_SOURCE_DIR}")

add_custom_target(codegen
    COMMAND $<TARGET_FILE:nepdate_c_codegen> ${codegen_args}
    COMMENT "Regenerating src/generated from shared/data"
    DEPENDS nepdate_c_codegen
    VERBATIM
)
add_custom_target(codegen_check
    COMMAND $<TARGET_FILE:nepdate_c_codegen> ${codegen_args} --check
    COMMENT "Checking src/generated against shared/data"
    DEPENDS nepdate_c_codegen
    VERBATIM
)

if(NEPDATE_C_BUILD_TESTS)
    add_subdirectory(tests)
endif()
