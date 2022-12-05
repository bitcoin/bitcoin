#[[.rst
#
# FindGuile
# ---------
# Find the development libraries for Guile.
#
# Exported Vars
# ~~~~~~~~~~~~~
#
# .. variable:: Guile_FOUND
#
#    Set to *true* if Guile was found.
#
# .. variable:: Guile_INCLUDE_DIRS
#
#    A list of include directories.
#
# .. variable:: Guile_LIBRARIES
#
#    A list of libraries needed to build you project.
#
# .. variable:: Guile_VERSION_STRING
#
#    Guile full version.
#
# .. variable:: Guile_VERSION_MAJOR
#
#    Guile major version.
#
# .. variable:: Guile_VERSION_MINOR
#
#    Guile minor version.
#
# .. variable:: Guile_VERSION_PATCH
#
#    Guile patch version.
#
# .. variable:: Guile_EXECUTABLE
#
#    Guile executable (optional).
#
# .. variable:: Guile_CONFIG_EXECUTABLE
#
#    Guile configuration executable (optional).
#
# .. variable:: Guile_ROOT_DIR
#
#    Guile installation root dir (optional).
#
# .. variable:: Guile_SITE_DIR
#
#    Guile installation module site dir (optional).
#
# .. variable:: Guile_EXTENSION_DIR
#
#    Guile installation extension dir (optional).
#
# Control VARS
# ~~~~~~~~~~~~
# :envvar:`Guile_ROOT_DIR`
#
#   Use this variable to provide hints to :filename:`find_{*}` commands,
#   you may pass it to :command:`cmake` or set the environtment variable.
#
# .. code-block:: cmake
#
#    % cmake . -Bbuild -DGuile_ROOT_DIR=<extra-path>
#
#    # or
#    % export Guile_ROOT_DIR=<extra-path>;
#    % cmake . -Bbuild
#
#    # or
#    % Guile_ROOT_DIR=<extra-path> cmake . -Bbuild
#
#
#]]


#[[.rst
#
# Copyright © 2016, Edelcides Gonçalves <eatg75 |0x40| gmail>
#
# Permission to use, copy, modify, and/or distribute this software for
# any purpose with or without fee is hereby granted, provided that the
# above copyright notice and this permission notice appear in all
# copies.
#
# *THE SOFTWARE IS PROVIDED* **AS IS** *AND ISC DISCLAIMS ALL
# WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL ISC BE
# LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
# OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
# PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
# TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE*.
#
# This file is not part of CMake
#
#]]


include (SelectLibraryConfigurations)
include (FindPackageHandleStandardArgs)

function (_guile_find_component_include_dir component header)
  find_path ("${component}_INCLUDE_DIR"
    ${header}
    HINTS
    "${GUile_ROOT_DIR}"
    ENV Guile_ROOT_DIR
    PATH_SUFFIXES
    Guile guile Guile-2.0 guile-2.0 Guile/2.0 guile/2.0 GC
    gc)

  set ("${component}_INCLUDE_DIR" "${${component}_INCLUDE_DIR}"
    PARENT_SCOPE)
endfunction ()

function (_guile_find_component_library component_name component)

  find_library ("${component_name}_LIBRARY_RELEASE"
    NAMES "${component}" "${component}-2.0"
    HINTS
    "${GUile_ROOT_DIR}"
    ENV Guile_ROOT_DIR
    PATHS
    /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
    /usr/lib64/${CMAKE_LIBRARY_ARCHITECTURE}
    /usr/lib32/${CMAKE_LIBRARY_ARCHITECTURE})

  if  (${component_name}_LIBRARY_RELEASE)
    select_library_configurations (${component_name})
    set ("${component_name}_LIBRARY_RELEASE"
      "${${component_name}_LIBRARY_RELEASE}" PARENT_SCOPE)
    set ("${component_name}_LIBRARY"
      "${${component_name}_LIBRARY}" PARENT_SCOPE)
    set ("${component_name}_LIBRARIES"
      "${${component_name}_LIBRARIES}" PARENT_SCOPE)
  endif ()
endfunction ()

function (_guile_find_version_2 header_file macro_name)
  file (STRINGS "${header_file}" _VERSION
    REGEX "#define[\t ]+${macro_name}[\t ]+[0-9]+")

  if (_VERSION)
    string (REGEX REPLACE
      ".*#define[\t ]+${macro_name}[\t ]+([0-9]+).*"
      "\\1" _VERSION_VALUE "${_VERSION}")
    if ("${_VERSION}" STREQUAL "${_VERSION_VALUE}")
      set (_VERSION_FOUND 0 PARENT_SCOPE)
    else ()
      set (_VERSION_FOUND 1 PARENT_SCOPE)
      set (_VERSION "${_VERSION_VALUE}" PARENT_SCOPE)
    endif ()
  else ()
    set (_VERSION_FOUND 0 PARENT_SCOPE)
  endif ()
endfunction ()


##### Entry Point #####

set (Guile_FOUND)
set (Guile_INCLUDE_DIRS)
set (Guile_LIBRARIES)
set (Guile_VERSION_STRING)
set (Guile_VERSION_MAJOR)
set (Guile_VERSION_MINOR)
set (Guile_VERSION_PATCH)
set (Guile_EXECUTABLE)

