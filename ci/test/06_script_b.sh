#!/usr/bin/env bash
#
# Copyright (c) 2018-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

if [[ $HOST = *-mingw32 ]]; then
  # Generate all binaries, so that they can be wrapped
  CI_EXEC make "$MAKEJOBS" -C src/secp256k1 VERBOSE=1
  CI_EXEC "${BASE_ROOT_DIR}/ci/test/wrap-wine.sh"
fi

if [ -n "$QEMU_USER_CMD" ]; then
  # Generate all binaries, so that they can be wrapped
  CI_EXEC make "$MAKEJOBS" -C src/secp256k1 VERBOSE=1
  CI_EXEC "${BASE_ROOT_DIR}/ci/test/wrap-qemu.sh"
fi

if [ -n "$USE_VALGRIND" ]; then
  CI_EXEC "${BASE_ROOT_DIR}/ci/test/wrap-valgrind.sh"
fi

if [ "$RUN_UNIT_TESTS" = "true" ]; then
  CI_EXEC "${TEST_RUNNER_ENV}" DIR_UNIT_TEST_DATA="${DIR_UNIT_TEST_DATA}" LD_LIBRARY_PATH="${DEPENDS_DIR}/${HOST}/lib" make "$MAKEJOBS" check VERBOSE=1
fi

if [ "$RUN_UNIT_TESTS_SEQUENTIAL" = "true" ]; then
  CI_EXEC "${TEST_RUNNER_ENV}" DIR_UNIT_TEST_DATA="${DIR_UNIT_TEST_DATA}" LD_LIBRARY_PATH="${DEPENDS_DIR}/${HOST}/lib" "${BASE_OUTDIR}/bin/test_bitcoin" --catch_system_errors=no -l test_suite
fi

if [ "$RUN_FUNCTIONAL_TESTS" = "true" ]; then
  CI_EXEC LD_LIBRARY_PATH="${DEPENDS_DIR}/${HOST}/lib" "${TEST_RUNNER_ENV}" test/functional/test_runner.py --ci "$MAKEJOBS" --tmpdirprefix "${BASE_SCRATCH_DIR}/test_runner/" --ansi --combinedlogslen=4000 --timeout-factor="${TEST_RUNNER_TIMEOUT_FACTOR}" "${TEST_RUNNER_EXTRA}" --quiet --failfast
fi

if [ "${RUN_TIDY}" = "true" ]; then
  export P_CI_DIR="${BASE_BUILD_DIR}/bitcoin-$HOST/src/"
  CI_EXEC run-clang-tidy "${MAKEJOBS}"
fi

if [ "$RUN_SECURITY_TESTS" = "true" ]; then
  CI_EXEC make test-security-check
fi

if [ "$RUN_FUZZ_TESTS" = "true" ]; then
  CI_EXEC LD_LIBRARY_PATH="${DEPENDS_DIR}/${HOST}/lib" test/fuzz/test_runner.py "${FUZZ_TESTS_CONFIG}" "$MAKEJOBS" -l DEBUG "${DIR_FUZZ_IN}"
fi
