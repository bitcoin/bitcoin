#!/bin/sh
# Copyright (c) 2013-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
set -e

assert_deps() {
  for dep; do
    if ! command -v "$dep" > /dev/null; then
      echo "configuration failed, please install $dep first"
      return 1
    fi
  done
}

# libtoolize is glibtoolize on macOS
if [ -z "${LIBTOOLIZE}" ] && GLIBTOOLIZE="$(command -v glibtoolize)"; then
  LIBTOOLIZE="${GLIBTOOLIZE}"
  export LIBTOOLIZE
else
  assert_deps libtoolize
fi

# fail early if required deps are missing
assert_deps aclocal autoreconf pkg-config

srcdir="$(dirname "$0")"
cd "$srcdir"
autoreconf --install --force --warnings=all
