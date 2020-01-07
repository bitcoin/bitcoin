#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

# Make sure default datadir does not exist and is never read by creating a dummy file
if [ "$TRAVIS_OS_NAME" == "osx" ]; then
  echo > $HOME/Library/Application\ Support/Bitcoin
else
  DOCKER_EXEC echo \> \$HOME/.bitcoin
fi

DOCKER_EXEC mkdir -p ${DEPENDS_DIR}/SDKs ${DEPENDS_DIR}/sdk-sources

if [ -n "$OSX_SDK" ] && [ ! -f ${DEPENDS_DIR}/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz ]; then
  curl --location --fail $SDK_URL/MacOSX${OSX_SDK}.sdk.tar.gz -o ${DEPENDS_DIR}/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz
fi
if [ -n "$OSX_SDK" ] && [ -f ${DEPENDS_DIR}/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz ]; then
  DOCKER_EXEC tar -C ${DEPENDS_DIR}/SDKs -xf ${DEPENDS_DIR}/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz
fi
if [[ $HOST = *-mingw32 ]]; then
  DOCKER_EXEC update-alternatives --set $HOST-g++ \$\(which $HOST-g++-posix\)
fi
if [ -z "$NO_DEPENDS" ]; then
  DOCKER_EXEC CONFIG_SHELL= make $MAKEJOBS -C depends HOST=$HOST $DEP_OPTS
fi
