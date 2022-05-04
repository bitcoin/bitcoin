# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Going to be compatible with Qt 6.2 LTS.
# See https://doc.qt.io/qt-6.2/cmake-get-started.html
cmake_minimum_required(VERSION 3.16)

project("Bitcoin Core"
  VERSION 23.0.0
  DESCRIPTION "Bitcoin client software"
  HOMEPAGE_URL "https://bitcoincore.org/"
  LANGUAGES CXX C ASM
)

add_compile_definitions(PACKAGE_NAME="${PROJECT_NAME}")
add_compile_definitions(PACKAGE_VERSION="${PROJECT_VERSION}")
add_compile_definitions(CLIENT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
add_compile_definitions(CLIENT_VERSION_MINOR=${PROJECT_VERSION_MINOR})
add_compile_definitions(CLIENT_VERSION_BUILD=${PROJECT_VERSION_PATCH})
add_compile_definitions(CLIENT_VERSION_IS_RELEASE=false)
add_compile_definitions(PACKAGE_BUGREPORT="https://github.com/bitcoin/bitcoin/issues")
add_compile_definitions(PACKAGE_URL="${PROJECT_HOMEPAGE_URL}")
add_compile_definitions(COPYRIGHT_YEAR=2022)
add_compile_definitions(COPYRIGHT_HOLDERS="The %s developers")
add_compile_definitions(COPYRIGHT_HOLDERS_SUBSTITUTION="${PROJECT_NAME}")

add_compile_options(-pipe -O2)
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  link_libraries(-Wl,-O2)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(CheckCXXSourceCompiles)
include(CheckCXXSymbolExists)
include(TestBigEndian)

# Global checks.
test_big_endian(WORDS_BIGENDIAN)
check_cxx_symbol_exists(fdatasync "unistd.h" HAVE_FDATASYNC)
if(NOT HAVE_FDATASYNC)
  set(HAVE_FDATASYNC 0)
endif()

option(USE_CCACHE "Use ccache for building" ON)
if(USE_CCACHE)
  find_program(CCACHE ccache)
  if(CCACHE)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  endif()
endif()

include(cmake/subtree-crc32c.cmake)
include(cmake/subtree-leveldb.cmake)
include(cmake/subtree-minisketch.cmake)
include(cmake/subtree-univalue.cmake)
