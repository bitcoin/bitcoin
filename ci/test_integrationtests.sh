#!/usr/bin/env bash
#
# This script is executed inside the builder image

export LC_ALL=C.UTF-8

set -e

PASS_ARGS="$*"

source ./ci/matrix.sh

if [ "$RUN_INTEGRATIONTESTS" != "true" ]; then
  echo "Skipping integration tests"
  exit 0
fi

export LD_LIBRARY_PATH=$BUILD_DIR/depends/$HOST/lib

cd build-ci/dashcore-$BUILD_TARGET

if [ "$SOCKETEVENTS" = "" ]; then
  # Let's switch socketevents mode to some random mode
  R=$(($RANDOM%3))
  if [ "$R" == "0" ]; then
    SOCKETEVENTS="select"
  elif [ "$R" == "1" ]; then
    SOCKETEVENTS="poll"
  else
    SOCKETEVENTS="epoll"
  fi
fi
echo "Using socketevents mode: $SOCKETEVENTS"
EXTRA_ARGS="--dashd-arg=-socketevents=$SOCKETEVENTS"

set +e
./test/functional/test_runner.py --ci --combinedlogslen=4000 --coverage --failfast --nocleanup --tmpdir=$(pwd)/testdatadirs $PASS_ARGS $EXTRA_ARGS
RESULT=$?
set -e

echo "Collecting logs..."
BASEDIR=$(ls testdatadirs)
if [ "$BASEDIR" != "" ]; then
  mkdir testlogs
  TESTDATADIRS=$(ls testdatadirs/$BASEDIR)
  for d in $TESTDATADIRS; do
    [[ "$d" ]] || break # found nothing
    [[ "$d" != "cache" ]] || continue # skip cache dir
    mkdir testlogs/$d
    PYTHONIOENCODING=UTF-8 ./test/functional/combine_logs.py -c ./testdatadirs/$BASEDIR/$d > ./testlogs/$d/combined.log
    PYTHONIOENCODING=UTF-8 ./test/functional/combine_logs.py --html ./testdatadirs/$BASEDIR/$d > ./testlogs/$d/combined.html
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
