#!/usr/bin/env bash
#
# This script is executed inside the builder image

export LC_ALL=C.UTF-8

set -e

source ./ci/matrix.sh

unset CC; unset CXX
unset DISPLAY

mkdir -p $CACHE_DIR/depends
mkdir -p $CACHE_DIR/sdk-sources

ln -s $CACHE_DIR/depends depends/built
ln -s $CACHE_DIR/sdk-sources depends/sdk-sources

mkdir -p depends/SDKs

if [ -n "$XCODE_VERSION" ]; then
  OSX_SDK_BASENAME="Xcode-${XCODE_VERSION}-${XCODE_BUILD_ID}-extracted-SDK-with-libcxx-headers.tar.gz"
  OSX_SDK_PATH="depends/sdk-sources/${OSX_SDK_BASENAME}"
  if [ ! -f "$OSX_SDK_PATH" ]; then
    curl --location --fail "${SDK_URL}/${OSX_SDK_BASENAME}" -o "$OSX_SDK_PATH"
  fi
  if [ -f "$OSX_SDK_PATH" ]; then
    tar -C depends/SDKs -xf "$OSX_SDK_PATH"
  fi
fi

make $MAKEJOBS -C depends HOST=$HOST $DEP_OPTS
