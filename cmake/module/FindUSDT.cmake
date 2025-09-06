# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

#[=======================================================================[
FindUSDT
--------

Finds the Userspace, Statically Defined Tracing header(s).

Imported Targets
^^^^^^^^^^^^^^^^

This module provides imported target ``USDT::headers``, if
USDT has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``USDT_FOUND``
  "True" if USDT found.

#]=======================================================================]

find_path(USDT_INCLUDE_DIR
  NAMES sys/sdt.h
)
mark_as_advanced(USDT_INCLUDE_DIR)

if(USDT_INCLUDE_DIR)
  include(CMakePushCheckState)
  cmake_push_check_state(RESET)

  include(CheckCXXSourceCompiles)
  set(CMAKE_REQUIRED_INCLUDES ${USDT_INCLUDE_DIR})
  check_cxx_source_compiles("
    #if defined(__arm__)
    #  define STAP_SDT_ARG_CONSTRAINT g
    #endif

    // Setting SDT_USE_VARIADIC lets systemtap (sys/sdt.h) know that we want to use
    // the optional variadic macros to define tracepoints.
    #define SDT_USE_VARIADIC 1
    #include <sys/sdt.h>

    int main()
    {
      STAP_PROBEV(context, event);
      int a, b, c, d, e, f, g;
      STAP_PROBEV(context, event, a, b, c, d, e, f, g);
    }
    " HAVE_USDT_H
  )

  cmake_pop_check_state()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(USDT
  REQUIRED_VARS USDT_INCLUDE_DIR HAVE_USDT_H
)

if(USDT_FOUND AND NOT TARGET USDT::headers)
  add_library(USDT::headers INTERFACE IMPORTED)
  set_target_properties(USDT::headers PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${USDT_INCLUDE_DIR}"
  )
  set(ENABLE_TRACING TRUE)
endif()
