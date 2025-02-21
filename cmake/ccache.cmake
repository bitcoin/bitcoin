# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# The <LANG>_COMPILER_LAUNCHER target property, used to integrate
# ccache, is supported only by the Makefile and Ninja generators.
if(CMAKE_GENERATOR MATCHES "Make|Ninja")
  find_program(CCACHE_EXECUTABLE ccache)
  if(CCACHE_EXECUTABLE)
    execute_process(
      COMMAND readlink -f ${CMAKE_CXX_COMPILER}
      OUTPUT_VARIABLE compiler_resolved_link
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(CCACHE_EXECUTABLE STREQUAL compiler_resolved_link AND NOT WITH_CCACHE)
      list(APPEND configure_warnings
        "Disabling ccache was attempted using -DWITH_CCACHE=${WITH_CCACHE}, but ccache masquerades as the compiler."
      )
      set(WITH_CCACHE ON)
    elseif(WITH_CCACHE)
      foreach(lang IN ITEMS C CXX OBJCXX)
        if(DEFINED CMAKE_${lang}_COMPILER_LAUNCHER)
          list(APPEND configure_warnings
            "CMAKE_${lang}_COMPILER_LAUNCHER is already set. Skipping built-in ccache support for ${lang}."
          )
        else()
          # Use `cmake -E env` as a cross-platform way to set the CCACHE_NOHASHDIR environment variable,
          # increasing ccache hit rate e.g. in case of multiple build directories or git worktrees.
          set(CMAKE_${lang}_COMPILER_LAUNCHER ${CMAKE_COMMAND} -E env CCACHE_NOHASHDIR=1 ${CCACHE_EXECUTABLE})
        endif()
      endforeach()
    endif()
  else()
    set(WITH_CCACHE OFF)
  endif()
else()
  set(WITH_CCACHE "Built-in ccache support for the '${CMAKE_GENERATOR}' generator is not available.")
endif()

mark_as_advanced(CCACHE_EXECUTABLE)
