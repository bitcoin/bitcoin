#!/usr/bin/env bash
#
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -o errexit -o pipefail -o xtrace

if [ "${DANGER_RUN_CI_ON_HOST}" != "1" ]; then
  echo "This script will make unsafe local and global modifications, so it can only be run inside a container and requires DANGER_RUN_CI_ON_HOST=1"
  exit 1
fi

mkdir -p "${DEPENDS_DIR}/SDKs" "${DEPENDS_DIR}/sdk-sources"

if [ -n "$NETBSD_VERSION" ] && [ ! -d "${DEPENDS_DIR}/SDKs/${NETBSD_SDK_BASENAME}" ]; then
  mkdir -p "${DEPENDS_DIR}/SDKs/${NETBSD_SDK_BASENAME}"
  for NETBSD_SDK_FILENAME in base.tar.xz comp.tar.xz; do
    NETBSD_SDK_PATH="${DEPENDS_DIR}/sdk-sources/${NETBSD_SDK_FILENAME}"
    if [ ! -f "$NETBSD_SDK_PATH" ]; then
      ${CI_RETRY_EXE} curl --location --fail "https://cdn.netbsd.org/pub/NetBSD/NetBSD-${NETBSD_VERSION}/amd64/binary/sets/${NETBSD_SDK_FILENAME}" -o "$NETBSD_SDK_PATH"
    fi
    tar -C "${DEPENDS_DIR}/SDKs/${NETBSD_SDK_BASENAME}" -xf "$NETBSD_SDK_PATH"
  done
fi

if [ -n "$FREEBSD_VERSION" ] && [ ! -d "${DEPENDS_DIR}/SDKs/${FREEBSD_SDK_BASENAME}" ]; then
  FREEBSD_SDK_FILENAME="base-${FREEBSD_VERSION}.txz"
  FREEBSD_SDK_PATH="${DEPENDS_DIR}/sdk-sources/${FREEBSD_SDK_FILENAME}"
  if [ ! -f "$FREEBSD_SDK_PATH" ]; then
    ${CI_RETRY_EXE} curl --location --fail "https://download.freebsd.org/releases/amd64/${FREEBSD_VERSION}-RELEASE/base.txz" -o "$FREEBSD_SDK_PATH"
  fi
  mkdir -p "${DEPENDS_DIR}/SDKs/${FREEBSD_SDK_BASENAME}"
  tar -C "${DEPENDS_DIR}/SDKs/${FREEBSD_SDK_BASENAME}" -xf "$FREEBSD_SDK_PATH"
fi

if [ -n "$OPENBSD_VERSION" ] && [ ! -d "${DEPENDS_DIR}/SDKs/${OPENBSD_SDK_BASENAME}" ]; then
  mkdir -p "${DEPENDS_DIR}/SDKs/${OPENBSD_SDK_BASENAME}"
  for OPENBSD_SDK_FILENAME in base79.tgz comp79.tgz; do
    OPENBSD_SDK_PATH="${DEPENDS_DIR}/sdk-sources/${OPENBSD_SDK_FILENAME}"
    if [ ! -f "$OPENBSD_SDK_PATH" ]; then
      ${CI_RETRY_EXE} curl --location --fail "https://cdn.openbsd.org/pub/OpenBSD/${OPENBSD_VERSION}/amd64/${OPENBSD_SDK_FILENAME}" -o "$OPENBSD_SDK_PATH"
    fi
    tar -C "${DEPENDS_DIR}/SDKs/${OPENBSD_SDK_BASENAME}" -xf "$OPENBSD_SDK_PATH"
    (
      # The SDK has versioned shared libs, but no unversioned libfoo.so symlink,
      # which breaks linking the kernel with lld. Create the symlinks.
      cd "${DEPENDS_DIR}/SDKs/${OPENBSD_SDK_BASENAME}/usr/lib"
      ln -sf libc++abi.so.*.*  libc++abi.so
      ln -sf libc++.so.*.*     libc++.so
      ln -sf libpthread.so.*.* libpthread.so
    )
  done
fi
