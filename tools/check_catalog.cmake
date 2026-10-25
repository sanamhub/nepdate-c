# Fails unless every row of shared/spec/vectors/csharp-tests/catalog.tsv whose phase is PHASE has a
# RUN_TEST_NAMED("<id>", ...) line, or a cs-deviation / cs-n/a / cs-pending comment naming its id.
# Prints the counts and each missing id.
#
#   cmake -DPHASE=1 -DCATALOG=<catalog.tsv> -DSOURCE=<test source>
#         -P tools/check_catalog.cmake

cmake_minimum_required(VERSION 3.25)

foreach(var PHASE CATALOG SOURCE)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "check_catalog: -D${var}=... is required")
    endif()
endforeach()

file(READ "${CATALOG}" catalog)
string(REPLACE "\r" "" catalog "${catalog}")
string(REPLACE ";" "<semicolon>" catalog "${catalog}")
string(REPLACE "\n" ";" rows "${catalog}")

file(READ "${SOURCE}" source)
string(REPLACE "\r" "" source "${source}")
string(REPLACE ";" "<semicolon>" source "${source}")
string(REPLACE "\n" " " source "${source}")

# The test ids of RUN_TEST_NAMED("<id>", ...).
set(test_ids "")
string(REGEX MATCHALL "RUN_TEST_NAMED\\( *\"[^\"]+\"" matches "${source}")
foreach(match IN LISTS matches)
    string(REGEX REPLACE "^RUN_TEST_NAMED\\( *\"([^\"]+)\"$" "\\1" id "${match}")
    list(APPEND test_ids "${id}")
endforeach()

# The ids of the cs-deviation, cs-n/a and cs-pending markers. A long marker may put the id after
# the asterisk of a wrapped comment, so allow any run of spaces and asterisks after the colon.
set(deviation_ids "")
set(na_ids "")
set(pending_ids "")
string(REGEX MATCHALL "cs-deviation: +[^ ]+" markers "${source}")
foreach(match IN LISTS markers)
    string(REGEX REPLACE "^cs-deviation: +" "" id "${match}")
    list(APPEND deviation_ids "${id}")
endforeach()
string(REGEX MATCHALL "cs-n/a: +[^ ]+" markers "${source}")
foreach(match IN LISTS markers)
    string(REGEX REPLACE "^cs-n/a: +" "" id "${match}")
    list(APPEND na_ids "${id}")
endforeach()
string(REGEX MATCHALL "cs-pending: +[^ ]+" markers "${source}")
foreach(match IN LISTS markers)
    string(REGEX REPLACE "^cs-pending: +" "" id "${match}")
    list(APPEND pending_ids "${id}")
endforeach()

set(missing "")
set(tests 0)
set(deviations 0)
set(na 0)
set(pending 0)
set(checked 0)
foreach(row IN LISTS rows)
    if(row STREQUAL "" OR row MATCHES "^#" OR row MATCHES "^id\t")
        continue()
    endif()
    string(REPLACE "\t" ";" fields "${row}")
    list(GET fields 0 id)
    list(GET fields -1 phase)
    if(NOT phase MATCHES "^[0-9]+$" OR phase GREATER PHASE)
        continue()
    endif()
    math(EXPR checked "${checked} + 1")
    if(id IN_LIST pending_ids)
        math(EXPR pending "${pending} + 1")
    elseif(id IN_LIST test_ids)
        math(EXPR tests "${tests} + 1")
    elseif(id IN_LIST deviation_ids)
        math(EXPR deviations "${deviations} + 1")
    elseif(id IN_LIST na_ids)
        math(EXPR na "${na} + 1")
    else()
        list(APPEND missing "${id}")
    endif()
endforeach()

if(missing)
    list(LENGTH missing count)
    list(JOIN missing "\n  " text)
    message(FATAL_ERROR "check_catalog: ${count} of ${checked} rows up to phase ${PHASE} have no "
        "test and no cs-deviation, cs-n/a or cs-pending line:\n  ${text}")
endif()
message(STATUS "check_catalog: phase ${PHASE}: all ${checked} rows covered "
    "(tests ${tests}, deviations ${deviations}, n/a ${na}, pending ${pending})")
