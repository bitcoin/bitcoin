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

    set(_description "flags that are appended to the command line after all other flags added by the build system. This variable is intended for debugging and special builds.")
    set(APPEND_CPPFLAGS "" CACHE STRING "Preprocessor ${_description}")
    set(APPEND_LDFLAGS "" CACHE STRING "Linker ${_description}")
    if("${language}" MATCHES "^C$")
      set(APPEND_CFLAGS "" CACHE STRING "C compiler ${_description}")
    endif()
    if("${language}" MATCHES "^(CXX|OBJCXX)$")
      set(APPEND_CXXFLAGS "" CACHE STRING "(Objective) C++ compiler ${_description}")
      string(APPEND CMAKE_${language}_COMPILE_OBJECT " ${APPEND_CPPFLAGS} ${APPEND_CXXFLAGS}")
      string(APPEND CMAKE_${language}_CREATE_SHARED_LIBRARY " ${APPEND_LDFLAGS}")
      string(APPEND CMAKE_${language}_LINK_EXECUTABLE " ${APPEND_LDFLAGS}")
    endif()
    unset(_description)
  endif()
  unset(_enabled_languages)
endmacro()
