# Registers a CTest test that runs one of this project's programs.
function(nepdate_c_add_test name target)
    add_test(NAME ${name} COMMAND ${target})
endfunction()
