#!/bin/bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Build previous releases.

CONFIG_FLAGS=""
FUNCTIONAL_TESTS=0
DELETE_EXISTING=0
USE_DEPENDS=0
RELEASE_PATH_POSTFIX=""
CONFIG_FLAGS=""

while getopts ":hfrdb" opt; do
  case $opt in
    h)
      echo "Usage: .previous_release.sh [options] tag1 tag2"
      echo "  options:"
      echo "  -h   Print this message"
      echo "  -f   Configure for functional tests"
      echo "  -r   Remove existing directory"
      echo "  -d   Use depends"
      echo "  -b   configure --with-incompatible-bdb"
      exit 0
      ;;
    f)
      FUNCTIONAL_TESTS=0
      CONFIG_FLAGS="$CONFIG_FLAGS --without-gui --disable-tests --disable-bench"
      ;;
    r)
      DELETE_EXISTING=1
      ;;
    d)
      USE_DEPENDS=1
      ;;
    b)
      CONFIG_FLAGS="$CONFIG_FLAGS --with-incompatible-bdb"
      RELEASE_PATH_POSTFIX="$RELEASE_PATH_POSTFIX-incompatible-bdb"
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
  esac
done

shift $((OPTIND-1))

if [ -z "$1" ]; then
  echo "Specify release tag(s), e.g.: .previous_release v0.15.1"
  exit 1
fi

for tag in "$@"
do
  RELEASE_PATH="build/releases/$tag$RELEASE_PATH_POSTFIX"
  if [ "$DELETE_EXISTING" -eq "1" ]; then
    rm -rf $RELEASE_PATH
  fi

  if [ ! -d "$RELEASE_PATH" ]; then
    if [ -f $(git rev-parse --git-dir)/shallow ]; then
      git fetch --unshallow --tags
    fi

    if [ -z $(git tag -l "$tag") ]; then
      echo "Tag $tag not found"
      exit 1
    fi

    git clone . $RELEASE_PATH
    pushd $RELEASE_PATH
    git checkout $tag

    if [ "$USE_DEPENDS" -eq "1" ]; then
      pushd depends
      if [ "$FUNCTIONAL_TESTS" -eq "1" ]; then
        make NO_QT=1
      else
        make
      fi
      HOST="${HOST:-$(./config.guess)}"
      popd
      CONFIG_FLAGS="--prefix=$PWD/depends/$HOST $CONFIG_FLAGS"
    fi

    ./autogen.sh
    ./configure $CONFIG_FLAGS
    make
    popd
  fi
done
