#!/usr/bin/env bash

# This script is executed inside the builder image

set -e

PASS_ARGS="$@"

source ./ci/matrix.sh

if [ "$RUN_TESTS" != "true" ]; then
  echo "Skipping integration tests"
  exit 0
fi

export LD_LIBRARY_PATH=$BUILD_DIR/depends/$HOST/lib

cd build-ci/dashcore-$BUILD_TARGET

./qa/pull-tester/rpc-tests.py --coverage $PASS_ARGS
