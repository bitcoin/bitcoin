# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

include(CMakePushCheckState)
include(CheckCXXSourceCompiles)

function(check_kj_exception_support)
  set(kj_libraries)
  set(kj_includes)
  set(_kj_inc)
  if(CAPNP_INCLUDE_DIRECTORY)
    list(GET CAPNP_INCLUDE_DIRECTORY 0 _kj_inc)
  endif()
  if(TARGET CapnProto::kj)
    set(kj_libraries CapnProto::kj)
  elseif(CapnProto_kj_IMPORTED_LOCATION AND _kj_inc)
    set(kj_libraries "${CapnProto_kj_IMPORTED_LOCATION}")
    set(kj_includes "${_kj_inc}")
  elseif(_kj_inc)
    get_filename_component(_kj_prefix "${_kj_inc}" DIRECTORY)
    # Prefix-only so depends headers cannot pair with a system libkj. NO_CACHE
    # so a later reconfigure cannot keep a stale path from another prefix.
    find_library(KJ_LIBRARY
      NAMES kj
      HINTS "${_kj_prefix}"
      PATH_SUFFIXES lib lib64 lib32 lib/${CMAKE_LIBRARY_ARCHITECTURE}
      NO_CACHE
      NO_DEFAULT_PATH
    )
    if(KJ_LIBRARY)
      set(kj_libraries "${KJ_LIBRARY}")
      set(kj_includes "${_kj_inc}")
    endif()
  endif()

  if(NOT kj_libraries)
    if(_kj_inc)
      message(FATAL_ERROR
        "Could not find libkj next to the Cap'n Proto headers used by IPC (${_kj_inc}).\n"
        "To resolve, choose one of the following:\n"
        "  - Rebuild Cap'n Proto with a matching library and header prefix\n"
        "  - Build Bitcoin Core using the depends capnproto package\n"
        "  - Build with -DENABLE_IPC=OFF to disable multiprocess support\n"
      )
    else()
      message(FATAL_ERROR
        "Could not locate the KJ library used by IPC.\n"
        "To resolve, choose one of the following:\n"
        "  - Install Cap'n Proto (0.7 or newer)\n"
        "  - Build with -DENABLE_IPC=OFF to disable multiprocess support\n"
      )
    endif()
  endif()

  cmake_push_check_state(RESET)

  check_cxx_source_compiles("
    int main()
    {
      try { throw 1; } catch (...) { return 0; }
      return 1;
    }
  " HAVE_KJ_CXX_EXCEPTIONS)

  if(NOT HAVE_KJ_CXX_EXCEPTIONS)
    message(FATAL_ERROR
      "C++ exception support is required when ENABLE_IPC is on.\n"
      "To resolve, choose one of the following:\n"
      "  - Remove -fno-exceptions (or equivalent) from the compiler flags\n"
      "  - Build with -DENABLE_IPC=OFF to disable multiprocess support\n"
      "If you changed compiler flags, delete CMakeCache.txt and re-run cmake.\n"
    )
  endif()

  set(CMAKE_REQUIRED_LIBRARIES ${kj_libraries})
  if(kj_includes)
    set(CMAKE_REQUIRED_INCLUDES ${kj_includes})
  endif()

  check_cxx_source_compiles("
    #include <kj/exception.h>
    int main()
    {
      (void)kj::getExceptionCallback();
      return 0;
    }
  " HAVE_KJ_LIB)

  if(NOT HAVE_KJ_LIB)
    message(FATAL_ERROR
      "The installed libkj could not be linked.\n"
      "To resolve, choose one of the following:\n"
      "  - Install Cap'n Proto (0.7 or newer)\n"
      "  - Build with -DENABLE_IPC=OFF to disable multiprocess support\n"
    )
  endif()

  # runCatchingExceptions calls this when exceptions are on; do not trust kj/common.h autodetection.
  set(CMAKE_REQUIRED_DEFINITIONS -DKJ_NO_EXCEPTIONS=0)
  check_cxx_source_compiles("
    #include <kj/exception.h>
    int main()
    {
      try {
        throw 1;
      } catch (...) {
        (void)kj::getCaughtExceptionAsKj();
      }
      return 0;
    }
  " HAVE_KJ_EXCEPTION_SYMBOLS)

  cmake_pop_check_state()

  if(NOT HAVE_KJ_EXCEPTION_SYMBOLS)
    message(FATAL_ERROR
      "The installed libkj was built without exception support (KJ_NO_EXCEPTIONS).\n"
      "Passing -DKJ_NO_EXCEPTIONS=0 when compiling Bitcoin Core does not fix this;\n"
      "libkj itself must be rebuilt.\n"
      "To resolve, choose one of the following:\n"
      "  - Rebuild Cap'n Proto with -DKJ_NO_EXCEPTIONS=0\n"
      "  - Build Bitcoin Core using the depends capnproto package\n"
      "  - Build with -DENABLE_IPC=OFF to disable multiprocess support\n"
      "If you rebuilt libkj, delete CMakeCache.txt and re-run cmake.\n"
    )
  endif()

  mark_as_advanced(HAVE_KJ_CXX_EXCEPTIONS HAVE_KJ_LIB HAVE_KJ_EXCEPTION_SYMBOLS)
  message(STATUS "KJ exception support: yes (compiler, libkj, exception symbols)")
endfunction()
