# nepdate-c-patro: the patro (tithi, holidays, events) library. It depends on the core target;
# the core never depends on it (hub ADR-0004).
add_library(patro "${PROJECT_SOURCE_DIR}/src/patro.c")
add_library(nepdate-c::patro ALIAS patro)

target_sources(patro
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
        FILES "${PROJECT_SOURCE_DIR}/include/nepdate_patro.h"
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
configure_file("${PROJECT_SOURCE_DIR}/cmake/nepdate-c-patro.pc.in"
    "${PROJECT_BINARY_DIR}/nepdate-c-patro.pc" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/nepdate-c-patro.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

nepdate_c_set_warnings(patro)
nepdate_c_set_instrumentation(patro)

install(TARGETS patro
    EXPORT nepdate-cTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    FILE_SET HEADERS
)
