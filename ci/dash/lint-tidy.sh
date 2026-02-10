#!/usr/bin/env bash
# Copyright (c) 2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -eo pipefail

# Warning: This script does not generate the compilation database these linters rely
#          only on nor do they set the requisite build parameters. Make sure you do
#          that *before* running this script.

# Cleanup old ctcache entries to keep cache size under limit
cleanup_ctcache() {
  local cache_dir=$1
  local max_size_mb=$2
  local cache_size_mb

  cache_size_mb=$(du -sm "${cache_dir}" 2>/dev/null | cut -f1)
  if [ "${cache_size_mb}" -gt "${max_size_mb}" ]; then
    echo "Cache size ${cache_size_mb}MB exceeds limit ${max_size_mb}MB, cleaning old entries..."
    # Remove files older than 7 days, or if still too large, oldest 20% of files
    find "${cache_dir}" -type f -mtime +7 -delete 2>/dev/null || true
    cache_size_mb=$(du -sm "${cache_dir}" 2>/dev/null | cut -f1)
    if [ "${cache_size_mb}" -gt "${max_size_mb}" ]; then
      local file_count remove_count
      file_count=$(find "${cache_dir}" -type f | wc -l)
      remove_count=$((file_count / 5))
      find "${cache_dir}" -type f -printf '%T+ %p\0' | sort -z | head -z -n "${remove_count}" | cut -z -d' ' -f2- | xargs -0 rm -f 2>/dev/null || true
      echo "Attempted to remove ${remove_count} oldest cache entries"
    fi
    echo "Cache size after cleanup: $(du -sh "${cache_dir}" 2>/dev/null | cut -f1)"
  fi
}

# Verify clang-tidy is available
CLANG_TIDY_BIN=$(command -v clang-tidy)
if [ -z "${CLANG_TIDY_BIN}" ]; then
  echo "Error: clang-tidy not found in PATH"
  exit 1
fi

# Setup ctcache for clang-tidy result caching
export CTCACHE_SAVE_OUTPUT=1
export CTCACHE_CLANG_TIDY="${CLANG_TIDY_BIN}"
mkdir -p "${CTCACHE_DIR}"

CLANG_TIDY_CACHE=$(command -v clang-tidy-cache)

# Verify ctcache is installed
if [ -z "${CLANG_TIDY_CACHE}" ]; then
  echo "Error: clang-tidy-cache not found in PATH"
  exit 1
fi

# Derive Python script path from wrapper location (matches wrapper's own logic)
CLANG_TIDY_CACHE_PY="$(dirname "${CLANG_TIDY_CACHE}")/src/ctcache/clang_tidy_cache.py"
if [ ! -f "${CLANG_TIDY_CACHE_PY}" ]; then
  echo "Error: ctcache Python script not found at ${CLANG_TIDY_CACHE_PY}"
  exit 1
fi

# Zero stats before run to get accurate statistics for this run only
python3 "${CLANG_TIDY_CACHE_PY}" --zero-stats 2>&1 || true

cd "${BASE_ROOT_DIR}/build-ci/dashcore-${BUILD_TARGET}/src"

if ! ( run-clang-tidy -clang-tidy-binary="${CLANG_TIDY_CACHE}" -quiet "${MAKEJOBS}" | tee tmp.tidy-out.txt ); then
  grep -C5 "error: " tmp.tidy-out.txt
  echo "^^^ ⚠️ Failure generated from clang-tidy"
  false
fi

# Show ctcache statistics and manage cache size
echo "=== ctcache statistics ==="
du -sh "${CTCACHE_DIR}" 2>/dev/null || echo "Cache directory not found"
python3 "${CLANG_TIDY_CACHE_PY}" --show-stats 2>&1 || true

# Limit cache size (ctcache has no built-in size management)
cleanup_ctcache "${CTCACHE_DIR}" "${CTCACHE_MAXSIZE_MB}"
echo "=========================="

cd "${BASE_ROOT_DIR}/build-ci/dashcore-${BUILD_TARGET}"
iwyu_tool.py \
  "src/compat" \
  "src/dbwrapper.cpp" \
  "src/init" \
  "src/node/chainstate.cpp" \
  "src/node/minisketchwrapper.cpp" \
  "src/policy/feerate.cpp" \
  "src/policy/packages.cpp" \
  "src/policy/settings.cpp" \
  "src/primitives/transaction.cpp" \
  "src/random.cpp" \
  "src/rpc/fees.cpp" \
  "src/rpc/signmessage.cpp" \
  "src/test/fuzz/txorphan.cpp" \
  "src/util/bip32.cpp" \
  "src/util/bytevectorhash.cpp" \
  "src/util/check.cpp" \
  "src/util/error.cpp" \
  "src/util/getuniquepath.cpp" \
  "src/util/hasher.cpp" \
  "src/util/message.cpp" \
  "src/util/moneystr.cpp" \
  "src/util/serfloat.cpp" \
  "src/util/spanparsing.cpp" \
  "src/util/string.cpp" \
  "src/util/strencodings.cpp" \
  "src/util/syserror.cpp" \
  "src/util/threadinterrupt.cpp" \
  "src/util/url.cpp" \
  "src/zmq" \
  -p . "${MAKEJOBS}" \
  -- -Xiwyu --cxx17ns -Xiwyu --mapping_file="${BASE_ROOT_DIR}/contrib/devtools/iwyu/bitcoin.core.imp" \
  -Xiwyu --max_line_length=160 \
  2>&1 | tee "/tmp/iwyu_ci.out"

cd "${BASE_ROOT_DIR}/build-ci/dashcore-${BUILD_TARGET}/src"
fix_includes.py --nosafe_headers < /tmp/iwyu_ci.out
git --no-pager diff
