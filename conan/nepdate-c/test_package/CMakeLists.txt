cmake_minimum_required(VERSION 3.25)
project(nepdate_c_conan_test LANGUAGES C)

find_package(nepdate-c CONFIG REQUIRED)

add_executable(example example.c)
set_target_properties(example PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_link_libraries(example PRIVATE nepdate-c::nepdate)
