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

while getopts ":hfrd" opt; do
  case $opt in
    h)
      echo "Usage: .previous_release.sh [options] tag1 tag2"
      echo "  options:"
      echo "  -h   Print this message"
      echo "  -f   Configure for functional tests"
      echo "  -r   Remove existing directory"
      echo "  -d   Use depends"
      exit 0
      ;;
    f)
      FUNCTIONAL_TESTS=1
      ;;
    r)
      DELETE_EXISTING=1
      ;;
    d)
      USE_DEPENDS=1
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
  if [ "$DELETE_EXISTING" -eq "1" ]; then
    rm -rf build/releases/$tag
  fi

  if [ "$FUNCTIONAL_TESTS" -eq "1" ]; then
    CONFIG_FLAGS="--without-gui --disable-tests --disable-bench"
  fi

  if [ ! -d "build/releases/$tag" ]; then
    if [ -f $(git rev-parse --git-dir)/shallow ]; then
      git fetch --unshallow --tags
    fi

    if [ -z $(git tag -l "$tag") ]; then
      echo "Tag $tag not found"
      exit 1
    fi

    git clone . build/releases/$tag
    pushd build/releases/$tag
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
