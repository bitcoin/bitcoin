#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

DOCKER_EXEC echo \> \$HOME/.bitcoin  # Make sure default datadir does not exist and is never read by creating a dummy file

mkdir -p depends/SDKs depends/sdk-sources

if [ -n "$OSX_SDK" ] && [ ! -f depends/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz ]; then
  curl --location --fail $SDK_URL/MacOSX${OSX_SDK}.sdk.tar.gz -o depends/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz
fi
if [ -n "$OSX_SDK" ] && [ -f depends/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz ]; then
  tar -C depends/SDKs -xf depends/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz
fi
if [[ $HOST = *-mingw32 ]]; then
  DOCKER_EXEC update-alternatives --set $HOST-g++ \$\(which $HOST-g++-posix\)
fi
if [ "$BITCOIND" = "bitcoin-qt" ]; then
  export DISPLAY=:99.0
  # From https://docs.travis-ci.com/user/gui-and-headless-browsers/#using-xvfb-directly
  DOCKER_EXEC /sbin/start-stop-daemon --start --pidfile /tmp/custom_xvfb_99.pid --make-pidfile --background --exec /usr/bin/Xvfb -- :99 -ac -screen 0 1280x1024x24;
fi
if [ -z "$NO_DEPENDS" ]; then
  DOCKER_EXEC CONFIG_SHELL= make $MAKEJOBS -C depends HOST=$HOST $DEP_OPTS
fi
