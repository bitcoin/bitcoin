#!/usr/bin/env bash
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -ex

if [ "$CI_OS_NAME" == "macos" ]; then
  top -l 1 -s 0 | awk ' /PhysMem/ {print}'
  echo "Number of CPUs: $(sysctl -n hw.logicalcpu)"
else
  free -m -h
  echo "Number of CPUs (nproc): $(nproc)"
  lscpu | grep Endian
fi
echo "Free disk space:"
df -h

if [ "$RUN_FUZZ_TESTS" = "true" ]; then
  export DIR_FUZZ_IN=${DIR_QA_ASSETS}/fuzz_seed_corpus/
  if [ ! -d "$DIR_FUZZ_IN" ]; then
    git clone --depth=1 https://github.com/bitcoin-core/qa-assets "${DIR_QA_ASSETS}"
  fi
  (
    cd "${DIR_QA_ASSETS}"
    echo "Using qa-assets repo from commit ..."
    git log -1
  )
elif [ "$RUN_UNIT_TESTS" = "true" ] || [ "$RUN_UNIT_TESTS_SEQUENTIAL" = "true" ]; then
  export DIR_UNIT_TEST_DATA=${DIR_QA_ASSETS}/unit_test_data/
  if [ ! -d "$DIR_UNIT_TEST_DATA" ]; then
    mkdir -p "$DIR_UNIT_TEST_DATA"
    curl --location --fail https://github.com/bitcoin-core/qa-assets/raw/main/unit_test_data/script_assets_test.json -o "${DIR_UNIT_TEST_DATA}/script_assets_test.json"
  fi
fi

mkdir -p "${BASE_SCRATCH_DIR}/sanitizer-output/"

if [ "$USE_BUSY_BOX" = "true" ]; then
  echo "Setup to use BusyBox utils"
  # tar excluded for now because it requires passing in the exact archive type in ./depends (fixed in later BusyBox version)
  # ar excluded for now because it does not recognize the -q option in ./depends (unknown if fixed)
  for util in $(busybox --list | grep -v "^ar$" | grep -v "^tar$" ); do ln -s "$(command -v busybox)" "${BINS_SCRATCH_DIR}/$util"; done
  # Print BusyBox version
  patch --help
fi

# Make sure default datadir does not exist and is never read by creating a dummy file
if [ "$CI_OS_NAME" == "macos" ]; then
  echo > "${HOME}/Library/Application Support/Bitcoin"
else
  echo > "${HOME}/.bitcoin"
fi

if [ -z "$NO_DEPENDS" ]; then
  if [[ $CI_IMAGE_NAME_TAG == *centos* ]]; then
    SHELL_OPTS="CONFIG_SHELL=/bin/dash"
  else
    SHELL_OPTS="CONFIG_SHELL="
  fi
  bash -c "$SHELL_OPTS make $MAKEJOBS -C depends HOST=$HOST $DEP_OPTS LOG=1"
fi
if [ "$DOWNLOAD_PREVIOUS_RELEASES" = "true" ]; then
  test/get_previous_releases.py -b -t "$PREVIOUS_RELEASES_DIR"
fi

BITCOIN_CONFIG_ALL="--enable-suppress-external-warnings --disable-dependency-tracking"
if [ -z "$NO_DEPENDS" ]; then
  BITCOIN_CONFIG_ALL="${BITCOIN_CONFIG_ALL} CONFIG_SITE=$DEPENDS_DIR/$HOST/share/config.site"
fi
if [ -z "$NO_WERROR" ]; then
  BITCOIN_CONFIG_ALL="${BITCOIN_CONFIG_ALL} --enable-werror"
fi

ccache --zero-stats --max-size="${CCACHE_SIZE}"
PRINT_CCACHE_STATISTICS="ccache --version | head -n 1 && ccache --show-stats"

if [ -n "$ANDROID_TOOLS_URL" ]; then
  make distclean || true
  ./autogen.sh
  bash -c "./configure $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG" || ( (cat config.log) && false)
  make "${MAKEJOBS}" && cd src/qt && ANDROID_HOME=${ANDROID_HOME} ANDROID_NDK_HOME=${ANDROID_NDK_HOME} make apk
  bash -c "${PRINT_CCACHE_STATISTICS}"
  exit 0
fi

BITCOIN_CONFIG_ALL="${BITCOIN_CONFIG_ALL} --enable-external-signer --prefix=$BASE_OUTDIR"

if [ -n "$CONFIG_SHELL" ]; then
  "$CONFIG_SHELL" -c "./autogen.sh"
else
  ./autogen.sh
fi

mkdir -p "${BASE_BUILD_DIR}"
cd "${BASE_BUILD_DIR}"

bash -c "${BASE_ROOT_DIR}/configure --cache-file=config.cache $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG" || ( (cat config.log) && false)

