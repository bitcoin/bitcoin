#!/usr/bin/env bash
# Copyright (c) 2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -eo pipefail

# Warning: This script does not generate the compilation database these linters rely
#          only on nor do they set the requisite build parameters. Make sure you do
#          that *before* running this script.

# Verify clang-tidy is available
if ! command -v clang-tidy >/dev/null 2>&1; then
  echo "Error: clang-tidy not found in PATH"
  exit 1
fi

# Setup ctcache for clang-tidy result caching
export CTCACHE_DIR="/cache/ctcache"
export CTCACHE_SAVE_OUTPUT=1
CTCACHE_CLANG_TIDY=$(which clang-tidy)
export CTCACHE_CLANG_TIDY
mkdir -p "${CTCACHE_DIR}"

CLANG_TIDY_CACHE="/usr/local/bin/clang-tidy-cache"
CLANG_TIDY_CACHE_PY="/usr/local/bin/src/ctcache/clang_tidy_cache.py"

# Verify ctcache is installed
if [ ! -x "${CLANG_TIDY_CACHE}" ]; then
  echo "Error: ctcache binary not found at ${CLANG_TIDY_CACHE}"
  exit 1
fi
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
CTCACHE_MAXSIZE_MB=50
CACHE_SIZE_MB=$(du -sm "${CTCACHE_DIR}" 2>/dev/null | cut -f1)
if [ "${CACHE_SIZE_MB}" -gt "${CTCACHE_MAXSIZE_MB}" ]; then
  echo "Cache size ${CACHE_SIZE_MB}MB exceeds limit ${CTCACHE_MAXSIZE_MB}MB, cleaning old entries..."
  # Remove files older than 7 days, or if still too large, oldest 20% of files
  find "${CTCACHE_DIR}" -type f -mtime +7 -delete 2>/dev/null || true
  CACHE_SIZE_MB=$(du -sm "${CTCACHE_DIR}" 2>/dev/null | cut -f1)
  if [ "${CACHE_SIZE_MB}" -gt "${CTCACHE_MAXSIZE_MB}" ]; then
    FILE_COUNT=$(find "${CTCACHE_DIR}" -type f | wc -l)
    REMOVE_COUNT=$((FILE_COUNT / 5))
    find "${CTCACHE_DIR}" -type f -printf '%T+ %p\0' | sort -z | head -z -n "${REMOVE_COUNT}" | cut -z -d' ' -f2- | xargs -0 rm -f 2>/dev/null || true
    echo "Removed ${REMOVE_COUNT} oldest cache entries"
  fi
  echo "Cache size after cleanup: $(du -sh "${CTCACHE_DIR}" 2>/dev/null | cut -f1)"
fi
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
  "src/threadinterrupt.cpp" \
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
  "src/util/url.cpp" \
  "src/zmq" \
  -p . "${MAKEJOBS}" \
  -- -Xiwyu --cxx17ns -Xiwyu --mapping_file="${BASE_ROOT_DIR}/contrib/devtools/iwyu/bitcoin.core.imp" \
  -Xiwyu --max_line_length=160 \
  2>&1 | tee "/tmp/iwyu_ci.out"

cd "${BASE_ROOT_DIR}/build-ci/dashcore-${BUILD_TARGET}/src"
fix_includes.py --nosafe_headers < /tmp/iwyu_ci.out
git --no-pager diff
