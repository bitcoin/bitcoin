#!/usr/bin/env bash
#
# This script is executed inside the builder image

export LC_ALL=C.UTF-8

set -e

source ./ci/matrix.sh

unset CC; unset CXX
unset DISPLAY

export CCACHE_COMPRESS=${CCACHE_COMPRESS:-1}
export CCACHE_SIZE=${CCACHE_SIZE:-400M}

if [ "$PULL_REQUEST" != "false" ]; then test/lint/commit-script-check.sh $COMMIT_RANGE; fi

if [ "$CHECK_DOC" = 1 ]; then
    # TODO: Verify subtrees
    #test/lint/git-subtree-check.sh src/crypto/ctaes
    #test/lint/git-subtree-check.sh src/secp256k1
    #test/lint/git-subtree-check.sh src/univalue
    #test/lint/git-subtree-check.sh src/leveldb
    # TODO: Check docs (re-enable after all Bitcoin PRs have been merged and docs fully fixed)
    #test/lint/check-doc.py
    # Check rpc consistency
    test/lint/check-rpc-mappings.py .
    # Run all linters
    test/lint/lint-all.sh
fi

ccache --max-size=$CCACHE_SIZE

if [ -n "$USE_SHELL" ]; then
  export CONFIG_SHELL="$USE_SHELL"
fi

BITCOIN_CONFIG_ALL="--disable-dependency-tracking --prefix=$BUILD_DIR/depends/$HOST --bindir=$OUT_DIR/bin --libdir=$OUT_DIR/lib"

( test -n "$USE_SHELL" && eval '"$USE_SHELL" -c "./autogen.sh"' ) || ./autogen.sh

rm -rf build-ci
mkdir build-ci
cd build-ci

../configure --cache-file=config.cache $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG || ( cat config.log && false)
make distdir VERSION=$BUILD_TARGET

cd dashcore-$BUILD_TARGET
./configure --cache-file=../config.cache $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG || ( cat config.log && false)

make $MAKEJOBS $GOAL || ( echo "Build failure. Verbose build follows." && make $GOAL V=1 ; false )
