# The warning set of ADR-0004, as errors. PRIVATE only: a user of the library inherits none of it.
function(nepdate_c_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wcast-qual
            -Wstrict-prototypes
            -Wmissing-prototypes
            -Wvla
            -pedantic-errors
            -Werror
        )
    endif()
endfunction()

# The public headers compiled as C++ (ADR-0001): the C++ counterpart of the set above.
function(nepdate_c_set_cxx_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /Zc:__cplusplus)
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wcast-qual
            -Wold-style-cast
            -Wzero-as-null-pointer-constant
            -pedantic-errors
            -Werror
        )
    endif()
endfunction()
