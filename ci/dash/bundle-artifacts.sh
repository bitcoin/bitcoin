#!/usr/bin/env bash
# Copyright (c) 2024-2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -eo pipefail

SH_NAME="$(basename "${0}")"
VERB="${1}"

if [ -z "${BUILD_TARGET}" ]; then
  echo "${SH_NAME}: BUILD_TARGET not defined, cannot continue!";
  exit 1;
elif [ -z "${BUNDLE_KEY}" ]; then
  echo "${SH_NAME}: BUNDLE_KEY not defined, cannot continue!";
  exit 1;
elif  [ ! "$(command -v zstd)" ]; then
  echo "${SH_NAME}: zstd not found, cannot continue!";
  exit 1;
elif [ -z "${VERB}" ]; then
  echo "${SH_NAME}: Verb missing, acceptable values 'create' or 'extract'";
  exit 1;
elif [ "${VERB}" != "create" ] && [ "${VERB}" != "extract" ]; then
  echo "${SH_NAME}: Invalid verb '${VERB}', expected 'create' or 'extract'";
  exit 1;
fi

OUTPUT_ARCHIVE="${BUNDLE_KEY}.tar.zst"
if [ -f "${OUTPUT_ARCHIVE}" ] && [ "${VERB}" = "create" ]; then
  echo "${SH_NAME}: ${OUTPUT_ARCHIVE} already exists, cannot continue!";
  exit 1;
elif [ ! -f "${OUTPUT_ARCHIVE}" ] && [ "${VERB}" = "extract" ]; then
  echo "${SH_NAME}: ${OUTPUT_ARCHIVE} missing, cannot continue!";
  exit 1;
fi

if [ "${VERB}" = "create" ]; then
  EXCLUSIONS=(
    "*.a"
    "*.o"
    ".deps"
    ".libs"
  )
  EXCLUSIONS_ARG=""
  for excl in "${EXCLUSIONS[@]}"
  do
    EXCLUSIONS_ARG+=" --exclude=${excl}";
  done

  # shellcheck disable=SC2086
  tar ${EXCLUSIONS_ARG} --use-compress-program="zstd -T0 -5" -cf "${OUTPUT_ARCHIVE}" "build-ci";
elif [ "${VERB}" = "extract" ]; then
  tar --use-compress-program="unzstd" -xf "${OUTPUT_ARCHIVE}";
else
  echo "${SH_NAME}: Generic error";
  exit 1;
fi
