#!/bin/sh
# Helper script for pull-tester.
#Param 1: path to bitcoin srcroot
#Param ...: arguments for build-test.sh

if [ $# -lt 1 ]; then
  echo "usage: $0 [bitcoin srcroot] build-test arguments..."
fi

cd $1
shift

./autogen.sh
./configure
./qa/pull-tester/build-tests.sh "$@"
