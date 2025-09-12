# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include(GNUInstallDirs)

# TODO: libbitcoinkernel is a work in progress consensus engine
#       library, as more and more modules are decoupled from the
#       consensus engine, this list will shrink to only those
#       which are absolutely necessary.
add_library(bitcoinkernel
  bitcoinkernel.cpp
  $<TARGET_OBJECTS:bitcoin_clientversion>
  $<TARGET_OBJECTS:bitcoin_crypto>
  $<TARGET_OBJECTS:leveldb>
  $<TARGET_OBJECTS:crc32c>
)

get_target_property(kernel_sources bitcoin_kernel SOURCES)
foreach(src ${kernel_sources})
  target_sources(bitcoinkernel PRIVATE "${CMAKE_SOURCE_DIR}/src/${src}")
endforeach()

get_target_property(util_sources bitcoin_kernel_util SOURCES)
foreach(src ${util_sources})
  target_sources(bitcoinkernel PRIVATE "${CMAKE_SOURCE_DIR}/src/util/${src}")
endforeach()

get_target_property(consensus_sources bitcoin_consensus SOURCES)
foreach(src ${consensus_sources})
  target_sources(bitcoinkernel PRIVATE "${CMAKE_SOURCE_DIR}/src/${src}")
endforeach()

target_link_libraries(bitcoinkernel
  PRIVATE
    core_interface
    secp256k1_objs
    $<$<PLATFORM_ID:Windows>:bcrypt>
    $<TARGET_NAME_IF_EXISTS:USDT::headers>
  PUBLIC
    Boost::headers
)

target_include_directories(bitcoinkernel PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/leveldb/include>)

# libbitcoinkernel requires default symbol visibility, explicitly
# specify that here so that things still work even when user
# configures with -DREDUCE_EXPORTS=ON
#
# Note this is a quick hack that will be removed as we
# incrementally define what to export from the library.
set_target_properties(bitcoinkernel PROPERTIES
  CXX_VISIBILITY_PRESET default
)

# Add a convenience libbitcoinkernel target as a synonym for bitcoinkernel.
add_custom_target(libbitcoinkernel)
add_dependencies(libbitcoinkernel bitcoinkernel)

configure_file(${PROJECT_SOURCE_DIR}/libbitcoinkernel.pc.in ${PROJECT_BINARY_DIR}/libbitcoinkernel.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/libbitcoinkernel.pc DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig" COMPONENT libbitcoinkernel)

install(TARGETS bitcoinkernel
  RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT libbitcoinkernel
  LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT libbitcoinkernel
  ARCHIVE
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT libbitcoinkernel
)
