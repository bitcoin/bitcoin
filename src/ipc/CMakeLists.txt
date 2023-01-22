# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

function(generate_from_capnp capnp_relpath)
  add_custom_command(
    OUTPUT ${capnp_relpath}.c++ ${capnp_relpath}.h ${capnp_relpath}.proxy-client.c++ ${capnp_relpath}.proxy-types.h ${capnp_relpath}.proxy-server.c++ ${capnp_relpath}.proxy-types.c++ ${capnp_relpath}.proxy.h
    COMMAND ${MPGEN_PREFIX}/bin/mpgen ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${capnp_relpath}
    DEPENDS ${capnp_relpath}
    VERBATIM
  )
endfunction()

generate_from_capnp(capnp/echo.capnp)
generate_from_capnp(capnp/init.capnp)

add_library(bitcoin_ipc STATIC EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_BINARY_DIR}/capnp/echo.capnp.c++
  ${CMAKE_CURRENT_BINARY_DIR}/capnp/echo.capnp.proxy-client.c++
  ${CMAKE_CURRENT_BINARY_DIR}/capnp/echo.capnp.proxy-server.c++
  ${CMAKE_CURRENT_BINARY_DIR}/capnp/echo.capnp.proxy-types.c++
  ${CMAKE_CURRENT_BINARY_DIR}/capnp/init.capnp.c++
  ${CMAKE_CURRENT_BINARY_DIR}/capnp/init.capnp.proxy-client.c++
  ${CMAKE_CURRENT_BINARY_DIR}/capnp/init.capnp.proxy-server.c++
  ${CMAKE_CURRENT_BINARY_DIR}/capnp/init.capnp.proxy-types.c++
  capnp/protocol.cpp
  interfaces.cpp
  process.cpp
)

target_include_directories(bitcoin_ipc
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
)

target_link_libraries(bitcoin_ipc
  PRIVATE
    PkgConfig::libmultiprocess
)
