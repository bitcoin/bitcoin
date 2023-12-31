#!/usr/bin/env bash
# Copyright (c) 2019-2023 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# This script is meant to be sourced into the actual build script. It contains the build matrix and will set all
# necessary environment variables for the request build target

export LC_ALL=C.UTF-8

source ./ci/test/00_setup_env.sh

# Configure sanitizers options
export ASAN_OPTIONS=""
export LSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/lsan"
export TSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/tsan:halt_on_error=1"
export UBSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/ubsan"

if [ "$BUILD_TARGET" = "arm-linux" ]; then
  source ./ci/test/00_setup_env_arm.sh
elif [ "$BUILD_TARGET" = "win64" ]; then
  source ./ci/test/00_setup_env_win64.sh
elif [ "$BUILD_TARGET" = "linux64" ]; then
  source ./ci/test/00_setup_env_native_qt5.sh
elif [ "$BUILD_TARGET" = "linux64_asan" ]; then
  source ./ci/test/00_setup_env_native_asan.sh
elif [ "$BUILD_TARGET" = "linux64_tsan" ]; then
  source ./ci/test/00_setup_env_native_tsan.sh
elif [ "$BUILD_TARGET" = "linux64_ubsan" ]; then
  source ./ci/test/00_setup_env_native_ubsan.sh
elif [ "$BUILD_TARGET" = "linux64_fuzz" ]; then
  source ./ci/test/00_setup_env_native_fuzz.sh
elif [ "$BUILD_TARGET" = "linux64_cxx20" ]; then
  source ./ci/test/00_setup_env_native_cxx20.sh
elif [ "$BUILD_TARGET" = "linux64_sqlite" ]; then
  source ./ci/test/00_setup_env_native_sqlite.sh
elif [ "$BUILD_TARGET" = "linux64_nowallet" ]; then
  source ./ci/test/00_setup_env_native_nowallet.sh
elif [ "$BUILD_TARGET" = "linux64_valgrind" ]; then
  source ./ci/test/00_setup_env_native_valgrind.sh
elif [ "$BUILD_TARGET" = "mac" ]; then
  source ./ci/test/00_setup_env_mac.sh
elif [ "$BUILD_TARGET" = "s390x" ]; then
  source ./ci/test/00_setup_env_s390x.sh
fi
