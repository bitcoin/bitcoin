#!/usr/bin/env bash
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

if [[ $HOST = *-mingw32 ]]; then
  # Generate all binaries, so that they can be wrapped
  CI_EXEC make "$MAKEJOBS" -C src/secp256k1 VERBOSE=1
  CI_EXEC make "$MAKEJOBS" -C src minisketch/test.exe VERBOSE=1
  CI_EXEC "${BASE_ROOT_DIR}/ci/test/wrap-wine.sh"
fi

if [ -n "$QEMU_USER_CMD" ]; then
  # Generate all binaries, so that they can be wrapped
  CI_EXEC make "$MAKEJOBS" -C src/secp256k1 VERBOSE=1
  CI_EXEC make "$MAKEJOBS" -C src minisketch/test VERBOSE=1
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
  CI_EXEC LD_LIBRARY_PATH="${DEPENDS_DIR}/${HOST}/lib" "${TEST_RUNNER_ENV}" test/functional/test_runner.py --ci "$MAKEJOBS" --tmpdirprefix "${BASE_SCRATCH_DIR}/test_runner/" --ansi --combinedlogslen=99999999 --timeout-factor="${TEST_RUNNER_TIMEOUT_FACTOR}" "${TEST_RUNNER_EXTRA}" --quiet --failfast
fi

if [ "${RUN_TIDY}" = "true" ]; then
  set -eo pipefail
  export P_CI_DIR="${BASE_BUILD_DIR}/bitcoin-$HOST/src/"
  ( CI_EXEC run-clang-tidy-16 -quiet "${MAKEJOBS}" ) | grep -C5 "error"
  export P_CI_DIR="${BASE_BUILD_DIR}/bitcoin-$HOST/"
  CI_EXEC "python3 ${DIR_IWYU}/include-what-you-use/iwyu_tool.py"\
          " src/common/init.cpp"\
          " src/common/url.cpp"\
          " src/compat"\
          " src/dbwrapper.cpp"\
          " src/init"\
          " src/kernel"\
          " src/node/chainstate.cpp"\
          " src/node/chainstatemanager_args.cpp"\
          " src/node/mempool_args.cpp"\
          " src/node/minisketchwrapper.cpp"\
          " src/node/utxo_snapshot.cpp"\
          " src/node/validation_cache_args.cpp"\
          " src/policy/feerate.cpp"\
          " src/policy/packages.cpp"\
          " src/policy/settings.cpp"\
          " src/primitives/transaction.cpp"\
          " src/random.cpp"\
          " src/rpc/fees.cpp"\
          " src/rpc/signmessage.cpp"\
          " src/test/fuzz/string.cpp"\
          " src/test/fuzz/txorphan.cpp"\
          " src/test/fuzz/util/"\
          " src/test/util/coins.cpp"\
          " src/uint256.cpp"\
          " src/util/bip32.cpp"\
          " src/util/bytevectorhash.cpp"\
          " src/util/check.cpp"\
          " src/util/error.cpp"\
          " src/util/exception.cpp"\
          " src/util/getuniquepath.cpp"\
          " src/util/hasher.cpp"\
          " src/util/message.cpp"\
          " src/util/moneystr.cpp"\
          " src/util/serfloat.cpp"\
          " src/util/spanparsing.cpp"\
          " src/util/strencodings.cpp"\
          " src/util/string.cpp"\
          " src/util/syserror.cpp"\
          " src/util/threadinterrupt.cpp"\
          " src/zmq"\
          " -p . ${MAKEJOBS}"\
          " -- -Xiwyu --cxx17ns -Xiwyu --mapping_file=${BASE_BUILD_DIR}/bitcoin-$HOST/contrib/devtools/iwyu/bitcoin.core.imp"\
          " |& tee /tmp/iwyu_ci.out"
  export P_CI_DIR="${BASE_ROOT_DIR}/src"
  CI_EXEC "python3 ${DIR_IWYU}/include-what-you-use/fix_includes.py --nosafe_headers < /tmp/iwyu_ci.out"
  CI_EXEC "git --no-pager diff"
fi

if [ "$RUN_SECURITY_TESTS" = "true" ]; then
  CI_EXEC make test-security-check
fi

if [ "$RUN_FUZZ_TESTS" = "true" ]; then
  CI_EXEC LD_LIBRARY_PATH="${DEPENDS_DIR}/${HOST}/lib" test/fuzz/test_runner.py "${FUZZ_TESTS_CONFIG}" "$MAKEJOBS" -l DEBUG "${DIR_FUZZ_IN}"
fi

if [ -z "$DANGER_RUN_CI_ON_HOST" ]; then
  echo "Stop and remove CI container by ID"
  docker container kill "${CI_CONTAINER_ID}"
fi
