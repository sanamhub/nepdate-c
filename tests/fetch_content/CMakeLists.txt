cmake_minimum_required(VERSION 3.25)
project(nepdate_c_fetch_content LANGUAGES C)

# A parent project that pulls nepdate-c in with the FetchContent declaration nepdate.com gives, and
# has its own targets named patro and nepdate. With CONSUMER_WITH_CPP it also fetches nepdate-cpp,
# whose core target is named nepdate, so that name goes to nepdate-cpp instead of this project.
# FETCHCONTENT_SOURCE_DIR_NEPDATE-C points the nepdate-c declaration at the checkout under test, so
# nepdate-c itself is never downloaded.
option(CONSUMER_WITH_CPP "Also fetch nepdate-cpp (needs the network and a C++20 compiler)" OFF)

if(NOT DEFINED FETCHCONTENT_SOURCE_DIR_NEPDATE-C)
    message(FATAL_ERROR "set FETCHCONTENT_SOURCE_DIR_NEPDATE-C to the nepdate-c checkout to test")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(nepdate-c
    GIT_REPOSITORY https://github.com/sanamhub/nepdate-c.git
    GIT_TAG v0.1.0-beta.1
)
set(fetched nepdate-c)
if(CONSUMER_WITH_CPP)
    enable_language(CXX)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
    FetchContent_Declare(nepdate
        GIT_REPOSITORY https://github.com/sanamhub/nepdate-cpp.git
        GIT_TAG v0.1.0-beta.1
    )
    list(APPEND fetched nepdate)
endif()
FetchContent_MakeAvailable(${fetched})

# This project's own patro library.
add_library(patro STATIC app_patro.c)
target_link_libraries(patro PUBLIC nepdate-c::nepdate nepdate-c::patro)

if(CONSUMER_WITH_CPP)
    if(NOT TARGET nepdate)
        message(FATAL_ERROR "nepdate-cpp did not define its nepdate target")
    endif()
    set(app consumer)
    add_executable(cpp_consumer ../side_by_side/cpp_consumer.cpp)
    target_link_libraries(cpp_consumer PRIVATE nepdate::nepdate)
else()
    # This project's own program named nepdate.
    set(app nepdate)
endif()
add_executable(${app} app.c)
target_link_libraries(${app} PRIVATE patro)
