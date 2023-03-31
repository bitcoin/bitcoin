# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# IMPORTANT: Changes which affect binary results may not be quietly gated
#            by CMake version.
#
# Debian 10 Buster, https://wiki.debian.org/LTS, EOL 2024:
#  - CMake 3.13.4, https://packages.debian.org/buster/cmake
#
# Ubuntu 22.04 Jammy, https://wiki.ubuntu.com/Releases, EOL 2032:
#  - CMake 3.22.1, https://packages.ubuntu.com/jammy/cmake
#
# Visual Studio 17 2022, https://visualstudio.microsoft.com:
#  - CMake 3.24
#
# All policies known to the running version of CMake and introduced
# in the 3.24 version or earlier will be set to use NEW behavior.
# All policies introduced in later versions will be unset.
# See: https://cmake.org/cmake/help/latest/manual/cmake-policies.7.html
cmake_minimum_required(VERSION 3.13...3.24)

project("Bitcoin Core"
  VERSION 24.99.0
  DESCRIPTION "Bitcoin client software"
  HOMEPAGE_URL "https://bitcoincore.org/"
  LANGUAGES CXX ASM
)

# Configurable options.
# When adding a new option, end the <help_text> with a full stop for consistency.
include(CMakeDependentOption)
cmake_dependent_option(CXX20 "Enable compilation in C++20 mode." OFF "NOT MSVC" ON)

if(CXX20)
  set(CMAKE_CXX_STANDARD 20)
else()
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(configure_warnings)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.14)
  include(CheckPIESupported)
  check_pie_supported(OUTPUT_VARIABLE check_pie_output LANGUAGES CXX)
  if(CMAKE_CXX_LINK_PIE_SUPPORTED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  endif()
else()
  check_cxx_source_links_with_flags(-fPIE "int main(){}" COMPILER_SUPPORTS_PIE)
  if(COMPILER_SUPPORTS_PIE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  endif()
endif()

message("\n")
message("Configure summary")
message("=================")
get_directory_property(definitions COMPILE_DEFINITIONS)
string(REPLACE ";" " " definitions "${definitions}")
message("Preprocessor defined macros ........... ${definitions}")
message("C compiler ............................ ${CMAKE_C_COMPILER}")
message("CFLAGS ................................ ${CMAKE_C_FLAGS}")
message("C++ compiler .......................... ${CMAKE_CXX_COMPILER}")
message("CXXFLAGS .............................. ${CMAKE_CXX_FLAGS}")
get_directory_property(common_compile_options COMPILE_OPTIONS)
string(REPLACE ";" " " common_compile_options "${common_compile_options}")
message("Common compile options ................ ${common_compile_options}")
get_directory_property(common_link_options LINK_OPTIONS)
string(REPLACE ";" " " common_link_options "${common_link_options}")
message("Common link options ................... ${common_link_options}")
if(DEFINED CMAKE_BUILD_TYPE)
  message("Build type:")
  message(" - CMAKE_BUILD_TYPE ................... ${CMAKE_BUILD_TYPE}")
  string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type)
  message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_${build_type}}")
  message(" - CXXFLAGS ........................... ${CMAKE_CXX_FLAGS_${build_type}}")
  message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_${build_type}}")
  message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_${build_type}}")
else()
  message("Available configurations .............. ${CMAKE_CONFIGURATION_TYPES}")
  message("Debug configuration:")
  message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_DEBUG}")
  message(" - CXXFLAGS ........................... ${CMAKE_CXX_FLAGS_DEBUG}")
  message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
  message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
  message("Release configuration:")
  message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_RELEASE}")
  message(" - CXXFLAGS ........................... ${CMAKE_CXX_FLAGS_RELEASE}")
  message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
  message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
endif()
message("\n")
if(configure_warnings)
    message("  ******\n")
    foreach(warning IN LISTS configure_warnings)
      message(WARNING "${warning}")
    endforeach()
    message("  ******\n")
endif()