make distdir VERSION="$HOST"

cd "${BASE_BUILD_DIR}/bitcoin-$HOST"

bash -c "./configure --cache-file=../config.cache $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG" || ( (cat config.log) && false)

if [[ "${RUN_TIDY}" == "true" ]]; then
  MAYBE_BEAR="bear --config src/.bear-tidy-config"
  MAYBE_TOKEN="--"
fi

bash -c "${MAYBE_BEAR} ${MAYBE_TOKEN} make $MAKEJOBS $GOAL" || ( echo "Build failure. Verbose build follows." && make "$GOAL" V=1 ; false )

bash -c "${PRINT_CCACHE_STATISTICS}"
du -sh "${DEPENDS_DIR}"/*/
du -sh "${PREVIOUS_RELEASES_DIR}"

if [[ $HOST = *-mingw32 ]]; then
  # Generate all binaries, so that they can be wrapped
  make "$MAKEJOBS" -C src/secp256k1 VERBOSE=1
  make "$MAKEJOBS" -C src minisketch/test.exe VERBOSE=1
  "${BASE_ROOT_DIR}/ci/test/wrap-wine.sh"
fi

if [ -n "$QEMU_USER_CMD" ]; then
  # Generate all binaries, so that they can be wrapped
  make "$MAKEJOBS" -C src/secp256k1 VERBOSE=1
  make "$MAKEJOBS" -C src minisketch/test VERBOSE=1
  "${BASE_ROOT_DIR}/ci/test/wrap-qemu.sh"
fi

if [ -n "$USE_VALGRIND" ]; then
  "${BASE_ROOT_DIR}/ci/test/wrap-valgrind.sh"
fi

if [ "$RUN_UNIT_TESTS" = "true" ]; then
  bash -c "${TEST_RUNNER_ENV} DIR_UNIT_TEST_DATA=${DIR_UNIT_TEST_DATA} LD_LIBRARY_PATH=${DEPENDS_DIR}/${HOST}/lib make $MAKEJOBS check VERBOSE=1"
fi

if [ "$RUN_UNIT_TESTS_SEQUENTIAL" = "true" ]; then
  bash -c "${TEST_RUNNER_ENV} DIR_UNIT_TEST_DATA=${DIR_UNIT_TEST_DATA} LD_LIBRARY_PATH=${DEPENDS_DIR}/${HOST}/lib ${BASE_OUTDIR}/bin/test_bitcoin --catch_system_errors=no -l test_suite"
fi

if [ "$RUN_FUNCTIONAL_TESTS" = "true" ]; then
  bash -c "LD_LIBRARY_PATH=${DEPENDS_DIR}/${HOST}/lib ${TEST_RUNNER_ENV} test/functional/test_runner.py --ci $MAKEJOBS --tmpdirprefix ${BASE_SCRATCH_DIR}/test_runner/ --ansi --combinedlogslen=99999999 --timeout-factor=${TEST_RUNNER_TIMEOUT_FACTOR} ${TEST_RUNNER_EXTRA} --quiet --failfast"
fi

if [ "${RUN_TIDY}" = "true" ]; then
  set -eo pipefail
  cd "${BASE_BUILD_DIR}/bitcoin-$HOST/src/"
  ( run-clang-tidy-16 -quiet "${MAKEJOBS}" ) | grep -C5 "error"
  # Filter out files by regex here, because regex may not be
  # accepted in src/.bear-tidy-config
  # Filter out:
  # * qt qrc and moc generated files
  # * secp256k1
  jq 'map(select(.file | test("src/qt/qrc_.*\\.cpp$|/moc_.*\\.cpp$|src/secp256k1/src/") | not))' ../compile_commands.json > tmp.json
  mv tmp.json ../compile_commands.json
  cd "${BASE_BUILD_DIR}/bitcoin-$HOST/"
  python3 "${DIR_IWYU}/include-what-you-use/iwyu_tool.py" \
           -p . "${MAKEJOBS}" \
           -- -Xiwyu --cxx17ns -Xiwyu --mapping_file="${BASE_BUILD_DIR}/bitcoin-$HOST/contrib/devtools/iwyu/bitcoin.core.imp" \
           -Xiwyu --max_line_length=160 \
           2>&1 | tee /tmp/iwyu_ci.out
  cd "${BASE_ROOT_DIR}/src"
  python3 "${DIR_IWYU}/include-what-you-use/fix_includes.py" --nosafe_headers < /tmp/iwyu_ci.out
  git --no-pager diff
fi

if [ "$RUN_FUZZ_TESTS" = "true" ]; then
  bash -c "LD_LIBRARY_PATH=${DEPENDS_DIR}/${HOST}/lib test/fuzz/test_runner.py ${FUZZ_TESTS_CONFIG} $MAKEJOBS -l DEBUG ${DIR_FUZZ_IN}"
fi
