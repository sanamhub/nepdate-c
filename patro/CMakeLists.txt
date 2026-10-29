# nepdate-c-patro: the patro (tithi, holidays, events) library. From the root project it is a
# second target in the nepdate-c package; from this directory it is a standalone package that finds
# an installed nepdate-c, which is what the vcpkg and Conan patro packages need.

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.25...4.1)
    project(nepdate-c-patro VERSION 0.1.0 LANGUAGES C)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS OFF)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    find_package(nepdate-c CONFIG REQUIRED)
    set(NEPDATE_C_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
    set(NEPDATE_C_PATRO_STANDALONE ON)
else()
    set(NEPDATE_C_ROOT "${PROJECT_SOURCE_DIR}")
    set(NEPDATE_C_PATRO_STANDALONE OFF)
endif()

add_library(patro "${NEPDATE_C_ROOT}/src/patro.c")
add_library(nepdate-c::patro ALIAS patro)

target_sources(patro
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${NEPDATE_C_ROOT}/include"
        FILES "${NEPDATE_C_ROOT}/include/nepdate_patro.h"
)
target_link_libraries(patro PUBLIC nepdate-c::nepdate)
set_target_properties(patro PROPERTIES
    OUTPUT_NAME nepdate-c-patro
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)
target_compile_definitions(patro PRIVATE NEPDATE_PATRO_BUILDING)
get_target_property(patro_type patro TYPE)
if(patro_type STREQUAL "STATIC_LIBRARY")
    target_compile_definitions(patro PUBLIC NEPDATE_PATRO_STATIC)
    set(NEPDATE_C_PATRO_PC_CFLAGS " -DNEPDATE_PATRO_STATIC")
else()
    set(NEPDATE_C_PATRO_PC_CFLAGS "")
endif()

# The .pc file finds the prefix from its own location. In the root build the core has already
# computed it; the standalone build computes the same value here.
if(NOT DEFINED NEPDATE_C_PC_PREFIX)
    if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
        set(NEPDATE_C_PC_PREFIX "${CMAKE_INSTALL_PREFIX}")
    else()
        file(RELATIVE_PATH pc_up "/root/${CMAKE_INSTALL_LIBDIR}/pkgconfig" "/root")
        set(NEPDATE_C_PC_PREFIX "\${pcfiledir}/${pc_up}")
    endif()
endif()
configure_file("${NEPDATE_C_ROOT}/cmake/nepdate-c-patro.pc.in"
    "${PROJECT_BINARY_DIR}/nepdate-c-patro.pc" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/nepdate-c-patro.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

if(NOT NEPDATE_C_PATRO_STANDALONE)
    nepdate_c_set_warnings(patro)
    nepdate_c_set_instrumentation(patro)
    install(TARGETS patro
        EXPORT nepdate-cTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET HEADERS
    )
else()
    install(TARGETS patro
        EXPORT nepdate-c-patroTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET HEADERS
    )
    install(EXPORT nepdate-c-patroTargets
        NAMESPACE nepdate-c::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/nepdate-c-patro"
    )
    configure_package_config_file("${NEPDATE_C_ROOT}/cmake/nepdate-c-patroConfig.cmake.in"
        "${PROJECT_BINARY_DIR}/nepdate-c-patroConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/nepdate-c-patro"
    )
    write_basic_package_version_file("${PROJECT_BINARY_DIR}/nepdate-c-patroConfigVersion.cmake"
        COMPATIBILITY SameMinorVersion
    )
    install(FILES
        "${PROJECT_BINARY_DIR}/nepdate-c-patroConfig.cmake"
        "${PROJECT_BINARY_DIR}/nepdate-c-patroConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/nepdate-c-patro"
    )
endif()
