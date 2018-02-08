#!/bin/sh
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
if [ -z "$OSSLSIGNCODE" ]; then
  OSSLSIGNCODE=osslsigncode
fi

if [ -z "$1" ]; then
  echo "usage: $0 <osslcodesign args>"
  echo "example: $0 -key codesign.key"
  exit 1
fi

OUT=signature-win.tar.gz
SRCDIR=unsigned
WORKDIR=./.tmp
OUTDIR="${WORKDIR}/out"
OUTSUBDIR="${OUTDIR}/win"
TIMESERVER=http://timestamp.digicert.com
CERTFILE="win-codesign.cert"

mkdir -p "${OUTSUBDIR}"
basename -a $(ls -1 "${SRCDIR}"/*-unsigned.exe) | while read UNSIGNED; do
  echo Signing "${UNSIGNED}"
  "${OSSLSIGNCODE}" sign -certs "${CERTFILE}" -t "${TIMESERVER}" -in "${SRCDIR}/${UNSIGNED}" -out "${WORKDIR}/${UNSIGNED}" "$@"
  "${OSSLSIGNCODE}" extract-signature -pem -in "${WORKDIR}/${UNSIGNED}" -out "${OUTSUBDIR}/${UNSIGNED}.pem" && rm "${WORKDIR}/${UNSIGNED}"
done

rm -f "${OUT}"
tar -C "${OUTDIR}" -czf "${OUT}" .
rm -rf "${WORKDIR}"
echo "Created ${OUT}"
