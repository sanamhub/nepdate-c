# Build modes for the presets asan-ubsan, msan, fuzz and coverage (ADR-0004). They apply to the
# library, tests and tools of this build tree only: the link options are BUILD_INTERFACE, so an
# installed package never carries them.

set(NEPDATE_C_SANITIZE "" CACHE STRING
    "Comma-separated -fsanitize= list, for example address,undefined")
option(NEPDATE_C_COVERAGE "Build with --coverage for gcovr" OFF)

set(_nepdate_c_sanitize_compile "")
set(_nepdate_c_sanitize_link "")
if(NEPDATE_C_SANITIZE)
    if(MSVC)
        # MSVC has AddressSanitizer only.
        if(NOT NEPDATE_C_SANITIZE STREQUAL "address,undefined" AND
           NOT NEPDATE_C_SANITIZE STREQUAL "address")
            message(FATAL_ERROR
                "NEPDATE_C_SANITIZE=${NEPDATE_C_SANITIZE} is not supported by this compiler")
        endif()
        # /RTC1, CMake's Debug default, can't be combined with /fsanitize=address.
        string(REGEX REPLACE "/RTC[1csu]+" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        set(_nepdate_c_sanitize_compile /fsanitize=address)
    else()
        set(_nepdate_c_sanitize_compile
            -fsanitize=${NEPDATE_C_SANITIZE} -fno-sanitize-recover=all -fno-omit-frame-pointer)
        if(NEPDATE_C_SANITIZE MATCHES "memory")
            list(APPEND _nepdate_c_sanitize_compile -fsanitize-memory-track-origins)
        endif()
        set(_nepdate_c_sanitize_link -fsanitize=${NEPDATE_C_SANITIZE})
    endif()

    # Fail at configure time, with a message tools/ci.cmake recognises, instead of at link time.
    # Some toolchains (zig cc for x86_64-windows-gnu) accept -fsanitize=address and build without
    # it, so the probe also asks the compiler whether each sanitizer is really on.
    set(_nepdate_c_probe "int main(void) { return 0; }\n")
    foreach(_kind IN ITEMS address memory)
        if(NEPDATE_C_SANITIZE MATCHES "${_kind}")
            string(TOUPPER "${_kind}" _upper)
            string(PREPEND _nepdate_c_probe
                "#if defined(__SANITIZE_${_upper}__)\n"
                "#elif defined(__has_feature)\n"
                "#if !__has_feature(${_kind}_sanitizer)\n#error ${_kind} sanitizer is off\n#endif\n"
                "#else\n#error ${_kind} sanitizer is off\n#endif\n")
        endif()
    endforeach()
    include(CheckCSourceCompiles)
    list(JOIN _nepdate_c_sanitize_compile " " CMAKE_REQUIRED_FLAGS)
    set(CMAKE_REQUIRED_LINK_OPTIONS ${_nepdate_c_sanitize_link})
    check_c_source_compiles("${_nepdate_c_probe}" NEPDATE_C_SANITIZE_LINKS)
    unset(CMAKE_REQUIRED_FLAGS)
    unset(CMAKE_REQUIRED_LINK_OPTIONS)
    if(NOT NEPDATE_C_SANITIZE_LINKS)
        message(FATAL_ERROR
            "NEPDATE_C_SANITIZE=${NEPDATE_C_SANITIZE} is not supported by this compiler")
    endif()
endif()

if(NEPDATE_C_COVERAGE AND NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "NEPDATE_C_COVERAGE is not supported by this compiler (gcovr needs GCC)")
endif()

# True when this build tree is instrumented, so its libraries can't be linked by an ordinary
# consumer project.
if(NEPDATE_C_SANITIZE OR NEPDATE_C_COVERAGE)
    set(NEPDATE_C_INSTRUMENTED ON)
else()
    set(NEPDATE_C_INSTRUMENTED OFF)
endif()

function(nepdate_c_set_instrumentation target)
    # A static library isn't linked itself; whatever links it needs the runtime flags instead.
    get_target_property(type ${target} TYPE)
    if(type STREQUAL "STATIC_LIBRARY")
        set(link_scope INTERFACE)
    else()
        set(link_scope PRIVATE)
    endif()
    if(NEPDATE_C_SANITIZE)
        target_compile_options(${target} PRIVATE ${_nepdate_c_sanitize_compile})
        if(_nepdate_c_sanitize_link)
            # A literal comma would end the generator expression's argument.
            string(REPLACE "," "$<COMMA>" link "${_nepdate_c_sanitize_link}")
            target_link_options(${target} ${link_scope} "$<BUILD_INTERFACE:${link}>")
        endif()
    endif()
    if(NEPDATE_C_COVERAGE)
        target_compile_options(${target} PRIVATE --coverage)
        target_link_options(${target} ${link_scope} "$<BUILD_INTERFACE:--coverage>")
    endif()
endfunction()
