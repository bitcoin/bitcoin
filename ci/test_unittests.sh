#!/usr/bin/env bash

# This script is executed inside the builder image

set -e

source ./ci/matrix.sh

if [ "$RUN_TESTS" != "true" ]; then
  echo "Skipping unit tests"
  exit 0
fi

# TODO this is not Travis agnostic
export BOOST_TEST_RANDOM=1$TRAVIS_BUILD_ID
export LD_LIBRARY_PATH=$BUILD_DIR/depends/$HOST/lib

export WINEDEBUG=fixme-all
export BOOST_TEST_LOG_LEVEL=test_suite

cd build-ci/dashcore-$BUILD_TARGET

if [ "$RUN_TESTS" = "true" -a "${DEP_OPTS#*NO_QT=1}" = "$DEP_OPTS" ]; then
  export DISPLAY=:99.0;
  # Start xvfb if needed, as documented at https://docs.travis-ci.com/user/gui-and-headless-browsers/#Using-xvfb-to-Run-Tests-That-Require-a-GUI
  /sbin/start-stop-daemon --start --pidfile /tmp/custom_xvfb_99.pid --make-pidfile --background --exec /usr/bin/Xvfb -- :99 -ac;
fi

if [ "$DIRECT_WINE_EXEC_TESTS" = "true" ]; then
  # Inside Docker, binfmt isn't working so we can't trust in make invoking windows binaries correctly
  wine ./src/test/test_dash.exe
else
  make $MAKEJOBS check VERBOSE=1
fi
