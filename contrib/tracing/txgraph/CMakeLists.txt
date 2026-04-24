# Standalone build for txgraph-replay tool.
#
# This CMakeLists.txt is NOT integrated into Bitcoin Core's build system.
# It builds txgraph-replay by linking against pre-built Bitcoin Core libraries.
#
# Usage:
#   cmake -B build \
#       -DBITCOIN_SOURCE_DIR=/path/to/bitcoin \
#       -DBITCOIN_BUILD_DIR=/path/to/bitcoin/build
#   cmake --build build
#
# Or use the convenience script: ./build_replay.sh [build_dir]

cmake_minimum_required(VERSION 3.22)
project(txgraph-replay LANGUAGES CXX)

set(BITCOIN_SOURCE_DIR "" CACHE PATH "Path to Bitcoin Core source tree root")
set(BITCOIN_BUILD_DIR "" CACHE PATH "Path to Bitcoin Core build directory")

if(NOT BITCOIN_SOURCE_DIR)
  message(FATAL_ERROR "BITCOIN_SOURCE_DIR is required. Set -DBITCOIN_SOURCE_DIR=/path/to/bitcoin")
endif()
if(NOT BITCOIN_BUILD_DIR)
  message(FATAL_ERROR "BITCOIN_BUILD_DIR is required. Set -DBITCOIN_BUILD_DIR=/path/to/bitcoin/build")
endif()

# Verify paths
if(NOT EXISTS "${BITCOIN_SOURCE_DIR}/src/txgraph.h")
  message(FATAL_ERROR "Cannot find src/txgraph.h in BITCOIN_SOURCE_DIR=${BITCOIN_SOURCE_DIR}")
endif()
if(NOT EXISTS "${BITCOIN_BUILD_DIR}/lib/libbitcoin_node.a")
  message(FATAL_ERROR "Cannot find lib/libbitcoin_node.a in BITCOIN_BUILD_DIR=${BITCOIN_BUILD_DIR}. Build Bitcoin Core first.")
endif()

add_executable(txgraph-replay txgraph_replay.cpp)
set_target_properties(txgraph-replay PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)

target_include_directories(txgraph-replay PRIVATE
  ${BITCOIN_BUILD_DIR}/src
  ${BITCOIN_SOURCE_DIR}/src
)

# Link pre-built Bitcoin Core static libraries (order matters for static linking)
target_link_libraries(txgraph-replay PRIVATE
  ${BITCOIN_BUILD_DIR}/lib/libbitcoin_node.a            # TxGraph implementation (txgraph.o)
  ${BITCOIN_BUILD_DIR}/lib/libbitcoin_util.a            # FastRandomContext (used by linearization), logging, Assert
  ${BITCOIN_BUILD_DIR}/lib/libbitcoin_crypto.a          # ChaCha20 CSPRNG and CSHA512 (used by FastRandomContext)
  ${BITCOIN_BUILD_DIR}/lib/libbitcoin_clientversion.a   # FormatFullVersion() (referenced by Assert error messages)
)

# Output binary alongside Bitcoin Core binaries
set_target_properties(txgraph-replay PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${BITCOIN_BUILD_DIR}/bin
)
