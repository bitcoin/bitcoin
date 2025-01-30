#!/usr/bin/env bash
# Copyright (c) 2021-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# This script is executed inside the builder image

export LC_ALL=C.UTF-8

set -e

source ./ci/dash/matrix.sh

if [ "$RUN_UNIT_TESTS" != "true" ] && [ "$RUN_UNIT_TESTS_SEQUENTIAL" != "true" ]; then
  echo "Skipping unit tests"
  exit 0
fi

export BOOST_TEST_RANDOM=${BOOST_TEST_RANDOM:-1}
export LD_LIBRARY_PATH=$DEPENDS_DIR/$HOST/lib

export WINEDEBUG=fixme-all
export BOOST_TEST_LOG_LEVEL=test_suite

cd build-ci/dashcore-$BUILD_TARGET

if [ "$DIRECT_WINE_EXEC_TESTS" = "true" ]; then
  # Inside Docker, binfmt isn't working so we can't trust in make invoking windows binaries correctly
  wine ./src/test/test_dash.exe
else
  if [ "$RUN_UNIT_TESTS_SEQUENTIAL" = "true" ]; then
    ./src/test/test_dash --catch_system_errors=no -l test_suite
  else
      make $MAKEJOBS check VERBOSE=1
  fi
fi
