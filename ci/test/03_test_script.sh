#!/usr/bin/env bash
#
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -ex

export ASAN_OPTIONS="detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1"
export LSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/lsan"
export TSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/tsan:halt_on_error=1"
export UBSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/ubsan:print_stacktrace=1:halt_on_error=1:report_error_type=1"

if [ "$CI_OS_NAME" == "macos" ]; then
  top -l 1 -s 0 | awk ' /PhysMem/ {print}'
  echo "Number of CPUs: $(sysctl -n hw.logicalcpu)"
else
  free -m -h
  echo "Number of CPUs (nproc): $(nproc)"
  echo "System info: $(uname --kernel-name --kernel-release)"
  lscpu
fi
echo "Free disk space:"
df -h

# What host to compile for. See also ./depends/README.md
# Tests that need cross-compilation export the appropriate HOST.
# Tests that run natively guess the host
export HOST=${HOST:-$("$BASE_ROOT_DIR/depends/config.guess")}

(
  # compact->outputs[i].file_size is uninitialized memory, so reading it is UB.
  # The statistic bytes_written is only used for logging, which is disabled in
  # CI, so as a temporary minimal fix to work around UB and CI failures, leave
  # bytes_written unmodified.
  # See https://github.com/bitcoin/bitcoin/pull/28359#issuecomment-1698694748
  # Tee patch to stdout to make it clear CI is testing modified code.
  tee >(patch -p1) <<'EOF'
--- a/src/leveldb/db/db_impl.cc
+++ b/src/leveldb/db/db_impl.cc
@@ -1028,9 +1028,6 @@ Status DBImpl::DoCompactionWork(CompactionState* compact) {
       stats.bytes_read += compact->compaction->input(which, i)->file_size;
     }
   }
-  for (size_t i = 0; i < compact->outputs.size(); i++) {
-    stats.bytes_written += compact->outputs[i].file_size;
-  }

   mutex_.Lock();
   stats_[compact->compaction->level() + 1].Add(stats);
EOF
)

if [ "$RUN_FUZZ_TESTS" = "true" ]; then
  export DIR_FUZZ_IN=${DIR_QA_ASSETS}/fuzz_seed_corpus/
  if [ ! -d "$DIR_FUZZ_IN" ]; then
    ${CI_RETRY_EXE} git clone --depth=1 https://github.com/bitcoin-core/qa-assets "${DIR_QA_ASSETS}"
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
    ${CI_RETRY_EXE} curl --location --fail https://github.com/bitcoin-core/qa-assets/raw/main/unit_test_data/script_assets_test.json -o "${DIR_UNIT_TEST_DATA}/script_assets_test.json"
  fi
fi

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

BITCOIN_CONFIG_ALL="--disable-dependency-tracking"
if [ -z "$NO_DEPENDS" ]; then
  BITCOIN_CONFIG_ALL="${BITCOIN_CONFIG_ALL} CONFIG_SITE=$DEPENDS_DIR/$HOST/share/config.site"
fi
if [ -z "$NO_WERROR" ]; then
  BITCOIN_CONFIG_ALL="${BITCOIN_CONFIG_ALL} --enable-werror"
fi

ccache --zero-stats
PRINT_CCACHE_STATISTICS="ccache --version | head -n 1 && ccache --show-stats"

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

if [ -n "$USE_VALGRIND" ]; then
  "${BASE_ROOT_DIR}/ci/test/wrap-valgrind.sh"
fi

if [ "$RUN_UNIT_TESTS" = "true" ]; then
  DIR_UNIT_TEST_DATA="${DIR_UNIT_TEST_DATA}" LD_LIBRARY_PATH="${DEPENDS_DIR}/${HOST}/lib" make "${MAKEJOBS}" check VERBOSE=1
fi

if [ "$RUN_UNIT_TESTS_SEQUENTIAL" = "true" ]; then
  DIR_UNIT_TEST_DATA="${DIR_UNIT_TEST_DATA}" LD_LIBRARY_PATH="${DEPENDS_DIR}/${HOST}/lib" "${BASE_OUTDIR}"/bin/test_bitcoin --catch_system_errors=no -l test_suite
fi

if [ "$RUN_FUNCTIONAL_TESTS" = "true" ]; then
  # shellcheck disable=SC2086
  LD_LIBRARY_PATH="${DEPENDS_DIR}/${HOST}/lib" test/functional/test_runner.py --ci "${MAKEJOBS}" --tmpdirprefix "${BASE_SCRATCH_DIR}"/test_runner/ --ansi --combinedlogslen=99999999 --timeout-factor="${TEST_RUNNER_TIMEOUT_FACTOR}" ${TEST_RUNNER_EXTRA} --quiet --failfast
fi

if [ "${RUN_TIDY}" = "true" ]; then
  cmake -B /tidy-build -DLLVM_DIR=/usr/lib/llvm-"${TIDY_LLVM_V}"/cmake -DCMAKE_BUILD_TYPE=Release -S "${BASE_ROOT_DIR}"/contrib/devtools/bitcoin-tidy
  cmake --build /tidy-build "$MAKEJOBS"
  cmake --build /tidy-build --target bitcoin-tidy-tests "$MAKEJOBS"

  set -eo pipefail
  cd "${BASE_BUILD_DIR}/bitcoin-$HOST/src/"
  if ! ( run-clang-tidy-"${TIDY_LLVM_V}" -quiet -load="/tidy-build/libbitcoin-tidy.so" "${MAKEJOBS}" | tee tmp.tidy-out.txt ); then
    grep -C5 "error: " tmp.tidy-out.txt
    echo "^^^ ⚠️ Failure generated from clang-tidy"
    false
  fi
  # Filter out files by regex here, because regex may not be
  # accepted in src/.bear-tidy-config
  # Filter out:
  # * qt qrc and moc generated files
  jq 'map(select(.file | test("src/qt/qrc_.*\\.cpp$|/moc_.*\\.cpp$") | not))' ../compile_commands.json > tmp.json
  mv tmp.json ../compile_commands.json
  cd "${BASE_BUILD_DIR}/bitcoin-$HOST/"
  python3 "/include-what-you-use/iwyu_tool.py" \
           -p . "${MAKEJOBS}" \
           -- -Xiwyu --cxx17ns -Xiwyu --mapping_file="${BASE_BUILD_DIR}/bitcoin-$HOST/contrib/devtools/iwyu/bitcoin.core.imp" \
           -Xiwyu --max_line_length=160 \
           2>&1 | tee /tmp/iwyu_ci.out
  cd "${BASE_ROOT_DIR}/src"
  python3 "/include-what-you-use/fix_includes.py" --nosafe_headers < /tmp/iwyu_ci.out
  git --no-pager diff
fi

if [ "$RUN_FUZZ_TESTS" = "true" ]; then
  # shellcheck disable=SC2086
  LD_LIBRARY_PATH="${DEPENDS_DIR}/${HOST}/lib" test/fuzz/test_runner.py ${FUZZ_TESTS_CONFIG} "${MAKEJOBS}" -l DEBUG "${DIR_FUZZ_IN}" --empty_min_time=60
fi
