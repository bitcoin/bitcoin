#!/usr/bin/env bash

# This script is executed inside the builder image

set -e

PASS_ARGS="$@"

source ./ci/matrix.sh

if [ "$RUN_INTEGRATIONTESTS" != "true" ]; then
  echo "Skipping integration tests"
  exit 0
fi

export LD_LIBRARY_PATH=$BUILD_DIR/depends/$HOST/lib

cd build-ci/dashcore-$BUILD_TARGET

set +e
./test/functional/test_runner.py --coverage --quiet --nocleanup --tmpdir=$(pwd)/testdatadirs $PASS_ARGS
RESULT=$?
set -e

echo "Collecting logs..."
BASEDIR=$(ls testdatadirs)
if [ "$BASEDIR" != "" ]; then
  mkdir testlogs
  for d in $(ls testdatadirs/$BASEDIR | grep -v '^cache$'); do
    mkdir testlogs/$d
    ./test/functional/combine_logs.py -c ./testdatadirs/$BASEDIR/$d > ./testlogs/$d/combined.log
    ./test/functional/combine_logs.py --html ./testdatadirs/$BASEDIR/$d > ./testlogs/$d/combined.html
    cd testdatadirs/$BASEDIR/$d
    LOGFILES="$(find . -name 'debug.log' -or -name "test_framework.log")"
    cd ../../..
    for f in $LOGFILES; do
      d2="testlogs/$d/$(dirname $f)"
      mkdir -p $d2
      cp testdatadirs/$BASEDIR/$d/$f $d2/
    done
  done
fi

mv testlogs ../../

exit $RESULT
