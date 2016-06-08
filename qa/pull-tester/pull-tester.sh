#!/bin/sh
# Copyright (c) 2013 The Crowncoin developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Helper script for pull-tester.
#Param 1: path to crowncoin srcroot
#Param ...: arguments for build-test.sh

if [ $# -lt 1 ]; then
  echo "usage: $0 [crowncoin srcroot] build-test arguments..."
fi

killall -q crowncoin-cli
killall -q crowncoind

cd $1
shift

./autogen.sh
./configure
./qa/pull-tester/build-tests.sh "$@"
