# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# Ubuntu 22.04 LTS Jammy Jellyfish, https://wiki.ubuntu.com/Releases, EOSS in June 2027:
#  - CMake 3.22.1, https://packages.ubuntu.com/jammy/cmake
#
# Centos Stream 9, EOL in May 2027:
#  - CMake 3.26.5, https://mirror.stream.centos.org/9-stream/AppStream/x86_64/os/Packages/
cmake_minimum_required(VERSION 3.22)
if(POLICY CMP0141)
  # MSVC debug information format flags are selected by an abstraction.
  # We want to use the CMAKE_MSVC_DEBUG_INFORMATION_FORMAT variable
  # to select the MSVC debug information format.
  cmake_policy(SET CMP0141 NEW)
endif()

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

#=============================
# Project / Package metadata
#=============================
set(PACKAGE_NAME "Bitcoin Core")
set(CLIENT_VERSION_MAJOR 27)
set(CLIENT_VERSION_MINOR 99)
set(CLIENT_VERSION_BUILD 0)
set(CLIENT_VERSION_RC 0)
set(CLIENT_VERSION_IS_RELEASE "false")
set(COPYRIGHT_YEAR "2024")

project(BitcoinCore
  VERSION ${CLIENT_VERSION_MAJOR}.${CLIENT_VERSION_MINOR}.${CLIENT_VERSION_BUILD}
  DESCRIPTION "Bitcoin client software"
  HOMEPAGE_URL "https://bitcoincore.org/"
  LANGUAGES NONE
)

set(PACKAGE_VERSION ${PROJECT_VERSION})
if(CLIENT_VERSION_RC GREATER 0)
  string(APPEND PACKAGE_VERSION "rc${CLIENT_VERSION_RC}")
endif()

set(COPYRIGHT_HOLDERS "The %s developers")
set(COPYRIGHT_HOLDERS_FINAL "The ${PACKAGE_NAME} developers")
set(PACKAGE_BUGREPORT "https://github.com/bitcoin/bitcoin/issues")

#=============================
# Language setup
#=============================
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_HOST_APPLE)
  # We do not use the install_name_tool when cross-compiling for macOS.
  # So disable this tool check in further enable_language() commands.
  set(CMAKE_PLATFORM_HAS_INSTALLNAME FALSE)
endif()
enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(configure_warnings)

message("\n")
message("Configure summary")
message("=================")
message("C++ compiler .......................... ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}, ${CMAKE_CXX_COMPILER}")
message("\n")
if(configure_warnings)
    message("  ******\n")
    foreach(warning IN LISTS configure_warnings)
      message(WARNING "${warning}")
    endforeach()
    message("  ******\n")
endif()
