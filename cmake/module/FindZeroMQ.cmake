# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

#[=======================================================================[
FindZeroMQ
----------

Finds the ZeroMQ headers and library.

This is a wrapper around find_package()/pkg_check_modules() commands that:
 - facilitates searching in various build environments
 - prints a standard log message

#]=======================================================================]

include(FindPackageHandleStandardArgs)
find_package(ZeroMQ ${ZeroMQ_FIND_VERSION} NO_MODULE QUIET)
if(ZeroMQ_FOUND)
  find_package_handle_standard_args(ZeroMQ
    REQUIRED_VARS ZeroMQ_DIR
    VERSION_VAR ZeroMQ_VERSION
  )
  if(TARGET libzmq)
    add_library(zeromq ALIAS libzmq)
  elseif(TARGET libzmq-static)
    add_library(zeromq ALIAS libzmq-static)
  endif()
  mark_as_advanced(ZeroMQ_DIR)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(libzmq QUIET
    IMPORTED_TARGET
    libzmq>=${ZeroMQ_FIND_VERSION}
  )
  find_package_handle_standard_args(ZeroMQ
    REQUIRED_VARS libzmq_LIBRARY_DIRS
    VERSION_VAR libzmq_VERSION
  )
  add_library(zeromq ALIAS PkgConfig::libzmq)
endif()
