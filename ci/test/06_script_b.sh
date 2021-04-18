#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

if [[ $HOST = *-mingw32 ]]; then
  # Generate all binaries, so that they can be wrapped
  DOCKER_EXEC make $MAKEJOBS -C src/secp256k1 VERBOSE=1
  DOCKER_EXEC make $MAKEJOBS -C src/univalue VERBOSE=1
  DOCKER_EXEC "${BASE_ROOT_DIR}/ci/test/wrap-wine.sh"
fi

if [ -n "$QEMU_USER_CMD" ]; then
  # Generate all binaries, so that they can be wrapped
  DOCKER_EXEC make $MAKEJOBS -C src/secp256k1 VERBOSE=1
  DOCKER_EXEC make $MAKEJOBS -C src/univalue VERBOSE=1
  DOCKER_EXEC "${BASE_ROOT_DIR}/ci/test/wrap-qemu.sh"
fi

if [ -n "$USE_VALGRIND" ]; then
  DOCKER_EXEC "${BASE_ROOT_DIR}/ci/test/wrap-valgrind.sh"
fi

bash -c "${CI_WAIT}" &  # Print dots in case the tests take a long time to run

if [ "$RUN_UNIT_TESTS" = "true" ]; then
  DOCKER_EXEC LD_LIBRARY_PATH=$DEPENDS_DIR/$HOST/lib make $MAKEJOBS check VERBOSE=1
fi

if [ "$RUN_UNIT_TESTS_SEQUENTIAL" = "true" ]; then
  DOCKER_EXEC LD_LIBRARY_PATH=$DEPENDS_DIR/$HOST/lib "${BASE_ROOT_DIR}/build/omnicore-*/src/test/test_omnicore" --catch_system_errors=no -l test_suite
fi

if [ "$RUN_FUNCTIONAL_TESTS" = "true" ]; then
  DOCKER_EXEC test/functional/test_runner.py --ci $MAKEJOBS --tmpdirprefix "${BASE_SCRATCH_DIR}/test_runner/" --ansi --combinedlogslen=4000 ${TEST_RUNNER_EXTRA} --quiet --failfast
fi

if [ "$RUN_FUZZ_TESTS" = "true" ]; then
  DOCKER_EXEC test/fuzz/test_runner.py ${FUZZ_TESTS_CONFIG} -l DEBUG ${DIR_FUZZ_IN}
fi

if [ "$RUN_OMNIJ_TESTS" = "true" ]; then
  DOCKER_EXEC source /tmp/cirrus-ci-build/test/pull-tester/omnicore-rpc-tests.sh
fi

