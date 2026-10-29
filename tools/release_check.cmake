# Exits 0 only if every version location equals VERSION and CHANGELOG.md has a dated section for it.
#
#   cmake -DVERSION=X.Y.Z -P tools/release_check.cmake
#   cmake -DVERSION=X.Y.Z-beta.1 -P tools/release_check.cmake
#
# CMake's project(VERSION) holds numbers only, so a pre-release is split: X.Y.Z goes in project()
# and NEPDATE_VERSION_MAJOR/MINOR/PATCH, the part after "-" in NEPDATE_VERSION_PRERELEASE ("" for a
# stable version), and the whole string in NEPDATE_VERSION_STRING.
#
# Locations: project() in CMakeLists.txt, the macros in include/nepdate.h, and a
# `## [VERSION] - YYYY-MM-DD` heading in CHANGELOG.md.

cmake_minimum_required(VERSION 3.25)

get_filename_component(root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# Semver 2.0 core plus an optional pre-release; build metadata is not allowed in a tag here.
set(ident "(0|[1-9][0-9]*|[0-9]*[A-Za-z-][0-9A-Za-z-]*)")
if(NOT VERSION MATCHES "^(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)(-${ident}(\\.${ident})*)?$")
    message(FATAL_ERROR "release_check: pass -DVERSION=X.Y.Z or X.Y.Z-PRERELEASE (got '${VERSION}')")
endif()
set(major "${CMAKE_MATCH_1}")
set(minor "${CMAKE_MATCH_2}")
set(patch "${CMAKE_MATCH_3}")
set(core "${major}.${minor}.${patch}")
string(LENGTH "${core}" core_length)
string(SUBSTRING "${VERSION}" ${core_length} -1 suffix)
set(prerelease "")
if(NOT suffix STREQUAL "")
    string(SUBSTRING "${suffix}" 1 -1 prerelease)
endif()

set(problems "")

file(READ "${root}/CMakeLists.txt" text)
if(NOT text MATCHES "project\\(nepdate-c VERSION ([0-9.]+)")
    list(APPEND problems "CMakeLists.txt: no project(nepdate-c VERSION ...)")
elseif(NOT CMAKE_MATCH_1 STREQUAL core)
    list(APPEND problems "CMakeLists.txt: project version ${CMAKE_MATCH_1}, want ${core}")
endif()

file(READ "${root}/include/nepdate.h" text)
foreach(part IN ITEMS MAJOR MINOR PATCH)
    string(TOLOWER "${part}" lower)
    if(NOT text MATCHES "#define NEPDATE_VERSION_${part} ([0-9]+)")
        list(APPEND problems "nepdate.h: NEPDATE_VERSION_${part} missing")
    elseif(NOT CMAKE_MATCH_1 STREQUAL ${lower})
        list(APPEND problems "nepdate.h: NEPDATE_VERSION_${part} is ${CMAKE_MATCH_1}, want ${${lower}}")
    endif()
endforeach()
if(NOT text MATCHES "#define NEPDATE_VERSION_PRERELEASE \"([^\"]*)\"")
    list(APPEND problems "nepdate.h: NEPDATE_VERSION_PRERELEASE missing")
elseif(NOT CMAKE_MATCH_1 STREQUAL prerelease)
    list(APPEND problems "nepdate.h: NEPDATE_VERSION_PRERELEASE is \"${CMAKE_MATCH_1}\", want \"${prerelease}\"")
endif()
if(NOT text MATCHES "#define NEPDATE_VERSION_STRING \"([^\"]*)\"")
    list(APPEND problems "nepdate.h: NEPDATE_VERSION_STRING missing")
elseif(NOT CMAKE_MATCH_1 STREQUAL VERSION)
    list(APPEND problems "nepdate.h: NEPDATE_VERSION_STRING is ${CMAKE_MATCH_1}, want ${VERSION}")
endif()

file(READ "${root}/patro/CMakeLists.txt" text)
if(NOT text MATCHES "project\\(nepdate-c-patro VERSION ([0-9.]+)")
    list(APPEND problems "patro/CMakeLists.txt: no project(nepdate-c-patro VERSION ...)")
elseif(NOT CMAKE_MATCH_1 STREQUAL core)
    list(APPEND problems "patro/CMakeLists.txt: project version ${CMAKE_MATCH_1}, want ${core}")
endif()

file(GLOB ports "${root}/ports/*/vcpkg.json")
foreach(port IN LISTS ports)
    file(READ "${port}" text)
    file(RELATIVE_PATH relative "${root}" "${port}")
    set(port_version "")
    string(JSON port_version ERROR_VARIABLE json_error GET "${text}" "version-semver")
    if(json_error)
        string(JSON port_version ERROR_VARIABLE json_error GET "${text}" version)
    endif()
    if(json_error)
        list(APPEND problems "${relative}: no version or version-semver")
    elseif(NOT port_version STREQUAL VERSION)
        list(APPEND problems "${relative}: version ${port_version}, want ${VERSION}")
    endif()
endforeach()

file(GLOB recipes "${root}/conan/*/conanfile.py")
foreach(recipe IN LISTS recipes)
    file(READ "${recipe}" text)
    file(RELATIVE_PATH relative "${root}" "${recipe}")
    if(NOT text MATCHES "version = \"([^\"]*)\"")
        list(APPEND problems "${relative}: no version = \"...\"")
    elseif(NOT CMAKE_MATCH_1 STREQUAL VERSION)
        list(APPEND problems "${relative}: version ${CMAKE_MATCH_1}, want ${VERSION}")
    endif()
endforeach()

file(READ "${root}/CHANGELOG.md" text)
string(REPLACE "." "\\." escaped "${VERSION}")
if(NOT text MATCHES "\n## \\[${escaped}\\] - [0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]\n")
    list(APPEND problems "CHANGELOG.md: no '## [${VERSION}] - YYYY-MM-DD' section")
endif()

if(problems)
    list(JOIN problems "\n  " lines)
    message(FATAL_ERROR "release_check: ${VERSION} doesn't match:\n  ${lines}")
endif()
message(STATUS "release_check: every version location says ${VERSION}")
