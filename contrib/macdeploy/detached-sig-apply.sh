#!/bin/sh
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
set -e

UNSIGNED="$1"
SIGNATURE="$2"
ARCH=x86_64
ROOTDIR=dist
TEMPDIR=signed.temp
OUTDIR=signed-app

if [ -z "$UNSIGNED" ]; then
  echo "usage: $0 <unsigned app> <signature>"
  exit 1
fi

if [ -z "$SIGNATURE" ]; then
  echo "usage: $0 <unsigned app> <signature>"
  exit 1
fi

rm -rf ${TEMPDIR} && mkdir -p ${TEMPDIR}
tar -C ${TEMPDIR} -xf ${UNSIGNED}
cp -rf "${SIGNATURE}"/* ${TEMPDIR}

if [ -z "${PAGESTUFF}" ]; then
  PAGESTUFF=${TEMPDIR}/pagestuff
fi

if [ -z "${CODESIGN_ALLOCATE}" ]; then
  CODESIGN_ALLOCATE=${TEMPDIR}/codesign_allocate
fi

find ${TEMPDIR} -name "*.sign" | while read i; do
  SIZE=$(stat -c %s "${i}")
  TARGET_FILE="$(echo "${i}" | sed 's/\.sign$//')"

  echo "Allocating space for the signature of size ${SIZE} in ${TARGET_FILE}"
  ${CODESIGN_ALLOCATE} -i "${TARGET_FILE}" -a ${ARCH} ${SIZE} -o "${i}.tmp"

  OFFSET=$(${PAGESTUFF} "${i}.tmp" -p | tail -2 | grep offset | sed 's/[^0-9]*//g')
  if [ -z ${QUIET} ]; then
    echo "Attaching signature at offset ${OFFSET}"
  fi

  dd if="$i" of="${i}.tmp" bs=1 seek=${OFFSET} count=${SIZE} 2>/dev/null
  mv "${i}.tmp" "${TARGET_FILE}"
  rm "${i}"
  echo "Success."
done
mv ${TEMPDIR}/${ROOTDIR} ${OUTDIR}
rm -rf ${TEMPDIR}
echo "Signed: ${OUTDIR}"
