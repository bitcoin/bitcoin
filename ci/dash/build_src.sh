#!/usr/bin/env bash
# Copyright (c) 2021-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# This script is executed inside the builder image

export LC_ALL=C.UTF-8

set -e

source ./ci/dash/matrix.sh

unset CC CXX DISPLAY;

if [ "$PULL_REQUEST" != "false" ]; then test/lint/commit-script-check.sh "$COMMIT_RANGE"; fi

if [ "$CHECK_DOC" = 1 ]; then
    # TODO: Verify subtrees
    #test/lint/git-subtree-check.sh src/crypto/ctaes
    #test/lint/git-subtree-check.sh src/secp256k1
    #test/lint/git-subtree-check.sh src/univalue
    #test/lint/git-subtree-check.sh src/leveldb
    # TODO: Check docs (re-enable after all Bitcoin PRs have been merged and docs fully fixed)
    #test/lint/check-doc.py
    # Run all linters
    test/lint/all-lint.py
fi

ccache --zero-stats

if [ -n "$CONFIG_SHELL" ]; then
  export CONFIG_SHELL="$CONFIG_SHELL"
fi

BITCOIN_CONFIG_ALL="--disable-dependency-tracking --prefix=$DEPENDS_DIR/$HOST --bindir=$BASE_OUTDIR/bin --libdir=$BASE_OUTDIR/lib"
if [ -z "$NO_WERROR" ]; then
  BITCOIN_CONFIG_ALL="${BITCOIN_CONFIG_ALL} --enable-werror"
fi

( test -n "$CONFIG_SHELL" && eval '"$CONFIG_SHELL" -c "./autogen.sh"' ) || ./autogen.sh

rm -rf build-ci
mkdir build-ci
cd build-ci

bash -c "../configure $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG" || ( cat config.log && false)
make distdir VERSION="$BUILD_TARGET"

cd "dashcore-$BUILD_TARGET"
bash -c "./configure $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG" || ( cat config.log && false)

# This step influences compilation and therefore will always be a part of the
# compile step
if [ "${RUN_TIDY}" = "true" ]; then
  MAYBE_BEAR="bear --config src/.bear-tidy-config"
  MAYBE_TOKEN="--"
fi

bash -c "${MAYBE_BEAR} ${MAYBE_TOKEN} make ${MAKEJOBS} ${GOAL}" || ( echo "Build failure. Verbose build follows." && make "$GOAL" V=1 ; false )

ccache --version | head -n 1 && ccache --show-stats

if [ -n "$USE_VALGRIND" ]; then
    echo "valgrind in USE!"
    "${BASE_ROOT_DIR}/ci/test/wrap-valgrind.sh"
fi

# GitHub Actions can segment a job into steps, linting is a separate step
# so Actions runners will perform this step separately.
if [ "${RUN_TIDY}" = "true" ] && [ "${GITHUB_ACTIONS}" != "true" ]; then
  "${BASE_ROOT_DIR}/ci/dash/lint-tidy.sh"
fi

if [ "$RUN_SECURITY_TESTS" = "true" ]; then
  make test-security-check
fi
