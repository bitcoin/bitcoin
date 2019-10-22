#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

#cd command is misunderstanded by cd Travis built-in command in some macOS
#lcd='cd'
#if [[ "$OSTYPE" == "darwin"* ]]; then
#  lcd='type cd'
#fi

#Avoid repeating this block several places in code
safe_cd()
{
  command +V cd
  type cd
  set +o errexit
  #$lcd $1 || (echo $2; exit 1)
  set -o errexit
}

BITCOIN_CONFIG_ALL="--disable-dependency-tracking --prefix=$BASE_BUILD_DIR/depends/$HOST --bindir=$BASE_OUTDIR/bin --libdir=$BASE_OUTDIR/lib"
if [ -z "$NO_DEPENDS" ]; then
  DOCKER_EXEC ccache --max-size=$CCACHE_SIZE
fi

BEGIN_FOLD autogen
if [ -n "$CONFIG_SHELL" ]; then
  DOCKER_EXEC "$CONFIG_SHELL" -c "./autogen.sh"
else
  DOCKER_EXEC ./autogen.sh
fi
END_FOLD

mkdir -p build

# Temporarily disable errexit, because Travis macOS fails without error message
safe_cd build "could not enter build directory"

BEGIN_FOLD configure
DOCKER_EXEC ../configure --cache-file=config.cache $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG || ( cat config.log && false)
END_FOLD

BEGIN_FOLD distdir
DOCKER_EXEC make distdir VERSION=$HOST
END_FOLD

safe_cd "bitcoin-$HOST" "could not enter distdir bitcoin-$HOST"

BEGIN_FOLD configure
DOCKER_EXEC ./configure --cache-file=../config.cache $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG || ( cat config.log && false)
END_FOLD

set -o errtrace
trap 'DOCKER_EXEC "cat ${BASE_BUILD_DIR}/sanitizer-output/* 2> /dev/null"' ERR

BEGIN_FOLD build
DOCKER_EXEC make $MAKEJOBS $GOAL || ( echo "Build failure. Verbose build follows." && DOCKER_EXEC make $GOAL V=1 ; false )
END_FOLD

safe_cd ${BASE_BUILD_DIR} "could not enter travis build dir $BASE_BUILD_DIR"
