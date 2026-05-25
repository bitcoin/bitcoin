#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

export LC_ALL=C.UTF-8

set -o errexit -o pipefail -o xtrace

NUM_JOBS=${NUM_JOBS:-1}
MAKEJOBS=${MAKEJOBS:--j${NUM_JOBS}}
BUILD_DIR=${BUILD_DIR:-build}
RAW_PROFILE_DIR="${BUILD_DIR}/raw_profile_data"
RAW_PROFILE_FILE="${PWD}/${RAW_PROFILE_DIR}/%m_%p.profraw"

build() {
  local -a launcher_args=()
  if command -v ccache >/dev/null 2>&1; then
    ccache --zero-stats
    launcher_args=(
      -DCMAKE_C_COMPILER_LAUNCHER=ccache
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
    )
  fi

  cmake -B "${BUILD_DIR}" -DCMAKE_C_COMPILER="clang" \
    -DCMAKE_CXX_COMPILER="clang++" \
    "${launcher_args[@]}" \
    -DAPPEND_CFLAGS="-fprofile-instr-generate -fcoverage-mapping" \
    -DAPPEND_CXXFLAGS="-fprofile-instr-generate -fcoverage-mapping" \
    -DAPPEND_LDFLAGS="-fprofile-instr-generate -fcoverage-mapping"
  cmake --build "${BUILD_DIR}" "${MAKEJOBS}"

  if command -v ccache >/dev/null 2>&1; then
    ccache --show-stats --verbose
  fi
}

test_coverage() {
  mkdir -p "${RAW_PROFILE_DIR}"
  LLVM_PROFILE_FILE="${RAW_PROFILE_FILE}" ctest --test-dir "${BUILD_DIR}" -j "${NUM_JOBS}"
  LLVM_PROFILE_FILE="${RAW_PROFILE_FILE}" "${BUILD_DIR}/test/functional/test_runner.py" -j "${NUM_JOBS}"
  find "${RAW_PROFILE_DIR}" -name "*.profraw" > "${BUILD_DIR}/raw_profile_data_files.txt"
  llvm-profdata merge -f "${BUILD_DIR}/raw_profile_data_files.txt" -o "${BUILD_DIR}/coverage.profdata"
}

generate_report() {
  llvm-cov show \
    --object="${BUILD_DIR}/bin/test_bitcoin" \
    --object="${BUILD_DIR}/bin/bitcoind" \
    -Xdemangler=llvm-cxxfilt \
    --instr-profile="${BUILD_DIR}/coverage.profdata" \
    --ignore-filename-regex="src/crc32c/|src/leveldb/|src/minisketch/|src/secp256k1/|src/test/" \
    --format=html \
    --show-instantiation-summary \
    --show-line-counts-or-regions \
    --show-expansions \
    --output-dir="${BUILD_DIR}/coverage_report" \
    --project-title="Bitcoin Core Coverage Report"
}

case "${1:-all}" in
  build)
    build
    ;;
  test)
    test_coverage
    ;;
  cov)
    generate_report
    ;;
  all)
    build
    test_coverage
    generate_report
    ;;
  *)
    echo "Usage: $0 [build|test|cov|all]" >&2
    exit 1
    ;;
esac
