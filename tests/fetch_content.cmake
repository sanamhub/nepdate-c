# Configures, builds and runs tests/fetch_content, a parent project that pulls this checkout in
# with FetchContent and has its own targets named nepdate and patro.
#
# Inputs: -DROOT -DSOURCE_DIR -DWORK -DGENERATOR -DMAKE_PROGRAM -DC_COMPILER -DTOOLCHAIN_FILE
# -DBUILD_TYPE

cmake_minimum_required(VERSION 3.25)

file(REMOVE_RECURSE "${WORK}")

function(run)
    list(JOIN ARGN " " printable)
    message(STATUS "fetch_content> ${printable}")
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE rc)
    if(NOT rc EQUAL 0)
        message(FATAL_ERROR "exited ${rc}: ${printable}")
    endif()
endfunction()

set(configure_args -G "${GENERATOR}" "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
    "-DFETCHCONTENT_SOURCE_DIR_NEPDATE-C=${ROOT}")
if(MAKE_PROGRAM)
    list(APPEND configure_args "-DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}")
endif()
if(TOOLCHAIN_FILE)
    list(APPEND configure_args "-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}")
else()
    list(APPEND configure_args "-DCMAKE_C_COMPILER=${C_COMPILER}")
endif()

run("${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${WORK}" ${configure_args})
run("${CMAKE_COMMAND}" --build "${WORK}")
run("${WORK}/nepdate")
