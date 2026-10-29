cmake_minimum_required(VERSION 3.25)
project(nepdate_side_by_side LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(nepdate-c CONFIG REQUIRED)
find_package(nepdate CONFIG REQUIRED)

add_executable(c_consumer c_consumer.c)
target_link_libraries(c_consumer PRIVATE nepdate-c::nepdate)

add_executable(cpp_consumer cpp_consumer.cpp)
target_link_libraries(cpp_consumer PRIVATE nepdate::nepdate)
