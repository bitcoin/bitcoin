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
  target_compile_definitions(${target} INTERFACE
    $<$<BOOL:${HAVE_EVHTTP_CONNECTION_GET_PEER_CONST_CHAR}>:HAVE_EVHTTP_CONNECTION_GET_PEER_CONST_CHAR>
  )
endfunction()

set(_libevent_components core extra)
if(NOT WIN32)
  list(APPEND _libevent_components pthreads)
endif()

find_package(Libevent ${Libevent_FIND_VERSION} QUIET
  NO_MODULE
)

include(FindPackageHandleStandardArgs)
if(Libevent_FOUND)
  find_package(Libevent ${Libevent_FIND_VERSION} QUIET
    REQUIRED COMPONENTS ${_libevent_components}
    NO_MODULE
  )
  find_package_handle_standard_args(Libevent
    REQUIRED_VARS Libevent_DIR
    VERSION_VAR Libevent_VERSION
  )
  check_evhttp_connection_get_peer(libevent::extra)
else()
  find_package(PkgConfig REQUIRED)
  foreach(component IN LISTS _libevent_components)
    pkg_check_modules(libevent_${component}
      REQUIRED QUIET
      IMPORTED_TARGET GLOBAL
      libevent_${component}>=${Libevent_FIND_VERSION}
    )
    if(TARGET PkgConfig::libevent_${component} AND NOT TARGET libevent::${component})
      add_library(libevent::${component} ALIAS PkgConfig::libevent_${component})
    endif()
  endforeach()
  find_package_handle_standard_args(Libevent
    REQUIRED_VARS libevent_core_LIBRARY_DIRS
    VERSION_VAR libevent_core_VERSION
  )
  check_evhttp_connection_get_peer(PkgConfig::libevent_extra)
endif()

unset(_libevent_components)

mark_as_advanced(Libevent_DIR)
mark_as_advanced(_event_h)
mark_as_advanced(_event_lib)
