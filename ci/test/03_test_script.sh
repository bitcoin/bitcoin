#!/usr/bin/env bash
#
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -ex

export ASAN_OPTIONS="detect_leaks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1"
export LSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/lsan"
export TSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/tsan:halt_on_error=1:second_deadlock_stack=1"
export UBSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/ubsan:print_stacktrace=1:halt_on_error=1:report_error_type=1"

echo "Number of available processing units: $(nproc)"
if [ "$CI_OS_NAME" == "macos" ]; then
  top -l 1 -s 0 | awk ' /PhysMem/ {print}'
else
  free -m -h
  echo "System info: $(uname --kernel-name --kernel-release)"
  lscpu
fi
echo "Free disk space:"
df -h

# We force an install of linux-headers again here via $PACKAGES to fix any
# kernel mismatch between a cached docker image and the underlying host.
# This can happen occasionally on hosted runners if the runner image is updated.
if [[ "$CONTAINER_NAME" == "ci_native_asan" ]]; then
  $CI_RETRY_EXE apt-get update
  ${CI_RETRY_EXE} bash -c "apt-get install --no-install-recommends --no-upgrade -y $PACKAGES"
fi

# What host to compile for. See also ./depends/README.md
# Tests that need cross-compilation export the appropriate HOST.
# Tests that run natively guess the host
export HOST=${HOST:-$("$BASE_ROOT_DIR/depends/config.guess")}

echo "=== BEGIN env ==="
env
echo "=== END env ==="

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
  export DIR_FUZZ_IN=${DIR_QA_ASSETS}/fuzz_corpora/
  if [ ! -d "$DIR_FUZZ_IN" ]; then
    ${CI_RETRY_EXE} git clone --depth=1 https://github.com/bitcoin-core/qa-assets "${DIR_QA_ASSETS}"
  fi
  (
    cd "${DIR_QA_ASSETS}"
    echo "Using qa-assets repo from commit ..."
    git log -1
  )
elif [ "$RUN_UNIT_TESTS" = "true" ]; then
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
  if [[ $CI_IMAGE_NAME_TAG == *alpine* ]]; then
    SHELL_OPTS="CONFIG_SHELL=/usr/bin/dash"
  else
    SHELL_OPTS="CONFIG_SHELL="
  fi
  bash -c "$SHELL_OPTS make $MAKEJOBS -C depends HOST=$HOST $DEP_OPTS LOG=1"
fi
if [ "$DOWNLOAD_PREVIOUS_RELEASES" = "true" ]; then
  test/get_previous_releases.py --target-dir "$PREVIOUS_RELEASES_DIR"
fi

BITCOIN_CONFIG_ALL="-DBUILD_BENCH=ON -DBUILD_FUZZ_BINARY=ON"
if [ -z "$NO_DEPENDS" ]; then
  BITCOIN_CONFIG_ALL="${BITCOIN_CONFIG_ALL} -DCMAKE_TOOLCHAIN_FILE=$DEPENDS_DIR/$HOST/toolchain.cmake"
fi
if [ -z "$NO_WERROR" ]; then
  BITCOIN_CONFIG_ALL="${BITCOIN_CONFIG_ALL} -DWERROR=ON"
fi

ccache --zero-stats
PRINT_CCACHE_STATISTICS="ccache --version | head -n 1 && ccache --show-stats"

# Folder where the build is done.
BASE_BUILD_DIR=${BASE_BUILD_DIR:-$BASE_SCRATCH_DIR/build-$HOST}

BITCOIN_CONFIG_ALL="$BITCOIN_CONFIG_ALL -DCMAKE_INSTALL_PREFIX=$BASE_OUTDIR -Werror=dev"

if [[ "${RUN_TIDY}" == "true" ]]; then
  BITCOIN_CONFIG_ALL="$BITCOIN_CONFIG_ALL -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
fi

eval "CMAKE_ARGS=($BITCOIN_CONFIG_ALL $BITCOIN_CONFIG)"
cmake -S "$BASE_ROOT_DIR" -B "$BASE_BUILD_DIR" "${CMAKE_ARGS[@]}" || (
  cd "${BASE_BUILD_DIR}"
  # shellcheck disable=SC2046
  cat $(cmake -P "${BASE_ROOT_DIR}/ci/test/GetCMakeLogFiles.cmake")
  false
)

