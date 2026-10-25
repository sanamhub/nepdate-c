# A command put in front of every test program, such as Valgrind in the valgrind preset.
set(NEPDATE_C_TEST_LAUNCHER "" CACHE STRING
    "Command list that runs each test program, for example valgrind;--error-exitcode=99")

# Registers a CTest test that runs one of this project's programs.
function(nepdate_c_add_test name target)
    if(NEPDATE_C_TEST_LAUNCHER)
        add_test(NAME ${name} COMMAND ${NEPDATE_C_TEST_LAUNCHER} $<TARGET_FILE:${target}>)
    else()
        # The bare target name, so CMake adds CMAKE_CROSSCOMPILING_EMULATOR when there is one.
        add_test(NAME ${name} COMMAND ${target})
    endif()
endfunction()
