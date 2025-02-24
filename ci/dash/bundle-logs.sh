#!/usr/bin/env bash
# Copyright (c) 2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -eo pipefail

SH_NAME="$(basename "${0}")"
LOG_DIRECTORY="testlogs"

if [ ! -d "${LOG_DIRECTORY}" ]; then
  echo "${SH_NAME}: '${LOG_DIRECTORY}' directory missing, will skip!";
  exit 0;
elif [ -z "${BUILD_TARGET}" ]; then
  echo "${SH_NAME}: BUILD_TARGET not defined, cannot continue!";
  exit 1;
fi

LOG_ARCHIVE="test_logs-${BUILD_TARGET}.tar.zst"
if [ -f "${LOG_ARCHIVE}" ]; then
  echo "${SH_NAME}: ${LOG_ARCHIVE} already exists, cannot continue!";
  exit 1;
fi

tar --use-compress-program="zstd -T0 -5" -cf "${LOG_ARCHIVE}" "${LOG_DIRECTORY}"
sha256sum "${LOG_ARCHIVE}" > "${LOG_ARCHIVE}.sha256";
