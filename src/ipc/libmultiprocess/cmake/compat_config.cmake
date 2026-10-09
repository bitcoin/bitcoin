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
