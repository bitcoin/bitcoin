# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

if(NOT MSVC)
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
      list(APPEND CMAKE_C_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
      list(APPEND CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
    endif()
  else()
    set(WITH_CCACHE OFF)
  endif()
  if(WITH_CCACHE)
    if(NOT CMAKE_DISABLE_PRECOMPILE_HEADERS)
      # ccache only allows us to reasonable set the necessary pch option as of version 4.8.
      # Rather than attempting to parse the version, try running with the necessary options and a dummy compile.
      execute_process(COMMAND "${CMAKE_CXX_COMPILER_LAUNCHER}" sloppiness=pch_defines,time_macros ${CMAKE_CXX_COMPILER} --version
        RESULT_VARIABLE CCACHE_TEST_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
      )
      if(NOT CCACHE_TEST_RESULT)
        list(APPEND CMAKE_CXX_COMPILER_LAUNCHER sloppiness=pch_defines,time_macros)
        # According to the ccache docs clang requires this option.
        try_append_cxx_flags("-Xclang -fno-pch-timestamp" TARGET core_interface SKIP_LINK)
      else()
        list(APPEND configure_warnings "For pre-compiled headers support with ccache, version 4.8+ is required. Disabling pch.")
        set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)
      endif()
    endif()
    try_append_cxx_flags("-fdebug-prefix-map=A=B" TARGET core_interface SKIP_LINK
      IF_CHECK_PASSED "-fdebug-prefix-map=${PROJECT_SOURCE_DIR}=."
    )
    try_append_cxx_flags("-fmacro-prefix-map=A=B" TARGET core_interface SKIP_LINK
      IF_CHECK_PASSED "-fmacro-prefix-map=${PROJECT_SOURCE_DIR}=."
    )
  endif()
endif()

mark_as_advanced(CCACHE_EXECUTABLE)
