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
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_HOST_APPLE)
      # We do not use the install_name_tool when cross-compiling for macOS.
      # So disable this tool check in further enable_language() command.
      set(CMAKE_PLATFORM_HAS_INSTALLNAME FALSE)
    endif()
    enable_language(${language})
  endif()
  unset(_enabled_languages)
endmacro()
