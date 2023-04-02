# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Must be included before CMAKE_INSTALL_INCLUDEDIR is used.
include(GNUInstallDirs)

set(bitcoinconsensus_src
  $<TARGET_OBJECTS:bitcoin_crypto>
  interpreter.cpp
  script.cpp
  script_error.cpp
  ../arith_uint256.cpp
  ../consensus/merkle.cpp
  ../consensus/tx_check.cpp
  ../hash.cpp
  ../primitives/block.cpp
  ../primitives/transaction.cpp
  ../pubkey.cpp
  ../uint256.cpp
  ../util/strencodings.cpp
)
set_property(SOURCE interpreter.cpp APPEND PROPERTY
  COMPILE_OPTIONS $<$<BOOL:${MINGW}>:-Wno-maybe-uninitialized>
)

if(BUILD_SHARED)
  add_library(bitcoinconsensus SHARED "")
  target_sources(bitcoinconsensus
    PRIVATE
      bitcoinconsensus.cpp
      ../support/cleanse.cpp
      ${bitcoinconsensus_src}
  )
  target_compile_definitions(bitcoinconsensus
    PRIVATE
      BUILD_BITCOIN_INTERNAL
  )
  target_include_directories(bitcoinconsensus
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  target_link_libraries(bitcoinconsensus
    PRIVATE
      secp256k1
  )
  set_target_properties(bitcoinconsensus PROPERTIES
    SOVERSION 0
    VERSION 0.0.0
  )
  install(TARGETS bitcoinconsensus
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endif()

if(BUILD_STATIC)
  add_library(bitcoinconsensus_static STATIC "")
  target_sources(bitcoinconsensus_static
    PRIVATE
      bitcoinconsensus.cpp
      ../support/cleanse.cpp
      ${bitcoinconsensus_src}
  )
  target_compile_definitions(bitcoinconsensus_static
    PRIVATE
      BUILD_BITCOIN_INTERNAL
  )
  target_include_directories(bitcoinconsensus_static
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  target_link_libraries(bitcoinconsensus_static
    PRIVATE
      secp256k1
  )
  if(NOT MSVC)
    set_target_properties(bitcoinconsensus_static PROPERTIES
      OUTPUT_NAME bitcoinconsensus
    )
  endif()
  install(TARGETS bitcoinconsensus_static
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endif()

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/bitcoinconsensus.h"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix "\${prefix}")
set(libdir "\${exec_prefix}/lib")
set(includedir "\${prefix}/include")
configure_file(${CMAKE_SOURCE_DIR}/libbitcoinconsensus.pc.in libbitcoinconsensus.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libbitcoinconsensus.pc DESTINATION lib/pkgconfig)
