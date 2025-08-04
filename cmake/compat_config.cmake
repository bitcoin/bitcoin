# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# compat_config.cmake -- compatibility workarounds meant to be included after
# cmake find_package() calls are made, before configuring the ebuild

# Define capnp_PREFIX if not defined to avoid issue on macos
# https://github.com/bitcoin-core/libmultiprocess/issues/26

if (NOT DEFINED capnp_PREFIX AND DEFINED CAPNP_INCLUDE_DIRS)
  get_filename_component(capnp_PREFIX "${CAPNP_INCLUDE_DIRS}" DIRECTORY)
endif()

if (NOT DEFINED CAPNPC_OUTPUT_DIR)
  set(CAPNPC_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
endif()

# CMake target definitions for backwards compatibility with Ubuntu bionic
# capnproto 0.6.1 package (https://packages.ubuntu.com/bionic/libcapnp-dev)
# https://github.com/bitcoin-core/libmultiprocess/issues/27

if (NOT DEFINED CAPNP_LIB_CAPNPC AND DEFINED CAPNP_LIB_CAPNP-RPC)
  string(REPLACE "-rpc" "c" CAPNP_LIB_CAPNPC "${CAPNP_LIB_CAPNP-RPC}")
endif()

if (NOT DEFINED CapnProto_capnpc_IMPORTED_LOCATION AND DEFINED CapnProto_capnp-rpc_IMPORTED_LOCATION)
  string(REPLACE "-rpc" "c" CapnProto_capnpc_IMPORTED_LOCATION "${CapnProto_capnp-rpc_IMPORTED_LOCATION}")
endif()

if (NOT TARGET CapnProto::capnp AND DEFINED CAPNP_LIB_CAPNP)
  add_library(CapnProto::capnp SHARED IMPORTED)
  set_target_properties(CapnProto::capnp PROPERTIES IMPORTED_LOCATION "${CAPNP_LIB_CAPNP}")
endif()

if (NOT TARGET CapnProto::capnpc AND DEFINED CAPNP_LIB_CAPNPC)
  add_library(CapnProto::capnpc SHARED IMPORTED)
  set_target_properties(CapnProto::capnpc PROPERTIES IMPORTED_LOCATION "${CAPNP_LIB_CAPNPC}")
endif()

if (NOT TARGET CapnProto::capnpc AND DEFINED CapnProto_capnpc_IMPORTED_LOCATION)
  add_library(CapnProto::capnpc SHARED IMPORTED)
  set_target_properties(CapnProto::capnpc PROPERTIES IMPORTED_LOCATION "${CapnProto_capnpc_IMPORTED_LOCATION}")
endif()

if (NOT TARGET CapnProto::capnp-rpc AND DEFINED CAPNP_LIB_CAPNP-RPC)
  add_library(CapnProto::capnp-rpc SHARED IMPORTED)
  set_target_properties(CapnProto::capnp-rpc PROPERTIES IMPORTED_LOCATION "${CAPNP_LIB_CAPNP-RPC}")
endif()

if (NOT TARGET CapnProto::kj AND DEFINED CAPNP_LIB_KJ)
  add_library(CapnProto::kj SHARED IMPORTED)
  set_target_properties(CapnProto::kj PROPERTIES IMPORTED_LOCATION "${CAPNP_LIB_KJ}")
endif()

if (NOT TARGET CapnProto::kj-async AND DEFINED CAPNP_LIB_KJ-ASYNC)
  add_library(CapnProto::kj-async SHARED IMPORTED)
  set_target_properties(CapnProto::kj-async PROPERTIES IMPORTED_LOCATION "${CAPNP_LIB_KJ-ASYNC}")
endif()
