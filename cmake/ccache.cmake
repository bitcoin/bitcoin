# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# The <LANG>_COMPILER_LAUNCHER target property, used to integrate
# Ccache, is supported only by the Makefiles and Ninja generators.
if(CMAKE_GENERATOR MATCHES "Makefiles|Ninja")
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
        set(CMAKE_${lang}_COMPILER_LAUNCHER
          ${CMAKE_COMMAND} -E env CCACHE_NOHASHDIR=1 ${CCACHE_EXECUTABLE}
        )
      endforeach()
    endif()
  else()
    set(WITH_CCACHE OFF)
  endif()
  if(WITH_CCACHE)
    try_append_cxx_flags("-fdebug-prefix-map=A=B" SKIP_LINK
      TARGET core_interface
      # Propagate these flags, which apply to both C++ and C, to the secp256k1 subtree.
      VAR SECP256K1_APPEND_CFLAGS
      IF_CHECK_PASSED "-fdebug-prefix-map=${PROJECT_SOURCE_DIR}=." "-fdebug-prefix-map=${CMAKE_BINARY_DIR}=."
    )
    try_append_cxx_flags("-fmacro-prefix-map=A=B" TARGET core_interface SKIP_LINK
      IF_CHECK_PASSED "-fmacro-prefix-map=${PROJECT_SOURCE_DIR}=."
    )
  endif()
endif()

mark_as_advanced(CCACHE_EXECUTABLE)
