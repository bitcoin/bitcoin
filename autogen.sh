#!/bin/sh
set -e
if which libtool >/dev/null; then
    echo
else
    echo Libtool is Missing. Bitcoin Requires Libtool To Compile.
fi
srcdir="$(dirname $0)"
cd "$srcdir"
if [ -z ${LIBTOOLIZE} ] && GLIBTOOLIZE="`which glibtoolize 2>/dev/null`"; then
  export LIBTOOLIZE="${GLIBTOOLIZE}"
fi
autoreconf --install --force
