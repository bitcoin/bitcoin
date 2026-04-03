#!/usr/bin/env bash

export LC_ALL=C

# Build txgraph-replay tool against a pre-built Bitcoin Core tree.
#
# Usage:
#   ./build_replay.sh [bitcoin_build_dir]
#
# If bitcoin_build_dir is not specified, defaults to <source_root>/build.
# This script works regardless of the current working directory.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
BUILD_DIR="${1:-${SOURCE_DIR}/build}"

if [ ! -f "${SOURCE_DIR}/src/txgraph.h" ]; then
    echo "Error: Cannot find Bitcoin Core source at ${SOURCE_DIR}" >&2
    exit 1
fi

if [ ! -f "${BUILD_DIR}/lib/libbitcoin_node.a" ]; then
    echo "Error: Cannot find pre-built libraries at ${BUILD_DIR}" >&2
    echo "Build Bitcoin Core first: cmake -B build && cmake --build build -j\$(nproc)" >&2
    exit 1
fi

cmake -B "${SCRIPT_DIR}/_build" -S "${SCRIPT_DIR}" \
    -DBITCOIN_SOURCE_DIR="${SOURCE_DIR}" \
    -DBITCOIN_BUILD_DIR="${BUILD_DIR}"

cmake --build "${SCRIPT_DIR}/_build" -j"$(nproc)"

echo "Built: ${BUILD_DIR}/bin/txgraph-replay"
