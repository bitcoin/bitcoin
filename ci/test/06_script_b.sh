#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

# Temporarily disable errexit, because Travis macOS fails without error message
set +o errexit
cd "build/bitcoin-$HOST" || (echo "could not enter distdir build/bitcoin-$HOST"; exit 1)
set -o errexit

if [ -n "$QEMU_USER_CMD" ]; then
  BEGIN_FOLD wrap-qemu
  echo "Prepare to run functional tests for HOST=$HOST"
  # Generate all binaries, so that they can be wrapped
  DOCKER_EXEC make $MAKEJOBS -C src/secp256k1 VERBOSE=1
  DOCKER_EXEC make $MAKEJOBS -C src/univalue VERBOSE=1
  for b_name in {"${BASE_OUTDIR}/bin"/*,src/secp256k1/*tests,src/univalue/{no_nul,test_json,unitester,object}}; do
    # shellcheck disable=SC2044
    for b in $(find "${BASE_ROOT_DIR}" -executable -type f -name $(basename $b_name)); do
      echo "Wrap $b ..."
      DOCKER_EXEC mv "$b" "${b}_orig"
      DOCKER_EXEC echo "\#\!/usr/bin/env bash" \> "$b"
      DOCKER_EXEC echo "$QEMU_USER_CMD \\\"${b}_orig\\\" \\\"\\\$@\\\"" \>\> "$b"
      DOCKER_EXEC chmod +x "$b"
    done
  done
  END_FOLD
fi

if [ "$RUN_UNIT_TESTS" = "true" ]; then
  BEGIN_FOLD unit-tests
  bash -c "${CI_WAIT}" &  # Print dots in case the unit tests take a long time to run
  DOCKER_EXEC LD_LIBRARY_PATH=$BASE_BUILD_DIR/depends/$HOST/lib make $MAKEJOBS check VERBOSE=1
  END_FOLD
fi

if [ "$RUN_FUNCTIONAL_TESTS" = "true" ]; then
  BEGIN_FOLD functional-tests
  DOCKER_EXEC test/functional/test_runner.py --ci $MAKEJOBS --tmpdirprefix "${BASE_SCRATCH_DIR}/test_runner/" --ansi --combinedlogslen=4000 ${TEST_RUNNER_EXTRA} --quiet --failfast
  END_FOLD
fi

if [ "$RUN_FUZZ_TESTS" = "true" ]; then
  BEGIN_FOLD fuzz-tests
  DOCKER_EXEC test/fuzz/test_runner.py -l DEBUG ${DIR_FUZZ_IN}
  END_FOLD
fi
