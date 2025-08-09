cmake_minimum_required(VERSION 3.22)

#=============================
# Project / Package Metadata
#=============================
project(minisketch
  VERSION 0.0.1
  DESCRIPTION "A library for BCH-based set reconciliation"
  HOMEPAGE_URL "https://github.com/bitcoin-core/minisketch"
  LANGUAGES CXX
)

# ============================================================
# Project Initialization
# ============================================================
enable_testing()
include(CTestUseLaunchers)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()

# Prevent include directories from parent project from leaking into this one.
set_property(DIRECTORY PROPERTY INCLUDE_DIRECTORIES "")

#=============================
# Language Setup
#=============================
if(DEFINED CMAKE_CXX_STANDARD)
  if(CMAKE_CXX_STANDARD EQUAL 98 OR CMAKE_CXX_STANDARD LESS 11)
    message(FATAL_ERROR "This project requires at least C++11")
  endif()
else()
  set(CMAKE_CXX_STANDARD 11)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT DEFINED CMAKE_CXX_EXTENSIONS)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

#=============================
# Configurable Options
#=============================
option(MINISKETCH_INSTALL "Enable installation." ${PROJECT_IS_TOP_LEVEL})
if(NOT PROJECT_IS_TOP_LEVEL)
  mark_as_advanced(MINISKETCH_INSTALL)
endif()

option(MINISKETCH_BUILD_TESTS "Build tests." ON)
option(MINISKETCH_BUILD_BENCHMARK "Build benchmark." OFF)

set(supported_fields "")
set(have_enabled_fields NO)
set(have_disabled_fields NO)
foreach(i RANGE 2 64)
  list(APPEND supported_fields ${i})
endforeach()
if(NOT DEFINED MINISKETCH_FIELDS)
  set(MINISKETCH_FIELDS ${supported_fields} CACHE STRING "Semicolon-separated list of field sizes to build. Default=all. Available sizes: ${supported_fields}.")
endif()
foreach(field IN LISTS supported_fields)
  if(field IN_LIST MINISKETCH_FIELDS)
    set(have_enabled_fields YES)
  else()
    set(have_disabled_fields YES)
    add_compile_definitions(DISABLE_FIELD_${field})
  endif()
endforeach()
if(NOT have_enabled_fields)
  message(FATAL_ERROR "No field sizes are enabled.")
endif()
unset(have_enabled_fields)
unset(supported_fields)

#=============================
# Build Options
#=============================
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(MSVC)
  add_compile_options(/Zc:__cplusplus)
endif()

if(MINGW)
  add_link_options(-static)
endif()

#=============================
# Diagnostics Options
#=============================
if(MSVC)
  # For both MSVC's cl.exe and clang-cl compilers.
  add_compile_options(/W3)  # Production quality warning level. Enables -Wall Clang's core option.
else()
  add_compile_options(-Wall)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(/wd4060)  # Disable warning C4060 "switch statement contains no 'case' or 'default' labels".
  add_compile_options(/wd4065)  # Disable warning C4065 "switch statement contains 'default' but no 'case' labels".
  add_compile_options(/wd4146)  # Disable warning C4146 "unary minus operator applied to unsigned type, result still unsigned".
  add_compile_options(/wd4244)  # Disable warning C4244 "conversion from 'type1' to 'type2', possible loss of data".
else()
  add_compile_options(-Wundef)
endif()

#=============================
# Main Processing
#=============================
include(SystemIntrospection)
add_subdirectory(src)

include(PrintConfigureSummary)
print_configure_summary()
