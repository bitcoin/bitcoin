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
    if(CCACHE_EXECUTABLE STREQUAL compiler_resolved_link)
      set(WITH_CCACHE "ccache masquerades as the compiler")
    elseif(WITH_CCACHE)
      list(APPEND CMAKE_C_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
      list(APPEND CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
    endif()
    try_append_cxx_flags("-fdebug-prefix-map=A=B" TARGET core_interface SKIP_LINK
      IF_CHECK_PASSED "-fdebug-prefix-map=${PROJECT_SOURCE_DIR}=."
    )
    try_append_cxx_flags("-fmacro-prefix-map=A=B" TARGET core_interface SKIP_LINK
      IF_CHECK_PASSED "-fmacro-prefix-map=${PROJECT_SOURCE_DIR}=."
    )
  else()
    set(WITH_CCACHE OFF)
  endif()
endif()

mark_as_advanced(CCACHE_EXECUTABLE)
