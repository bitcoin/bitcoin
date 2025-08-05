#!/usr/bin/env bash
# Copyright (c) 2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -eo pipefail

# Warning: This script does not generate the compilation database these linters rely
#          only on nor do they set the requisite build parameters. Make sure you do
#          that *before* running this script.

cd "${BASE_ROOT_DIR}/build-ci/dashcore-${BUILD_TARGET}/src"
if ! ( run-clang-tidy -quiet "${MAKEJOBS}" | tee tmp.tidy-out.txt ); then
  grep -C5 "error: " tmp.tidy-out.txt
  echo "^^^ ⚠️ Failure generated from clang-tidy"
  false
fi

cd "${BASE_ROOT_DIR}/build-ci/dashcore-${BUILD_TARGET}"
iwyu_tool.py \
  "src/compat" \
  "src/init" \
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
  "src/util/strencodings.cpp" \
  "src/util/syserror.cpp" \
  "src/util/url.cpp" \
  -p . "${MAKEJOBS}" \
  -- -Xiwyu --cxx17ns -Xiwyu --mapping_file="${BASE_ROOT_DIR}/contrib/devtools/iwyu/bitcoin.core.imp" \
  2>&1 | tee "/tmp/iwyu_ci.out"

cd "${BASE_ROOT_DIR}/build-ci/dashcore-${BUILD_TARGET}/src"
fix_includes.py --nosafe_headers < /tmp/iwyu_ci.out
git --no-pager diff
