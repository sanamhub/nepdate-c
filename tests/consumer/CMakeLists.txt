cmake_minimum_required(VERSION 3.25)
project(nepdate_c_consumer LANGUAGES C)

# A separate project that uses an installed nepdate-c, built by the install_consumer test once
# through the CMake package and once through pkg-config.
option(CONSUMER_USE_PKG_CONFIG "Find nepdate-c with pkg-config instead of find_package" OFF)

add_executable(consumer consumer.c)
set_target_properties(consumer PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)

if(CONSUMER_USE_PKG_CONFIG)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NEPDATE_C REQUIRED IMPORTED_TARGET nepdate-c)
    target_link_libraries(consumer PRIVATE PkgConfig::NEPDATE_C)
    pkg_check_modules(NEPDATE_C_PATRO QUIET IMPORTED_TARGET nepdate-c-patro)
    if(NEPDATE_C_PATRO_FOUND)
        target_link_libraries(consumer PRIVATE PkgConfig::NEPDATE_C_PATRO)
        target_compile_definitions(consumer PRIVATE CONSUMER_PATRO)
    endif()
else()
    find_package(nepdate-c CONFIG REQUIRED)
    find_package(nepdate-c-patro CONFIG QUIET)
    target_link_libraries(consumer PRIVATE nepdate-c::nepdate)
    if(TARGET nepdate-c::patro)
        target_link_libraries(consumer PRIVATE nepdate-c::patro)
        target_compile_definitions(consumer PRIVATE CONSUMER_PATRO)
    endif()
endif()
