#!/bin/sh
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
set -e

SIGNAPPLE=signapple
TEMPDIR=sign.temp

BUNDLE_ROOT=dist
BUNDLE_NAME="Bitcoin-Qt.app"
UNSIGNED_BUNDLE="${BUNDLE_ROOT}/${BUNDLE_NAME}"
UNSIGNED_BINARY="${UNSIGNED_BUNDLE}/Contents/MacOS/Bitcoin-Qt"

ARCH=$(${SIGNAPPLE} info ${UNSIGNED_BINARY} | head -n 1 | cut -d " " -f 1)

OUTDIR="osx/${ARCH}-apple-darwin"
OUTROOT="${TEMPDIR}/${OUTDIR}"

OUT="signature-osx-${ARCH}.tar.gz"

if [ "$#" -ne 3 ]; then
  echo "usage: $0 <path to key> <path to app store connect key> <apple developer team uuid>"
  exit 1
fi

rm -rf ${TEMPDIR}
mkdir -p ${TEMPDIR}

stty -echo
printf "Enter the passphrase for %s: " "$1"
read cs_key_pass
printf "\n"
printf "Enter the passphrase for %s: " "$2"
read api_key_pass
printf "\n"
stty echo

# Sign and notarize app bundle
${SIGNAPPLE} sign -f --hardened-runtime --detach "${OUTROOT}/${BUNDLE_ROOT}" --passphrase "${cs_key_pass}" "$1" "${UNSIGNED_BUNDLE}"
${SIGNAPPLE} apply "${UNSIGNED_BUNDLE}" "${OUTROOT}/${BUNDLE_ROOT}/${BUNDLE_NAME}"
${SIGNAPPLE} notarize --detach "${OUTROOT}/${BUNDLE_ROOT}" --passphrase "${api_key_pass}" "$2" "$3" "${UNSIGNED_BUNDLE}"

# Sign each binary
find . -maxdepth 3 -wholename "*/bin/*" -type f -exec realpath --relative-to=. {} \; | while read -r bin
do
    bin_dir=$(dirname "${bin}")
    bin_name=$(basename "${bin}")
    ${SIGNAPPLE} sign -f --hardened-runtime --detach "${OUTROOT}/${bin_dir}" --passphrase "${cs_key_pass}" "$1" "${bin}"
    ${SIGNAPPLE} apply "${bin}" "${OUTROOT}/${bin_dir}/${bin_name}.${ARCH}sign"
done

# Notarize the binaries
# Binaries cannot have stapled notarizations so this does not actually generate any output
binaries_dir=$(dirname "$(find . -maxdepth 2 -wholename '*/bin' -type d -exec realpath --relative-to=. {} \;)")
${SIGNAPPLE} notarize --passphrase "${api_key_pass}" "$2" "$3" "${binaries_dir}"

tar -C "${TEMPDIR}" -czf "${OUT}" "${OUTDIR}"
rm -rf "${TEMPDIR}"
echo "Created ${OUT}"
