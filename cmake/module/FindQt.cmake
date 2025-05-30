# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

#[=======================================================================[
FindQt
------

Finds the Qt headers and libraries.

This is a wrapper around find_package() command that:
 - facilitates searching in various build environments
 - prints a standard log message

#]=======================================================================]

set(_qt_homebrew_prefix)
if(CMAKE_HOST_APPLE)
  find_program(HOMEBREW_EXECUTABLE brew)
  if(HOMEBREW_EXECUTABLE)
    execute_process(
      COMMAND ${HOMEBREW_EXECUTABLE} --prefix qt@${Qt_FIND_VERSION_MAJOR}
      OUTPUT_VARIABLE _qt_homebrew_prefix
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()
endif()

find_package(Qt${Qt_FIND_VERSION_MAJOR} ${Qt_FIND_VERSION}
  COMPONENTS ${Qt_FIND_COMPONENTS}
  HINTS ${_qt_homebrew_prefix}
  PATH_SUFFIXES Qt${Qt_FIND_VERSION_MAJOR}  # Required on OpenBSD systems.
)
unset(_qt_homebrew_prefix)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Qt
  REQUIRED_VARS Qt${Qt_FIND_VERSION_MAJOR}_DIR
  VERSION_VAR Qt${Qt_FIND_VERSION_MAJOR}_VERSION
)

foreach(component IN LISTS Qt_FIND_COMPONENTS ITEMS "")
  mark_as_advanced(Qt${Qt_FIND_VERSION_MAJOR}${component}_DIR)
endforeach()
