#!/bin/sh -e
# Written by Luke Dashjr in 2012
# This program is released under the terms of the Creative Commons "CC0 1.0 Universal" license and/or copyright waiver.

if test -z "$srcdir"; then
	srcdir=`dirname "$0"`
	if test -z "$srcdir"; then
		srcdir=.
	fi
fi
autoreconf --force --install --verbose "$srcdir"
