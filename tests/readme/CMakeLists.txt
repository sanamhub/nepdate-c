# Builds the C example from README.md itself, so the quick start cannot drift from the library.
file(READ "${PROJECT_SOURCE_DIR}/README.md" readme)

string(FIND "${readme}" "## Quick start" quick)
if(quick LESS 0)
    message(FATAL_ERROR "README.md: no '## Quick start' section")
endif()
string(SUBSTRING "${readme}" ${quick} -1 readme)
string(FIND "${readme}" "```c" fence)
if(fence LESS 0)
    message(FATAL_ERROR "README.md: no C example in the quick start")
endif()
math(EXPR begin "${fence} + 4")
string(SUBSTRING "${readme}" ${begin} -1 readme)
string(FIND "${readme}" "```" end)
if(end LESS 0)
    message(FATAL_ERROR "README.md: the quick start C example is not closed")
endif()
string(SUBSTRING "${readme}" 0 ${end} example)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/readme_example.c" "${example}\n")

add_executable(readme_example "${CMAKE_CURRENT_BINARY_DIR}/readme_example.c")
set_target_properties(readme_example PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
target_link_libraries(readme_example PRIVATE nepdate-c::nepdate)
nepdate_c_set_warnings(readme_example)
nepdate_c_set_instrumentation(readme_example)
nepdate_c_add_test(readme_example readme_example)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/README.md")
