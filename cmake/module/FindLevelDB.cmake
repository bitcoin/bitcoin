# Copyright (c) 2025-present The Bitcoin Knots developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include(CheckSourceCompiles)
include(CMakePushCheckState)

message(CHECK_START "Looking for system LevelDB")

find_path(LevelDB_INCLUDE_DIR
  NAMES leveldb/filter_policy.h
  DOC "Path to LevelDB include directory (ie, the directory ABOVE leveldb/db.h)"
)
mark_as_advanced(LevelDB_INCLUDE_DIR)

find_path(LevelDB_MemEnv_INCLUDE_DIR
  NAMES memenv.h
  HINTS "${LevelDB_INCLUDE_DIR}/leveldb/helpers"
  DOC "Path to include directory for MemEnv LevelDB helper (ie, the directory with memenv.h)"
)
mark_as_advanced(LevelDB_MemEnv_INCLUDE_DIR)

function(_try_leveldb_compile libs cachevar)
  cmake_push_check_state(RESET)
  list(APPEND CMAKE_REQUIRED_INCLUDES "${LevelDB_INCLUDE_DIR}")
  list(APPEND CMAKE_REQUIRED_INCLUDES "${LevelDB_MemEnv_INCLUDE_DIR}")
  list(APPEND CMAKE_REQUIRED_LIBRARIES "${libs}")
  set(CMAKE_REQUIRED_QUIET 1)
  check_source_compiles(
    CXX
    [[
      #include <leveldb/env.h>
      #include <memenv.h>

      int main() {
        leveldb::Env *myenv = leveldb::NewMemEnv(leveldb::Env::Default());
        delete myenv;
      }
    ]]
    "${cachevar}"
  )
  cmake_pop_check_state()
endfunction()

function(find_leveldb_libs release_type suffixes _LevelDB_lib_hint)
  find_library("LevelDB_LIBRARY_${release_type}"
    NAMES leveldb
    HINTS "${_LevelDB_lib_hint}"
    PATH_SUFFIXES "${suffixes}"
  )
  mark_as_advanced("LevelDB_LIBRARY_${release_type}")

  _try_leveldb_compile("${LevelDB_LIBRARY_${release_type}}" "LevelDB_memenv_works_without_helper_lib_${release_type}")
  if(LevelDB_memenv_works_without_helper_lib_${release_type})
    set("LevelDB_MemEnv_LIBRARY_${release_type}" "" CACHE STRING "")
    set(LevelDB_MemEnv_FOUND ON CACHE INTERNAL "")
  else()
    find_library("LevelDB_MemEnv_LIBRARY_${release_type}"
      NAMES memenv
      HINTS "${_LevelDB_lib_hint}"
      PATH_SUFFIXES "${suffixes}"
    )

    set(libs ${LevelDB_LIBRARY_${release_type}} ${LevelDB_MemEnv_LIBRARY_${release_type}})
    _try_leveldb_compile("${libs}" "LevelDB_memenv_works_with_helper_lib_${release_type}")
    if(NOT LevelDB_memenv_works_with_helper_lib_${release_type})
      set(LevelDB_MemEnv_FOUND OFF CACHE INTERNAL "")
    endif()
  endif()
  mark_as_advanced("LevelDB_MemEnv_LIBRARY_${release_type}")
endfunction()

get_filename_component(_LevelDB_lib_hint "${LevelDB_INCLUDE_DIR}" DIRECTORY)

find_leveldb_libs(RELEASE lib "${_LevelDB_lib_hint}")
find_leveldb_libs(DEBUG debug/lib "${_LevelDB_lib_hint}")

unset(_LevelDB_lib_hint)

include(SelectLibraryConfigurations)
select_library_configurations(LevelDB)
select_library_configurations(LevelDB_MemEnv)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LevelDB
  REQUIRED_VARS LevelDB_LIBRARY LevelDB_INCLUDE_DIR LevelDB_MemEnv_INCLUDE_DIR LevelDB_MemEnv_FOUND
)

if(LevelDB_FOUND)
  message(CHECK_PASS "found")
  if(NOT TARGET leveldb)
    add_library(leveldb UNKNOWN IMPORTED)
    set_target_properties(leveldb PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${LevelDB_INCLUDE_DIR};${LevelDB_MemEnv_INCLUDE_DIR}"
    )
    if(LevelDB_LIBRARY_RELEASE)
      set_property(TARGET leveldb APPEND PROPERTY
        IMPORTED_CONFIGURATIONS RELEASE
      )
      set_target_properties(leveldb PROPERTIES
        IMPORTED_LOCATION_RELEASE "${LevelDB_LIBRARY_RELEASE}"
        IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "${LevelDB_MemEnv_LIBRARY_RELEASE}"
      )
    endif()
    if(LevelDB_LIBRARY_DEBUG)
      set_property(TARGET leveldb APPEND PROPERTY
        IMPORTED_CONFIGURATIONS DEBUG
      )
      set_target_properties(leveldb PROPERTIES
        IMPORTED_LOCATION_DEBUG "${LevelDB_LIBRARY_DEBUG}"
        IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "${LevelDB_MemEnv_LIBRARY_DEBUG}"
      )
    endif()
  endif()
else()
  message(CHECK_FAIL "not found")
endif()
