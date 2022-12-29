# Copyright (c) 2018 The Bitcoin developers

find_program(BREW brew)

function(find_brew_prefix VAR NAME)
    if(NOT BREW)
        return()
    endif()

    if(DEFINED ${VAR})
        return()
    endif()

    execute_process(
        COMMAND ${BREW} --prefix ${NAME}
        OUTPUT_VARIABLE PREFIX
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(${VAR} ${PREFIX} PARENT_SCOPE)
endfunction()
