#!/bin/sh
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
