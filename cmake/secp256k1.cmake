# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

function(add_secp256k1 subdir)
  message("")
  message("Configuring secp256k1 subtree...")
  set(BUILD_SHARED_LIBS OFF)
  set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)
  set(SECP256K1_ENABLE_MODULE_ECDH OFF CACHE BOOL "" FORCE)
  set(SECP256K1_ENABLE_MODULE_RECOVERY ON CACHE BOOL "" FORCE)
  set(SECP256K1_ENABLE_MODULE_MUSIG ON CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_TESTS ${BUILD_TESTS} CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_EXHAUSTIVE_TESTS ${BUILD_TESTS} CACHE BOOL "" FORCE)
  if(NOT BUILD_TESTS)
    # Always skip the ctime tests, if we are building no other tests.
    # Otherwise, they are built if Valgrind is available. See SECP256K1_VALGRIND.
    set(SECP256K1_BUILD_CTIME_TESTS ${BUILD_TESTS} CACHE BOOL "" FORCE)
  endif()
  set(SECP256K1_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  include(GetTargetInterface)
  # -fsanitize and related flags apply to both C++ and C,
  # so we can pass them down to libsecp256k1 as CFLAGS and LDFLAGS.
  get_target_interface(SECP256K1_APPEND_CFLAGS "" sanitize_interface COMPILE_OPTIONS)
  string(STRIP "${SECP256K1_APPEND_CFLAGS} ${APPEND_CPPFLAGS}" SECP256K1_APPEND_CFLAGS)
  string(STRIP "${SECP256K1_APPEND_CFLAGS} ${APPEND_CFLAGS}" SECP256K1_APPEND_CFLAGS)
  set(SECP256K1_APPEND_CFLAGS ${SECP256K1_APPEND_CFLAGS} CACHE STRING "" FORCE)
  get_target_interface(SECP256K1_APPEND_LDFLAGS "" sanitize_interface LINK_OPTIONS)
  string(STRIP "${SECP256K1_APPEND_LDFLAGS} ${APPEND_LDFLAGS}" SECP256K1_APPEND_LDFLAGS)
  set(SECP256K1_APPEND_LDFLAGS ${SECP256K1_APPEND_LDFLAGS} CACHE STRING "" FORCE)
  # We want to build libsecp256k1 with the most tested RelWithDebInfo configuration.
  enable_language(C)
  foreach(config IN LISTS CMAKE_BUILD_TYPE CMAKE_CONFIGURATION_TYPES)
    if(config STREQUAL "")
      continue()
    endif()
    string(TOUPPER "${config}" config)
    set(CMAKE_C_FLAGS_${config} "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  endforeach()
  # If the CFLAGS environment variable is defined during building depends
  # and configuring this build system, its content might be duplicated.
  if(DEFINED ENV{CFLAGS})
    deduplicate_flags(CMAKE_C_FLAGS)
  endif()

  add_subdirectory(${subdir})
  set_target_properties(secp256k1 PROPERTIES
    EXCLUDE_FROM_ALL TRUE
  )
endfunction()
