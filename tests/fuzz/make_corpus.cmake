# Writes the seed corpora of the libFuzzer targets from the shared vectors. The files are generated
# in the build tree, never committed:
#
#   cmake -DSHARED_DIR=<shared> -DOUT_DIR=<build>/fuzz/seeds -P make_corpus.cmake

cmake_minimum_required(VERSION 3.25)

foreach(var SHARED_DIR OUT_DIR)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "make_corpus: -D${var}=... is required")
    endif()
endforeach()

function(read_rows path out)
    file(READ "${path}" text)
    string(REPLACE "\r" "" text "${text}")
    string(REPLACE "\n" ";" rows "${text}")
    set(result "")
    foreach(row IN LISTS rows)
        if(row STREQUAL "" OR row MATCHES "^#" OR row MATCHES "^input\t" OR
           row MATCHES "^bs\tpattern\tresult$")
            continue()
        endif()
        list(APPEND result "${row}")
    endforeach()
    set(${out} "${result}" PARENT_SCOPE)
endfunction()

function(write_rows directory rows column prefix)
    file(MAKE_DIRECTORY "${directory}")
    set(number 0)
    foreach(row IN LISTS rows)
        string(REPLACE "\t" "<tab>" row "${row}")
        string(REPLACE "<tab>" ";" fields "${row}")
        list(GET fields ${column} value)
        string(REPLACE ";" "\\;" value "${value}")
        math(EXPR number "${number} + 1")
        set(file "${directory}/${prefix}${number}")
        file(WRITE "${file}" "${value}")
    endforeach()
endfunction()

read_rows("${SHARED_DIR}/spec/vectors/parse.tsv" parse_rows)
read_rows("${SHARED_DIR}/spec/vectors/csharp-golden/format-pattern.tsv" format_rows)

write_rows("${OUT_DIR}/parse_strict" "${parse_rows}" 0 "")
write_rows("${OUT_DIR}/parse_lenient" "${parse_rows}" 0 "")
write_rows("${OUT_DIR}/format_pattern" "${format_rows}" 1 "")
# from_unix takes fixed-width binary input; the fuzzer starts from an empty seed directory.
file(MAKE_DIRECTORY "${OUT_DIR}/from_unix")
