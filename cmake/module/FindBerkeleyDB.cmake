# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

#[=======================================================================[
FindBerkeleyDB
--------------

Finds the Berkeley DB headers and library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides imported target ``BerkeleyDB::BerkeleyDB``, if
Berkeley DB has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``BerkeleyDB_FOUND``
  "True" if Berkeley DB found.

``BerkeleyDB_VERSION``
  The MAJOR.MINOR version of Berkeley DB found.

#]=======================================================================]

set(_BerkeleyDB_homebrew_prefix)
if(CMAKE_HOST_APPLE)
  find_program(HOMEBREW_EXECUTABLE brew)
  if(HOMEBREW_EXECUTABLE)
    # The Homebrew package manager installs the berkeley-db* packages as
    # "keg-only", which means they are not symlinked into the default prefix.
    # To find such a package, the find_path() and find_library() commands
    # need additional path hints that are computed by Homebrew itself.
    execute_process(
      COMMAND ${HOMEBREW_EXECUTABLE} --prefix berkeley-db@4
      OUTPUT_VARIABLE _BerkeleyDB_homebrew_prefix
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()
endif()

find_path(BerkeleyDB_INCLUDE_DIR
  NAMES db_cxx.h
  HINTS ${_BerkeleyDB_homebrew_prefix}/include
  PATH_SUFFIXES 4.8 48 db4.8 4 db4 5.3 db5.3 5 db5
)
mark_as_advanced(BerkeleyDB_INCLUDE_DIR)
unset(_BerkeleyDB_homebrew_prefix)

if(NOT BerkeleyDB_LIBRARY)
  if(VCPKG_TARGET_TRIPLET)
    # The vcpkg package manager installs the berkeleydb package with the same name
    # of release and debug libraries. Therefore, the default search paths set by
    # vcpkg's toolchain file cannot be used to search libraries as the debug one
    # will always be found.
    set(CMAKE_FIND_USE_CMAKE_PATH FALSE)
  endif()

  get_filename_component(_BerkeleyDB_lib_hint "${BerkeleyDB_INCLUDE_DIR}" DIRECTORY)

  find_library(BerkeleyDB_LIBRARY_RELEASE
    NAMES db_cxx-4.8 db4_cxx db48 db_cxx-5.3 db_cxx-5 db_cxx libdb48
    NAMES_PER_DIR
    HINTS ${_BerkeleyDB_lib_hint}
    PATH_SUFFIXES lib
  )
  mark_as_advanced(BerkeleyDB_LIBRARY_RELEASE)

  find_library(BerkeleyDB_LIBRARY_DEBUG
    NAMES db_cxx-4.8 db4_cxx db48 db_cxx-5.3 db_cxx-5 db_cxx libdb48
    NAMES_PER_DIR
    HINTS ${_BerkeleyDB_lib_hint}
    PATH_SUFFIXES debug/lib
  )
  mark_as_advanced(BerkeleyDB_LIBRARY_DEBUG)

  unset(_BerkeleyDB_lib_hint)
  unset(CMAKE_FIND_USE_CMAKE_PATH)

  include(SelectLibraryConfigurations)
  select_library_configurations(BerkeleyDB)
  # The select_library_configurations() command sets BerkeleyDB_FOUND, but we
  # want the one from the find_package_handle_standard_args() command below.
  unset(BerkeleyDB_FOUND)
endif()

if(BerkeleyDB_INCLUDE_DIR)
  file(STRINGS "${BerkeleyDB_INCLUDE_DIR}/db.h" _BerkeleyDB_version_strings REGEX "^#define[\t ]+DB_VERSION_(MAJOR|MINOR|PATCH)[ \t]+[0-9]+.*")
  string(REGEX REPLACE ".*#define[\t ]+DB_VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" _BerkeleyDB_version_major "${_BerkeleyDB_version_strings}")
  string(REGEX REPLACE ".*#define[\t ]+DB_VERSION_MINOR[ \t]+([0-9]+).*" "\\1" _BerkeleyDB_version_minor "${_BerkeleyDB_version_strings}")
  string(REGEX REPLACE ".*#define[\t ]+DB_VERSION_PATCH[ \t]+([0-9]+).*" "\\1" _BerkeleyDB_version_patch "${_BerkeleyDB_version_strings}")
  unset(_BerkeleyDB_version_strings)
  # The MAJOR.MINOR.PATCH version will be logged in the following find_package_handle_standard_args() command.
  set(_BerkeleyDB_full_version ${_BerkeleyDB_version_major}.${_BerkeleyDB_version_minor}.${_BerkeleyDB_version_patch})
  set(BerkeleyDB_VERSION ${_BerkeleyDB_version_major}.${_BerkeleyDB_version_minor})
  unset(_BerkeleyDB_version_major)
  unset(_BerkeleyDB_version_minor)
  unset(_BerkeleyDB_version_patch)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BerkeleyDB
  REQUIRED_VARS BerkeleyDB_LIBRARY BerkeleyDB_INCLUDE_DIR
  VERSION_VAR _BerkeleyDB_full_version
)
unset(_BerkeleyDB_full_version)

if(BerkeleyDB_FOUND AND NOT TARGET BerkeleyDB::BerkeleyDB)
  add_library(BerkeleyDB::BerkeleyDB UNKNOWN IMPORTED)
  set_target_properties(BerkeleyDB::BerkeleyDB PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${BerkeleyDB_INCLUDE_DIR}"
  )
  if(BerkeleyDB_LIBRARY_RELEASE)
    set_property(TARGET BerkeleyDB::BerkeleyDB APPEND PROPERTY
      IMPORTED_CONFIGURATIONS RELEASE
    )
    set_target_properties(BerkeleyDB::BerkeleyDB PROPERTIES
      IMPORTED_LOCATION_RELEASE "${BerkeleyDB_LIBRARY_RELEASE}"
    )
  endif()
  if(BerkeleyDB_LIBRARY_DEBUG)
    set_property(TARGET BerkeleyDB::BerkeleyDB APPEND PROPERTY
      IMPORTED_CONFIGURATIONS DEBUG)
    set_target_properties(BerkeleyDB::BerkeleyDB PROPERTIES
      IMPORTED_LOCATION_DEBUG "${BerkeleyDB_LIBRARY_DEBUG}"
    )
  endif()
endif()
