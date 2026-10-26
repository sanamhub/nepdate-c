cmake_minimum_required(VERSION 3.25...4.1)

project(nepdate-c VERSION 0.0.0 LANGUAGES C)

# The regenerate command is `cmake --build <dir> --target codegen` (ADR-0001). CMake 3.31 and later
# reserve that target name for their own CODEGEN commands unless this policy is OLD.
if(POLICY CMP0171)
    cmake_policy(SET CMP0171 OLD)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_VISIBILITY_PRESET hidden)

option(NEPDATE_C_BUILD_TESTS "Build the tests" ${PROJECT_IS_TOP_LEVEL})
option(NEPDATE_C_BUILD_FUZZERS "Build the libFuzzer targets (Clang and the fuzz preset)" OFF)
# The option exists so a core-only package can turn the patro library off.
option(NEPDATE_C_BUILD_PATRO "Build the nepdate-c::patro library" ON)
option(NEPDATE_C_WITH_TM "Build the struct tm bridge (needs <time.h>)" ON)
option(NEPDATE_C_BUILD_TOOLS "Build tools/codegen" OFF)
option(NEPDATE_C_CXX_HEADERS "Compile the public headers as C++ too (ADR-0001)" OFF)

if(NEPDATE_C_CXX_HEADERS)
    enable_language(CXX)
    # No C++ modules here, so skip the dependency scan that needs clang-scan-deps.
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(cmake/warnings.cmake)
include(cmake/instrumentation.cmake)
include(cmake/testing.cmake)

# Programs go next to the DLL, so a shared build on Windows runs its tests without a PATH change.
if(PROJECT_IS_TOP_LEVEL AND NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
endif()

add_library(nepdate src/nepdate.c)
add_library(nepdate-c::nepdate ALIAS nepdate)
target_sources(nepdate
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES include/nepdate.h
)
set_target_properties(nepdate PROPERTIES
    OUTPUT_NAME nepdate-c
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)
target_compile_definitions(nepdate PRIVATE NEPDATE_BUILDING)
get_target_property(nepdate_type nepdate TYPE)
if(nepdate_type STREQUAL "STATIC_LIBRARY")
    # No dllimport in the users of a static library (include/nepdate.h).
    target_compile_definitions(nepdate PUBLIC NEPDATE_STATIC)
    set(NEPDATE_C_PC_CFLAGS " -DNEPDATE_STATIC")
else()
    set(NEPDATE_C_PC_CFLAGS "")
endif()
if(NEPDATE_C_WITH_TM)
    target_sources(nepdate PRIVATE src/tm.c)
else()
    # The header hides the struct tm declarations from this build's users too, through the
    # CMake package and the pkg-config file alike.
    target_compile_definitions(nepdate PUBLIC NEPDATE_C_WITH_TM=0)
    string(APPEND NEPDATE_C_PC_CFLAGS " -DNEPDATE_C_WITH_TM=0")
endif()
nepdate_c_set_warnings(nepdate)
nepdate_c_set_instrumentation(nepdate)

set(NEPDATE_C_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/nepdate-c")

install(TARGETS nepdate
    EXPORT nepdate-cTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    FILE_SET HEADERS
)
install(EXPORT nepdate-cTargets
    NAMESPACE nepdate-c::
    DESTINATION "${NEPDATE_C_CONFIG_INSTALL_DIR}"
)
configure_package_config_file(cmake/nepdate-cConfig.cmake.in
    "${PROJECT_BINARY_DIR}/nepdate-cConfig.cmake"
    INSTALL_DESTINATION "${NEPDATE_C_CONFIG_INSTALL_DIR}"
)
write_basic_package_version_file("${PROJECT_BINARY_DIR}/nepdate-cConfigVersion.cmake"
    COMPATIBILITY SameMinorVersion
)
install(FILES
    "${PROJECT_BINARY_DIR}/nepdate-cConfig.cmake"
    "${PROJECT_BINARY_DIR}/nepdate-cConfigVersion.cmake"
    DESTINATION "${NEPDATE_C_CONFIG_INSTALL_DIR}"
)

# The .pc file finds the prefix from its own location, so `cmake --install --prefix` can put the
# tree anywhere.
if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
    set(NEPDATE_C_PC_PREFIX "${CMAKE_INSTALL_PREFIX}")
else()
    file(RELATIVE_PATH pc_up "/root/${CMAKE_INSTALL_LIBDIR}/pkgconfig" "/root")
    set(NEPDATE_C_PC_PREFIX "\${pcfiledir}/${pc_up}")
endif()
configure_file(cmake/nepdate-c.pc.in "${PROJECT_BINARY_DIR}/nepdate-c.pc" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/nepdate-c.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

if(NEPDATE_C_BUILD_TESTS)
    enable_testing()
endif()

if(NEPDATE_C_BUILD_PATRO AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/patro/CMakeLists.txt")
    add_subdirectory(patro)
endif()

if(NEPDATE_C_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(NEPDATE_C_BUILD_FUZZERS)
    add_subdirectory(tests/fuzz)
endif()

if(NEPDATE_C_BUILD_TOOLS)
    add_subdirectory(tools/codegen)
endif()
