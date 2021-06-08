# Try to find the GMP library
# https://gmplib.org/
#
# This module supports requiring a minimum version, e.g. you can do
#   find_package(GMP 6.0.0)
# to require version 6.0.0 to newer of GMP.
#
# Once done this will define
#
#  GMP_FOUND - system has GMP lib with correct version
#  GMP_INCLUDES - the GMP include directory
#  GMP_LIBRARIES - the GMP library
#  GMP_VERSION - GMP version
#
# Copyright (c) 2016 Jack Poulson, <jack.poulson@gmail.com>
# Redistribution and use is allowed according to the terms of the BSD license.

find_path(GMP_INCLUDES NAMES gmp.h PATHS $ENV{GMPDIR} ${INCLUDE_INSTALL_DIR})

# Set GMP_FIND_VERSION to 5.1.0 if no minimum version is specified
if(NOT GMP_FIND_VERSION)
  if(NOT GMP_FIND_VERSION_MAJOR)
    set(GMP_FIND_VERSION_MAJOR 5)
  endif()
  if(NOT GMP_FIND_VERSION_MINOR)
    set(GMP_FIND_VERSION_MINOR 1)
  endif()
  if(NOT GMP_FIND_VERSION_PATCH)
    set(GMP_FIND_VERSION_PATCH 0)
  endif()
  set(GMP_FIND_VERSION
    "${GMP_FIND_VERSION_MAJOR}.${GMP_FIND_VERSION_MINOR}.${GMP_FIND_VERSION_PATCH}")
endif()

message("GMP_INCLUDES=${GMP_INCLUDES}")
if(GMP_INCLUDES)
  # Since the GMP version macros may be in a file included by gmp.h of the form
  # gmp-.*[_]?.*.h (e.g., gmp-x86_64.h), we search each of them.
  file(GLOB GMP_HEADERS "${GMP_INCLUDES}/gmp.h" "${GMP_INCLUDES}/gmp-*.h")
  foreach(gmp_header_filename ${GMP_HEADERS})
    file(READ "${gmp_header_filename}" _gmp_version_header)
    string(REGEX MATCH
      "define[ \t]+__GNU_MP_VERSION[ \t]+([0-9]+)" _gmp_major_version_match
      "${_gmp_version_header}")
    if(_gmp_major_version_match)
      set(GMP_MAJOR_VERSION "${CMAKE_MATCH_1}")
      string(REGEX MATCH "define[ \t]+__GNU_MP_VERSION_MINOR[ \t]+([0-9]+)"
        _gmp_minor_version_match "${_gmp_version_header}")
      set(GMP_MINOR_VERSION "${CMAKE_MATCH_1}")
      string(REGEX MATCH "define[ \t]+__GNU_MP_VERSION_PATCHLEVEL[ \t]+([0-9]+)"
        _gmp_patchlevel_version_match "${_gmp_version_header}")
      set(GMP_PATCHLEVEL_VERSION "${CMAKE_MATCH_1}")
      set(GMP_VERSION
        ${GMP_MAJOR_VERSION}.${GMP_MINOR_VERSION}.${GMP_PATCHLEVEL_VERSION})
    endif()
  endforeach()

  # Check whether found version exists and exceeds the minimum requirement
  if(NOT GMP_VERSION)
    set(GMP_VERSION_OK FALSE)
    message(STATUS "GMP version was not detected")
  elseif(${GMP_VERSION} VERSION_LESS ${GMP_FIND_VERSION})
    set(GMP_VERSION_OK FALSE)
    message(STATUS "GMP version ${GMP_VERSION} found in ${GMP_INCLUDES}, "
                   "but at least version ${GMP_FIND_VERSION} is required")
  else()
    set(GMP_VERSION_OK TRUE)
  endif()
endif()

find_library(GMP_LIBRARIES gmp PATHS $ENV{GMPDIR} ${LIB_INSTALL_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(gmp DEFAULT_MSG
                                  GMP_INCLUDES GMP_LIBRARIES GMP_VERSION_OK)
mark_as_advanced(GMP_INCLUDES GMP_LIBRARIES)
