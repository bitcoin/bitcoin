#!/bin/sh
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
set -e

ROOTDIR=dist
BUNDLE="${ROOTDIR}/Litecoin-Qt.app"
SIGNAPPLE=signapple
TEMPDIR=sign.temp
OUT=signature-osx.tar.gz
OUTROOT=osx/dist

if [ -z "$1" ]; then
  echo "usage: $0 <signapple args>"
  echo "example: $0 <path to key>"
  exit 1
fi

rm -rf ${TEMPDIR}
mkdir -p ${TEMPDIR}

${SIGNAPPLE} sign -f --detach "${TEMPDIR}/${OUTROOT}"  "$@" "${BUNDLE}"

tar -C "${TEMPDIR}" -czf "${OUT}" .
rm -rf "${TEMPDIR}"
echo "Created ${OUT}"
