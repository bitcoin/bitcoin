# Copyright 2010, Dimitri Kaparis <kaparis.dimitri at gmail dot com>

# CMake module to find Berkeley DB

# For MSVC, only static libraries are used (with 's' suffix)

# This module uses:
#
# BerkeleyDB_ROOT_DIR - set to BerkeleyDB's root directory

# This module defines:
# BerkeleyDB_FOUND - True if BerkleyDB is found
# BerkeleyDB_INCLUDE_DIR - BerkeleyDB's include directory
# BerkeleyDB_LIBRARIES - Libraries needed to use Berkeley DB

if (BerkeleyDB_FIND_VERSION_MAJOR AND BerkeleyDB_FIND_VERSION_MINOR)
  if (MSVC)
    set(NAME_EXTENSION ${BerkeleyDB_FIND_VERSION_MAJOR}${BerkeleyDB_FIND_VERSION_MINOR})
  else (MSVC)
    set(NAME_EXTENSION -${BerkeleyDB_FIND_VERSION_MAJOR}.${BerkeleyDB_FIND_VERSION_MINOR})
  endif(MSVC)
    
elseif (BerkeleyDB_FIND_VERSION_MAJOR)
  if (MSVC)
    set(NAME_EXTENSION ${BerkeleyDB_FIND_VERSION_MAJOR})
  else (MSVC)
    set(NAME_EXTENSION -${BerkeleyDB_FIND_VERSION_MAJOR})
  endif(MSVC)
endif(BerkeleyDB_FIND_VERSION_MAJOR AND BerkeleyDB_FIND_VERSION_MINOR)

#library names
SET(BerkeleyDB_NAMES ${BerkeleyDB_NAMES} db db_cxx)

#If a specific version is required append the extension.  
if (NAME_EXTENSION)
  foreach (NAME ${BerkeleyDB_NAMES})
    set(NEW_NAMES ${NEW_NAMES} "${NAME}${NAME_EXTENSION}")
  endforeach (NAME)
  set (BerkeleyDB_NAMES ${NEW_NAMES})
endif (NAME_EXTENSION)

find_path(BerkeleyDB_INCLUDE_DIR NAMES db_cxx.h
          PATHS ${DB_ROOT_DIR} $ENV{DBROOTDIR}
          PATH_SUFFIXES include db${NAME_EXTENSION}
          )

if (MSVC)
    if (CMAKE_CL_64)
        set(_db_lib_path_SUFFIXES_DEBUG Debug_AMD64)
        set(_db_lib_path_SUFFIXES_RELEASE Release_AMD64)
    else (CMAKE_CL_64)
        set(_db_lib_path_SUFFIXES_DEBUG Debug)
        set(_db_lib_path_SUFFIXES_RELEASE Release)
    endif (CMAKE_CL_64)
    find_library(DBLIB_STATIC_RELEASE libdb${NAME_EXTENSION}s
                 PATHS ${DB_ROOT_DIR} $ENV{DBROOTDIR} ${BerkeleyDB_INCLUDE_DIR}
                 PATH_SUFFIXES ${_db_lib_path_SUFFIXES_RELEASE} lib)
    find_library(DBLIB_STATIC_DEBUG libdb${NAME_EXTENSION}sd
                 PATHS ${DB_ROOT_DIR} $ENV{DBROOTDIR} ${BerkeleyDB_INCLUDE_DIR}
                 PATH_SUFFIXES ${_db_lib_path_SUFFIXES_DEBUG} lib)
    set(BerkeleyDB_LIBRARIES optimized ${DBLIB_STATIC_RELEASE}
                     debug ${DBLIB_STATIC_DEBUG})

else(MSVC)
    find_library(BerkeleyDB_LIB db${NAME_EXTENSION}
                 PATHS ${DB_ROOT_DIR} $ENV{DBROOTDIR} ${BerkeleyDB_INCLUDE_DIR}
                       /usr/local/lib
                 PATH_SUFFIXES lib
                               db${NAME_EXTENSION})
    find_library(BerkeleyDB_LIBCXX db_cxx${NAME_EXTENSION}
                 PATHS ${DB_ROOT_DIR} $ENV{DBROOTDIR} ${BerkeleyDB_INCLUDE_DIR}
                       /usr/local/lib
                 PATH_SUFFIXES lib db${NAME_EXTENSION})
    set(BerkeleyDB_LIBRARIES ${BerkeleyDB_LIB})
    if (BerkeleyDB_LIBCXX)
        list(APPEND BerkeleyDB_LIBRARIES ${BerkeleyDB_LIBCXX})
    endif (BerkeleyDB_LIBCXX)
endif (MSVC)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BerkeleyDB DEFAULT_MSG BerkeleyDB_INCLUDE_DIR BerkeleyDB_LIBRARIES)

MARK_AS_ADVANCED(
  BerkeleyDB_LIBRARIES
  BerkeleyDB_INCLUDE_DIR
  )
