#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Build previous releases.

export LC_ALL=C

CONFIG_FLAGS=""
FUNCTIONAL_TESTS=0
DELETE_EXISTING=0
USE_DEPENDS=0
DOWNLOAD_BINARY=0
CONFIG_FLAGS=""
TARGET="releases"

while getopts ":hfrdbt:" opt; do
  case $opt in
    h)
      echo "Usage: .previous_release.sh [options] tag1 tag2"
      echo "  options:"
      echo "  -h   Print this message"
      echo "  -f   Configure for functional tests"
      echo "  -r   Remove existing directory"
      echo "  -d   Use depends"
      echo "  -b   Download release binary"
      echo "  -t   Target directory (default: releases)"
      exit 0
      ;;
    f)
      FUNCTIONAL_TESTS=1
      CONFIG_FLAGS="$CONFIG_FLAGS --without-gui --disable-tests --disable-bench"
      ;;
    r)
      DELETE_EXISTING=1
      ;;
    d)
      USE_DEPENDS=1
      ;;
    b)
      DOWNLOAD_BINARY=1
      ;;
    t)
      TARGET=$OPTARG
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

if [ ! -d "$TARGET" ]; then
  mkdir -p $TARGET
fi

if [ "$DOWNLOAD_BINARY" -eq "1" ]; then
  HOST="${HOST:-$(./depends/config.guess)}"
  case "$HOST" in
    x86_64-*-linux*)
      PLATFORM=x86_64-linux-gnu
      ;;
    x86_64-apple-darwin*)
      PLATFORM=osx64
      ;;
    *)
      echo "Not sure which binary to download for $HOST."
      exit 1
      ;;
  esac
fi

echo "Releases directory: $TARGET"
pushd "$TARGET" || exit 1
{
  for tag in "$@"
  do
    if [ "$DELETE_EXISTING" -eq "1" ]; then
      if [ -d "$tag" ]; then
        rm -r "$tag"
      fi
    fi

    if [ "$DOWNLOAD_BINARY" -eq "0" ]; then

      if [ ! -d "$tag" ]; then
        if [ -z $(git tag -l "$tag") ]; then
          echo "Tag $tag not found"
          exit 1
        fi

        git clone https://github.com/bitcoin/bitcoin "$tag"
        pushd "$tag" || exit 1
        {
          git checkout "$tag"
          if [ "$USE_DEPENDS" -eq "1" ]; then
            pushd depends || exit 1
            {
              if [ "$FUNCTIONAL_TESTS" -eq "1" ]; then
                make NO_QT=1
              else
                make
              fi
              HOST="${HOST:-$(./config.guess)}"
            }
            popd || exit 1
            CONFIG_FLAGS="--prefix=$PWD/depends/$HOST $CONFIG_FLAGS"
          fi
          ./autogen.sh
          ./configure $CONFIG_FLAGS
          make
          # Move binaries, so they're in the same place as in the release download:
          mkdir bin
          mv src/bitcoind src/bitcoin-cli src/bitcoin-tx bin
          if [ "$FUNCTIONAL_TESTS" -eq "0" ]; then
            mv src/qt/bitcoin-qt bin
          fi
        }
        popd || exit 1
      fi
    else
      if [ -d "$tag" ]; then
        echo "Using cached $tag"
      else
        mkdir "$tag"
        if [[ "$tag" =~ v(.*)(rc[0-9]+)$ ]]; then
            BIN_PATH="bin/bitcoin-core-${BASH_REMATCH[1]}/test.${BASH_REMATCH[2]}"
        else
            BIN_PATH="bin/bitcoin-core-${tag:1}"
        fi
        URL="https://bitcoin.org/$BIN_PATH/bitcoin-${tag:1}-$PLATFORM.tar.gz"
        echo "Fetching: $URL"
        if ! curl -O -f $URL; then
            echo "Download failed."
            exit 1
        fi
        tar -zxf "bitcoin-${tag:1}-$PLATFORM.tar.gz" -C "$tag" --strip-components=1 "bitcoin-${tag:1}"
        rm "bitcoin-${tag:1}-$PLATFORM.tar.gz"
      fi
    fi
  done
}
popd || exit 1
