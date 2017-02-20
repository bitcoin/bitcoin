#!/bin/sh -
#
# $Id$
#
# Check to make sure the queue macros pass our tests.

[ -f ../libdb.a ] || (cd .. && make libdb.a) || {
	echo 'FAIL: unable to find or build libdb.a'
	exit 1
}

if cc -g -Wall -I../../dbinc q.c ../libdb.a -o t; then
	:
else
	echo "FAIL: unable to compile test program q.c"
	exit 1
fi

if ./t; then
	:
else
	echo "FAIL: test program failed"
	exit 1
fi

exit 0
