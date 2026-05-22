# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# A wrapper around CMake’s `enable_language()` command that
# applies Bitcoin Core-specific settings and checks.
# Implemented as a macro (not a function) because
# `enable_language()` may not be called from within a function.
macro(bitcoincore_enable_language language)
  get_cmake_property(_enabled_languages ENABLED_LANGUAGES)
  if(NOT "${language}" IN_LIST _enabled_languages)
    if(CMAKE_VERSION VERSION_LESS 4.2 AND CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_HOST_APPLE)
      # We do not use the install_name_tool when cross-compiling for macOS.
      # However, CMake < 4.2 still searches for Apple's version of the tool,
      # which causes an error during configuration.
      # See:
      # - https://gitlab.kitware.com/cmake/cmake/-/issues/27069
      # - https://gitlab.kitware.com/cmake/cmake/-/merge_requests/10955
      # So disable this tool check in further enable_language() commands.
      set(CMAKE_INSTALL_NAME_TOOL "${CMAKE_COMMAND} -E true")
    endif()
    enable_language(${language})
  endif()
  unset(_enabled_languages)
endmacro()
