#!/bin/sh
# Copyright (c) 2013-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
set -e
srcdir="$(dirname "$0")"
cd "$srcdir"
if [ -z "${LIBTOOLIZE}" ] && GLIBTOOLIZE="$(command -v glibtoolize)"; then
  LIBTOOLIZE="${GLIBTOOLIZE}"
  export LIBTOOLIZE
fi
command -v autoreconf >/dev/null || \
  (echo "configuration failed, please install autoconf first" && exit 1)
autoreconf --install --force --warnings=all

# autoreconf / libtoolize may generate versions of config.guess and config.sub
# that are too old to recognise some triplets, i.e arm64-apple-darwin20, so
# copy over the generated config.guess & config.sub with updated versions.
cp depends/config.guess build-aux/config.guess
cp depends/config.sub build-aux/config.sub

cp depends/config.guess src/secp256k1/build-aux/config.guess
cp depends/config.sub src/secp256k1/build-aux/config.sub

cp depends/config.guess src/univalue/build-aux/config.guess
cp depends/config.sub src/univalue/build-aux/config.sub
