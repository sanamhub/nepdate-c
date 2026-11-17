# Installs a build tree to a scratch prefix, checks the installed layout, then configures, builds
# and runs tests/consumer against it: once through find_package, and once through pkg-config when
# PKG_CONFIG is set.
#
# Inputs: -DBUILD_DIR -DSOURCE_DIR -DWORK -DGENERATOR -DMAKE_PROGRAM -DC_COMPILER -DTOOLCHAIN_FILE
# -DBUILD_TYPE -DLIBDIR -DINCLUDEDIR -DBINDIR -DPKG_CONFIG

cmake_minimum_required(VERSION 3.25)

set(prefix "${WORK}/prefix")
file(REMOVE_RECURSE "${WORK}")

function(run)
    list(JOIN ARGN " " printable)
    message(STATUS "install_consumer> ${printable}")
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE rc)
    if(NOT rc EQUAL 0)
        message(FATAL_ERROR "exited ${rc}: ${printable}")
    endif()
endfunction()

run("${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${prefix}")

foreach(file IN ITEMS
        "${INCLUDEDIR}/nepdate.h"
        "${LIBDIR}/cmake/nepdate-c/nepdate-cConfig.cmake"
        "${LIBDIR}/cmake/nepdate-c/nepdate-cConfigVersion.cmake"
        "${LIBDIR}/pkgconfig/nepdate-c.pc")
    if(NOT EXISTS "${prefix}/${file}")
        message(FATAL_ERROR "not installed: ${file}")
    endif()
endforeach()

set(configure_args -G "${GENERATOR}" "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
    "-DCMAKE_PREFIX_PATH=${prefix}")
if(MAKE_PROGRAM)
    list(APPEND configure_args "-DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}")
endif()
if(TOOLCHAIN_FILE)
    list(APPEND configure_args "-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}")
else()
    list(APPEND configure_args "-DCMAKE_C_COMPILER=${C_COMPILER}")
endif()

# A shared build on Windows finds its DLL through PATH.
if(CMAKE_HOST_WIN32)
    set(ENV{PATH} "${prefix}/${BINDIR};$ENV{PATH}")
else()
    set(ENV{PATH} "${prefix}/${BINDIR}:$ENV{PATH}")
endif()

set(modes package)
if(PKG_CONFIG)
    list(APPEND modes pkg-config)
    set(ENV{PKG_CONFIG_PATH} "${prefix}/${LIBDIR}/pkgconfig")
    set(ENV{PKG_CONFIG} "${PKG_CONFIG}")
    run("${PKG_CONFIG}" --cflags --libs nepdate-c)
else()
    message(STATUS "install_consumer: pkg-config not found, checking the CMake package only")
endif()

foreach(mode IN LISTS modes)
    set(dir "${WORK}/consumer-${mode}")
    if(mode STREQUAL "pkg-config")
        set(use_pkg_config ON)
    else()
        set(use_pkg_config OFF)
    endif()
    run("${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${dir}" ${configure_args}
        "-DCONSUMER_USE_PKG_CONFIG=${use_pkg_config}")
    run("${CMAKE_COMMAND}" --build "${dir}")
    run("${dir}/consumer")
endforeach()

# The standalone patro package builds against the installed core, and refuses a core of another
# version (ADR-0013). The fake prefix holds a core config that reports 9.9.9 and nothing else, so
# the version check must stop the configure before it looks for a target.
# The package installed above also carries nepdate-c::patro, so the standalone build needs a
# core-only install, the way the vcpkg and Conan packages have it.
set(root_source "${SOURCE_DIR}/../..")
set(patro_source "${root_source}/patro")
set(core_prefix "${WORK}/core-only-prefix")
run("${CMAKE_COMMAND}" -S "${root_source}" -B "${WORK}/core-only" ${configure_args}
    -DNEPDATE_C_BUILD_PATRO=OFF -DNEPDATE_C_BUILD_TESTS=OFF)
run("${CMAKE_COMMAND}" --build "${WORK}/core-only")
run("${CMAKE_COMMAND}" --install "${WORK}/core-only" --prefix "${core_prefix}")
set(patro_args ${configure_args})
list(FILTER patro_args EXCLUDE REGEX "^-DCMAKE_PREFIX_PATH=")
run("${CMAKE_COMMAND}" -S "${patro_source}" -B "${WORK}/patro-standalone" ${patro_args}
    "-DCMAKE_PREFIX_PATH=${core_prefix}")
run("${CMAKE_COMMAND}" --build "${WORK}/patro-standalone")

set(fake_core "${WORK}/fake-core/lib/cmake/nepdate-c")
file(MAKE_DIRECTORY "${fake_core}")
file(WRITE "${fake_core}/nepdate-cConfig.cmake" "")
file(WRITE "${fake_core}/nepdate-cConfigVersion.cmake"
    "set(PACKAGE_VERSION 9.9.9)\nset(PACKAGE_VERSION_COMPATIBLE TRUE)\n")
set(mismatch_args ${configure_args})
list(FILTER mismatch_args EXCLUDE REGEX "^-DCMAKE_PREFIX_PATH=")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${patro_source}" -B "${WORK}/patro-mismatch" ${mismatch_args}
        "-DCMAKE_PREFIX_PATH=${WORK}/fake-core"
    RESULT_VARIABLE mismatch_rc
    ERROR_VARIABLE mismatch_error)
if(mismatch_rc EQUAL 0)
    message(FATAL_ERROR "the standalone patro configured against a core of another version")
endif()
if(NOT mismatch_error MATCHES "needs nepdate-c [^ ]+, found 9\.9\.9")
    message(FATAL_ERROR "the standalone patro failed for another reason:\n${mismatch_error}")
endif()
message(STATUS "install_consumer: the standalone patro refused core 9.9.9")