_guile_find_component_include_dir (Guile "libguile.h")
if (Guile_INCLUDE_DIR)
  _guile_find_version_2 ("${Guile_INCLUDE_DIR}/libguile/version.h"
    SCM_MAJOR_VERSION)
  if (_VERSION_FOUND)
    set (Guile_VERSION_MAJOR "${_VERSION}")
  else ()
    message (FATAL_ERROR "FindGuile: Failed to find Guile_MAJOR_VERSION.")
  endif ()

  _guile_find_version_2 ("${Guile_INCLUDE_DIR}/libguile/version.h"
    SCM_MINOR_VERSION)
  if (_VERSION_FOUND)
    set (Guile_VERSION_MINOR "${_VERSION}")
  else ()
    message (FATAL_ERROR "FindGuile: Failed to find Guile_MINOR_VERSION.")
  endif ()

  _guile_find_version_2 ("${Guile_INCLUDE_DIR}/libguile/version.h"
    SCM_MICRO_VERSION)
  if (_VERSION_FOUND)
    set (Guile_VERSION_PATCH "${_VERSION}")
  else ()
    message (FATAL_ERROR "FindGuile: Failed to find Guile_MICRO_VERSION.")
  endif ()
  set (Guile_VERSION_STRING "${Guile_VERSION_MAJOR}.${Guile_VERSION_MINOR}.${Guile_VERSION_PATCH}")

  unset (_VERSION_FOUND)
  unset (_VERSION)
endif ()

_guile_find_component_include_dir (Guile_GC "gc.h")
_guile_find_component_library (Guile guile)
_guile_find_component_library (Guile_GC gc)

find_program (Guile_EXECUTABLE
  guile
  DOC "Guile executable.")

if (Guile_EXECUTABLE)
  execute_process (COMMAND ${Guile_EXECUTABLE} --version
    RESULT_VARIABLE _status
    OUTPUT_VARIABLE _output
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  string (REGEX REPLACE ".*\\(GNU Guile\\)[\t ]+([0-9]+)\\..*" "\\1"
    _guile_ver_major "${_output}")

  string (REGEX REPLACE ".*\\(GNU Guile\\)[\t ]+[0-9]+\\.([0-9]+).*" "\\1"
    _guile_ver_minor "${_output}")

  string (REGEX REPLACE ".*\\(GNU Guile\\)[\t ]+[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1"
    _guile_ver_patch "${_output}")

  set (_version "${_guile_ver_major}.${_guile_ver_minor}.${_guile_ver_patch}")

  if (NOT Guile_FIND_QUIETLY)
    if (NOT Guile_VERSION_STRING STREQUAL _version)
      message (WARNING "FindGuile: Versions provided by library differs from the one provided by executable.")
    endif ()

    if (NOT _status STREQUAL "0")
      message (WARNING "FindGuile: guile (1) process exited abnormally.")
    endif ()
  endif ()

  unset (_version)
  unset (_status)
  unset (_version)
  unset (_guile_ver_major)
  unset (_guile_ver_minor)
  unset (_guile_ver_patch)
endif ()

find_package_handle_standard_args (GC
  "FindGuile: Failed to find dependency GC."
  Guile_GC_INCLUDE_DIR
  Guile_GC_LIBRARY
  Guile_GC_LIBRARIES
  Guile_GC_LIBRARY_RELEASE)

find_package_handle_standard_args (Guile
  REQUIRED_VARS
  Guile_INCLUDE_DIR
  Guile_LIBRARY
  Guile_LIBRARIES
  Guile_LIBRARY_RELEASE
  GC_FOUND
  VERSION_VAR Guile_VERSION_STRING)

if (Guile_FOUND)
  list (APPEND Guile_INCLUDE_DIRS "${Guile_INCLUDE_DIR}"
    "${Guile_GC_INCLUDE_DIR}")

  if (NOT TARGET Guile::Library AND NOT TARGET GC::Library)
    add_library (Guile::GC::Library UNKNOWN IMPORTED)
    set_property (TARGET Guile::GC::Library APPEND
      PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

    set_target_properties (Guile::GC::Library
      PROPERTIES
      INTERFACE_INCLUDE_DIRS
      "${Guile_GC_INCLUDE_DIR}"
      IMPORTED_LOCATION
      "${Guile_GC_LIBRARY}"
      IMPORTED_LOCATION_RELEASE
      "${Guile_GC_LIBRARY_RELEASE}")

    add_library (Guile::Library UNKNOWN IMPORTED)
    set_property (TARGET Guile::Library APPEND
      PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
    set_property (TARGET Guile::Library APPEND
      PROPERTY
      INTERFACE_LINK_LIBRARIES
      Guile::GC::Library)

    set_target_properties (Guile::Library
      PROPERTIES
      INTERFACE_INCLUDE_DIRSr
      "${Guile_INCLUDE_DIR}"
      IMPORTED_LOCATION "${Guile_LIBRARY}"
      IMPORTED_LOCATION_RELEASE
      "${Guile_LIBRARY_RELEASE}")

    set (Guile_LIBRARIES Guile::Library Guile::GC::Library)
  endif ()
endif ()

find_program(Guile_CONFIG_EXECUTABLE
  NAMES guile-config
  DOC "Guile configutration binary")

if (Guile_CONFIG_EXECUTABLE)
  execute_process (COMMAND ${Guile_CONFIG_EXECUTABLE} info prefix
    OUTPUT_VARIABLE Guile_ROOT_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process (COMMAND ${Guile_CONFIG_EXECUTABLE} info sitedir
    OUTPUT_VARIABLE Guile_SITE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process (COMMAND ${Guile_CONFIG_EXECUTABLE} info extensiondir
    OUTPUT_VARIABLE Guile_EXTENSION_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif ()

mark_as_advanced (Guile_EXECUTABLE
  Guile_INCLUDE_DIR
  Guile_LIBRARY
  Guile_LIBRARY_RELEASE
  Guile_GC_INCLUDE_DIR
  Guile_GC_LIBRARY
  Guile_GC_LIBRARY_RELEASE)
