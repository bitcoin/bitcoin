#!/bin/sh
# Copyright (c) 2013-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set -e
srcdir="$(dirname $0)"
cd "$srcdir"

if [ -z ${LIBTOOLIZE} ]; then
  if ! LIBTOOLIZE="`which glibtoolize 2>/dev/null`" || [ ! -x "$LIBTOOLIZE" ]; then
    if ! LIBTOOLIZE="`which libtoolize 2>/dev/null`" || [ ! -x "$LIBTOOLIZE" ]; then
      echo "glibtoolize/libtoolize not found (usually packaged in libtool-bin/libtool)" \
      && exit 1
    fi
  fi
fi
export LIBTOOLIZE

which autoreconf >/dev/null || \
  (echo "configuration failed, please install autoconf first" && exit 1)
autoreconf --install --force --warnings=all
