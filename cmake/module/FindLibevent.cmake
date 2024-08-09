# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

#[=======================================================================[
FindLibevent
------------

Finds the Libevent headers and libraries.

This is a wrapper around find_package()/pkg_check_modules() commands that:
 - facilitates searching in various build environments
 - prints a standard log message

#]=======================================================================]

# Check whether evhttp_connection_get_peer expects const char**.
# See https://github.com/libevent/libevent/commit/a18301a2bb160ff7c3ffaf5b7653c39ffe27b385
function(check_evhttp_connection_get_peer target)
  include(CMakePushCheckState)
  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_LIBRARIES ${target})
  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("
    #include <cstdint>
    #include <event2/http.h>

    int main()
    {
        evhttp_connection* conn = (evhttp_connection*)1;
        const char* host;
        uint16_t port;
        evhttp_connection_get_peer(conn, &host, &port);
    }
    " HAVE_EVHTTP_CONNECTION_GET_PEER_CONST_CHAR
  )
  cmake_pop_check_state()
  set(HAVE_EVHTTP_CONNECTION_GET_PEER_CONST_CHAR ${HAVE_EVHTTP_CONNECTION_GET_PEER_CONST_CHAR} PARENT_SCOPE)
endfunction()


include(FindPackageHandleStandardArgs)
if(VCPKG_TARGET_TRIPLET)
  find_package(Libevent ${Libevent_FIND_VERSION} NO_MODULE QUIET
    COMPONENTS extra
  )
  find_package_handle_standard_args(Libevent
    REQUIRED_VARS Libevent_DIR
    VERSION_VAR Libevent_VERSION
  )
  check_evhttp_connection_get_peer(libevent::extra)
  add_library(libevent::libevent ALIAS libevent::extra)
  mark_as_advanced(Libevent_DIR)
  mark_as_advanced(_event_h)
  mark_as_advanced(_event_lib)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(libevent QUIET
    IMPORTED_TARGET
    libevent>=${Libevent_FIND_VERSION}
  )
  set(_libevent_required_vars libevent_LIBRARY_DIRS libevent_FOUND)
  if(NOT WIN32)
    pkg_check_modules(libevent_pthreads QUIET
      IMPORTED_TARGET
      libevent_pthreads>=${Libevent_FIND_VERSION}
    )
    list(APPEND _libevent_required_vars libevent_pthreads_FOUND)
  endif()
  find_package_handle_standard_args(Libevent
    REQUIRED_VARS ${_libevent_required_vars}
    VERSION_VAR libevent_VERSION
  )
  unset(_libevent_required_vars)
  check_evhttp_connection_get_peer(PkgConfig::libevent)
  add_library(libevent::libevent ALIAS PkgConfig::libevent)
  if(NOT WIN32)
    add_library(libevent::pthreads ALIAS PkgConfig::libevent_pthreads)
  endif()
endif()
