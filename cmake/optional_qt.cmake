# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set(WITH_GUI "auto" CACHE STRING "Build GUI ([auto], Qt5, no)")
set(with_gui_values "auto" "Qt5" "no")
if(NOT WITH_GUI IN_LIST with_gui_values)
  message(FATAL_ERROR "WITH_GUI value is \"${WITH_GUI}\", but must be one of \"auto\", \"Qt5\" or \"no\".")
endif()

if(WITH_GUI AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.16)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    set(CMAKE_OBJCXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_OBJCXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_OBJCXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
  elseif(WITH_GUI STREQUAL "auto")
    message(WARNING "CMake >= 3.16 is required to build the GUI for macOS.\n"
                    "To skip this warning check, use \"-DWITH_GUI=no\".\n")
    set(WITH_GUI "no")
  else()
    message(FATAL_ERROR "CMake >= 3.16 is required to build the GUI for macOS.")
  endif()
endif()

if(WITH_GUI STREQUAL "auto")
  set(bitcoin_qt_versions Qt5)
else()
  set(bitcoin_qt_versions ${WITH_GUI})
endif()

if(WITH_GUI)
  set(QT_NO_CREATE_VERSIONLESS_FUNCTIONS ON)
  set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)

  if(CMAKE_HOST_APPLE)
    execute_process(
      COMMAND brew --prefix qt@5
      OUTPUT_VARIABLE qt5_brew_prefix
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()

  if(WITH_GUI STREQUAL "auto")
    # The PATH_SUFFIXES option is required on OpenBSD systems.
    find_package(QT NAMES ${bitcoin_qt_versions}
      COMPONENTS Core
      HINTS ${qt5_brew_prefix}
      PATH_SUFFIXES Qt5
    )
    if(QT_FOUND)
      set(WITH_GUI Qt${QT_VERSION_MAJOR})
    else()
      message(WARNING "Qt not found, disabling.\n"
                      "To skip this warning check, use \"-DWITH_GUI=no\".\n")
      set(WITH_GUI "no")
    endif()
  endif()
endif()
