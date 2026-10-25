# Runs the generator as a user would and checks the command-line behaviour (ADR-0001).
# Inputs: -DCODEGEN=<exe> -DSHARED=<shared dir> -DCOMMITTED=<committed header> -DWORK=<scratch dir>

# Script mode starts with old policies; IN_LIST below needs CMP0057.
cmake_minimum_required(VERSION 3.25...4.1)

set(relative "src/generated/calendar_tables.h")
file(REMOVE_RECURSE "${WORK}")

foreach(run IN ITEMS a b)
    # The tool creates files, not directories.
    file(MAKE_DIRECTORY "${WORK}/${run}/src/generated")
    execute_process(COMMAND "${CODEGEN}" --shared "${SHARED}" --out "${WORK}/${run}"
        RESULT_VARIABLE rc)
    if(NOT rc EQUAL 0)
        message(FATAL_ERROR "codegen run ${run} exited ${rc}")
    endif()
endforeach()

execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files
    "${WORK}/a/${relative}" "${WORK}/b/${relative}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "two runs gave different bytes")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files
    "${WORK}/a/${relative}" "${COMMITTED}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "${COMMITTED} is out of date: build the codegen target")
endif()

file(READ "${WORK}/a/${relative}" hex HEX)
string(REGEX REPLACE "(..)" "\\1;" bytes "${hex}")
if("0d" IN_LIST bytes)
    message(FATAL_ERROR "generated header contains a CR byte")
endif()

execute_process(COMMAND "${CODEGEN}" --shared "${SHARED}" --out "${WORK}/a" --check
    RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "--check on a fresh tree exited ${rc}, expected 0")
endif()

# One byte changed: 2199 becomes 2198.
file(READ "${WORK}/a/${relative}" text)
string(REPLACE "NEPDATE_MAX_YEAR 2199" "NEPDATE_MAX_YEAR 2198" changed "${text}")
if(changed STREQUAL text)
    message(FATAL_ERROR "the header has no NEPDATE_MAX_YEAR 2199 to change")
endif()
file(WRITE "${WORK}/a/${relative}" "${changed}")
execute_process(COMMAND "${CODEGEN}" --shared "${SHARED}" --out "${WORK}/a" --check
    RESULT_VARIABLE rc ERROR_VARIABLE err)
if(NOT rc EQUAL 1)
    message(FATAL_ERROR "--check after a byte change exited ${rc}, expected 1")
endif()
string(FIND "${err}" "${WORK}/a/${relative}" named)
if(named EQUAL -1)
    message(FATAL_ERROR "--check did not print the changed path; it printed: ${err}")
endif()
file(READ "${WORK}/a/${relative}" after)
if(NOT after STREQUAL changed)
    message(FATAL_ERROR "--check wrote to the file")
endif()

# Bad arguments and a missing input exit 2.
execute_process(COMMAND "${CODEGEN}" --out "${WORK}/a" RESULT_VARIABLE rc OUTPUT_QUIET ERROR_QUIET)
if(NOT rc EQUAL 2)
    message(FATAL_ERROR "missing --shared exited ${rc}, expected 2")
endif()
execute_process(COMMAND "${CODEGEN}" --shared "${WORK}/nowhere" --out "${WORK}/a"
    RESULT_VARIABLE rc OUTPUT_QUIET ERROR_QUIET)
if(NOT rc EQUAL 2)
    message(FATAL_ERROR "a missing shared directory exited ${rc}, expected 2")
endif()
