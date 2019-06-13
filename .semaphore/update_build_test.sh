#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

sudo apt-get update
sudo apt-get install --no-install-recommends --no-upgrade -qq $PACKAGES

./autogen.sh
if [ "$TARGET_HOST" != "system" ]; then
	cd depends
	ls -la
	cache list
	cache restore $TARGET_HOST-built
	ls -la
	make HOST=$TARGET_HOST -j4 V=1
	ls -la
	cache store $TARGET_HOST-built built/$TARGET_HOST
	cache list
	cd ..
fi

if [[ $TARGET_HOST = *-mingw32 ]]; then
  sudo update-alternatives --set $TARGET_HOST-g++ $(which $TARGET_HOST-g++-posix)
fi

if [ "$TARGET_HOST" = "system" ]; then
	./configure $BITCOIN_CONFIG
else
	./configure --prefix=`pwd`/depends/$TARGET_HOST $BITCOIN_CONFIG
fi
make -j4

if [ "$RUN_UNIT_TESTS" = "true" ]; then
	make check VERBOSE=1
fi

if [ "$RUN_FUNCTIONAL_TESTS" = "true" ]; then
	./test/functional/test_runner.py --ci --combinedlogslen=4000 --quiet --failfast
fi