# shellcheck disable=SC2086
cmake --build "${BASE_BUILD_DIR}" "$MAKEJOBS" --target all $GOAL || (
  echo "Build failure. Verbose build follows."
  # shellcheck disable=SC2086
  cmake --build "${BASE_BUILD_DIR}" -j1 --target all $GOAL --verbose
  false
)

bash -c "${PRINT_CCACHE_STATISTICS}"
if [ "$CI" = "true" ]; then
  hit_rate=$(ccache -s | grep "Hits:" | head -1 | sed 's/.*(\(.*\)%).*/\1/')
  if [ "${hit_rate%.*}" -lt 75 ]; then
      echo "::notice title=low ccache hitrate::Ccache hit-rate in $CONTAINER_NAME was $hit_rate%"
  fi
fi
du -sh "${DEPENDS_DIR}"/*/
du -sh "${PREVIOUS_RELEASES_DIR}"

if [ -n "${CI_LIMIT_STACK_SIZE}" ]; then
  ulimit -s 512
fi

if [[ ${BARE_METAL_RISCV} == "true" ]]; then
    echo -e "#include <script/script_error.h>\n int main() { return ScriptErrorString(ScriptError_t::SCRIPT_ERR_UNKNOWN_ERROR).size() > 0; }" > test.cpp

    /opt/riscv-ilp32/bin/riscv32-unknown-elf-g++ -I "${BASE_ROOT_DIR}"/src -g -std=c++20 -march=rv32gc -mabi=ilp32 -c test.cpp -o test.o

    echo -e ".section .text
          .global _start
          .type _start, @function

          _start:
              .option push
              .option norelax
              la gp, __global_pointer$
              .option pop

              call main

              # Put Exit2 system call number into the a7 register
              li a7, 93
              ecall" > start.s

    /opt/riscv-ilp32/bin/riscv32-unknown-elf-gcc -c start.s -o start.o

    echo -e "#include <sys/stat.h>
          void _exit(int code) { while(1); }
          int _sbrk(int incr) { return 0; }
          int _write(int file, char *ptr, int len) { return 0; }
          int _close(int file) { return -1; }
          int _fstat(int file, struct stat *st) { st->st_mode = S_IFCHR; return 0; }
          int _isatty(int file) { return 1; }
          int _lseek(int file, int ptr, int dir) { return 0; }
          int _read(int file, char *ptr, int len) { return 0; }
          int _kill(int pid, int sig) { return -1; }
          int _getpid(void) { return -1; }" > syscalls.c

    /opt/riscv-ilp32/bin/riscv32-unknown-elf-gcc -g -march=rv32i -mabi=ilp32 -c syscalls.c -o syscalls.o

    /opt/riscv-ilp32/bin/riscv32-unknown-elf-g++ -g -std=c++20 -march=rv32gc -mabi=ilp32 \
        -nostdlib \
        /opt/riscv-ilp32/lib/gcc/riscv32-unknown-elf/14.2.0/crtbegin.o \
        test.o \
        start.o \
        syscalls.o \
        "${BASE_BUILD_DIR}"/lib/libbitcoin_consensus.a \
        "${BASE_BUILD_DIR}"/lib/libbitcoin_crypto.a \
        "${BASE_BUILD_DIR}"/src/secp256k1/lib/libsecp256k1.a \
        /opt/riscv-ilp32/riscv32-unknown-elf/lib/libstdc++.a \
        /opt/newlib/riscv32-unknown-elf/lib/libc.a \
        /opt/newlib/riscv32-unknown-elf/lib/libm.a \
        /opt/riscv-ilp32/lib/gcc/riscv32-unknown-elf/14.2.0/libgcc.a \
        -o test.elf
fi

if [ -n "$USE_VALGRIND" ]; then
  "${BASE_ROOT_DIR}/ci/test/wrap-valgrind.sh"
fi

if [ "$RUN_CHECK_DEPS" = "true" ]; then
  "${BASE_ROOT_DIR}/contrib/devtools/check-deps.sh" "${BASE_BUILD_DIR}"
fi

if [ "$RUN_UNIT_TESTS" = "true" ]; then
  DIR_UNIT_TEST_DATA="${DIR_UNIT_TEST_DATA}" \
  LD_LIBRARY_PATH="${DEPENDS_DIR}/${HOST}/lib" \
  CTEST_OUTPUT_ON_FAILURE=ON \
  ctest --test-dir "${BASE_BUILD_DIR}" \
    --stop-on-failure \
    "${MAKEJOBS}" \
    --timeout $(( TEST_RUNNER_TIMEOUT_FACTOR * 60 ))
fi

if [ "$RUN_FUNCTIONAL_TESTS" = "true" ]; then
  # parses TEST_RUNNER_EXTRA as an array which allows for multiple arguments such as TEST_RUNNER_EXTRA='--exclude "rpc_bind.py --ipv6"'
  eval "TEST_RUNNER_EXTRA=($TEST_RUNNER_EXTRA)"
  LD_LIBRARY_PATH="${DEPENDS_DIR}/${HOST}/lib" \
  "${BASE_BUILD_DIR}/test/functional/test_runner.py" \
    --ci "${MAKEJOBS}" \
    --tmpdirprefix "${BASE_SCRATCH_DIR}/test_runner/" \
    --ansi \
    --combinedlogslen=99999999 \
    --timeout-factor="${TEST_RUNNER_TIMEOUT_FACTOR}" \
    "${TEST_RUNNER_EXTRA[@]}" \
    --quiet \
    --failfast
fi

if [ "${RUN_TIDY}" = "true" ]; then
  cmake -B /tidy-build -DLLVM_DIR=/usr/lib/llvm-"${TIDY_LLVM_V}"/cmake -DCMAKE_BUILD_TYPE=Release -S "${BASE_ROOT_DIR}"/contrib/devtools/bitcoin-tidy
  cmake --build /tidy-build "$MAKEJOBS"
  cmake --build /tidy-build --target bitcoin-tidy-tests "$MAKEJOBS"

  set -eo pipefail
  # Filter out:
  # * qt qrc and moc generated files
  jq 'map(select(.file | test("src/qt/.*_autogen/.*\\.cpp$") | not))' "${BASE_BUILD_DIR}/compile_commands.json" > tmp.json
  mv tmp.json "${BASE_BUILD_DIR}/compile_commands.json"

  cd "${BASE_BUILD_DIR}/src/"
  if ! ( run-clang-tidy-"${TIDY_LLVM_V}" -quiet -load="/tidy-build/libbitcoin-tidy.so" "${MAKEJOBS}" | tee tmp.tidy-out.txt ); then
    grep -C5 "error: " tmp.tidy-out.txt
    echo "^^^ ⚠️ Failure generated from clang-tidy"
    false
  fi

  cd "${BASE_ROOT_DIR}"
  python3 "/include-what-you-use/iwyu_tool.py" \
           -p "${BASE_BUILD_DIR}" "${MAKEJOBS}" \
           -- -Xiwyu --cxx17ns -Xiwyu --mapping_file="${BASE_ROOT_DIR}/contrib/devtools/iwyu/bitcoin.core.imp" \
           -Xiwyu --max_line_length=160 \
           2>&1 | tee /tmp/iwyu_ci.out
  cd "${BASE_ROOT_DIR}/src"
  python3 "/include-what-you-use/fix_includes.py" --nosafe_headers < /tmp/iwyu_ci.out
  git --no-pager diff
fi

if [ "$RUN_FUZZ_TESTS" = "true" ]; then
  # shellcheck disable=SC2086
  LD_LIBRARY_PATH="${DEPENDS_DIR}/${HOST}/lib" \
  "${BASE_BUILD_DIR}/test/fuzz/test_runner.py" \
    ${FUZZ_TESTS_CONFIG} \
    "${MAKEJOBS}" \
    -l DEBUG \
    "${DIR_FUZZ_IN}" \
    --empty_min_time=60
fi
