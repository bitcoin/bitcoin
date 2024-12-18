#!/bin/sh
# Copyright (c) 2014-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
if [ -z "$OSSLSIGNCODE" ]; then
  OSSLSIGNCODE=osslsigncode
fi

if [ "$#" -ne 1 ]; then
  echo "usage: $0 <path to key>"
  echo "example: $0 codesign.key"
  exit 1
fi

OUT=signature-win.tar.gz
SRCDIR=unsigned
WORKDIR=./.tmp
OUTDIR="${WORKDIR}/out"
OUTSUBDIR="${OUTDIR}/win"
TIMESERVER=http://timestamp.comodoca.com
CERTFILE="win-codesign.cert"

stty -echo
printf "Enter the passphrase for %s: " "$1"
read cs_key_pass
printf "\n"
stty echo


mkdir -p "${OUTSUBDIR}"
find ${SRCDIR} -wholename "*.exe" -type f -exec realpath --relative-to=. {} \; | while read -r bin
do
    echo Signing "${bin}"
    bin_base="$(realpath --relative-to=${SRCDIR} "${bin}")"
    mkdir -p "$(dirname ${WORKDIR}/"${bin_base}")"
    "${OSSLSIGNCODE}" sign -certs "${CERTFILE}" -t "${TIMESERVER}" -h sha256 -in "${bin}" -out "${WORKDIR}/${bin_base}" -key "$1" -pass "${cs_key_pass}"
    mkdir -p "$(dirname ${OUTSUBDIR}/"${bin_base}")"
    "${OSSLSIGNCODE}" extract-signature -pem -in "${WORKDIR}/${bin_base}" -out "${OUTSUBDIR}/${bin_base}.pem" && rm "${WORKDIR}/${bin_base}"
done

rm -f "${OUT}"
tar -C "${OUTDIR}" -czf "${OUT}" .
rm -rf "${WORKDIR}"
echo "Created ${OUT}"
