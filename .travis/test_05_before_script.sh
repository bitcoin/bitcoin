#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

DOCKER_EXEC echo \> \$HOME/.bitcoin  # Make sure default datadir does not exist and is never read by creating a dummy file
OSX_SDK_BASENAME="Xcode-${XCODE_VERSION}-${XCODE_BUILD_ID}-extracted-SDK-with-libcxx-headers.tar.gz"
OSX_SDK_PATH="depends/sdk-sources/${OSX_SDK_BASENAME}"

mkdir -p depends/SDKs depends/sdk-sources

if [ -n "$XCODE_VERSION" ] && [ ! -f "$OSX_SDK_PATH" ]; then
  curl --location --fail "${SDK_URL}/${OSX_SDK_BASENAME}" -o "$OSX_SDK_PATH"
fi
if [ -n "$XCODE_VERSION" ] && [ -f "$OSX_SDK_PATH" ]; then
  DOCKER_EXEC tar -C "depends/SDKs" -xf "$OSX_SDK_PATH"
fi
if [[ $HOST = *-mingw32 ]]; then
  DOCKER_EXEC update-alternatives --set $HOST-g++ \$\(which $HOST-g++-posix\)
fi
if [ -z "$NO_DEPENDS" ]; then
  DOCKER_EXEC CONFIG_SHELL= make $MAKEJOBS -C depends HOST=$HOST $DEP_OPTS
fi
