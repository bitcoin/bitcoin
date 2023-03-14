#!/usr/bin/env bash
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

CI_EXEC_ROOT () { bash -c "$*"; }
export -f CI_EXEC_ROOT

if [ -n "$DPKG_ADD_ARCH" ]; then
  CI_EXEC_ROOT dpkg --add-architecture "$DPKG_ADD_ARCH"
fi

if [[ $CI_IMAGE_NAME_TAG == *centos* ]]; then
  ${CI_RETRY_EXE} CI_EXEC_ROOT dnf -y install epel-release
  ${CI_RETRY_EXE} CI_EXEC_ROOT dnf -y --allowerasing install "$CI_BASE_PACKAGES" "$PACKAGES"
elif [ "$CI_USE_APT_INSTALL" != "no" ]; then
  if [[ "${ADD_UNTRUSTED_BPFCC_PPA}" == "true" ]]; then
    # Ubuntu 22.04 LTS and Debian 11 both have an outdated bpfcc-tools packages.
    # The iovisor PPA is outdated as well. The next Ubuntu and Debian releases will contain updated
    # packages. Meanwhile, use an untrusted PPA to install an up-to-date version of the bpfcc-tools
    # package.
    # TODO: drop this once we can use newer images in GCE
    CI_EXEC_ROOT add-apt-repository ppa:hadret/bpfcc
  fi
  if [[ -n "${APPEND_APT_SOURCES_LIST}" ]]; then
    CI_EXEC_ROOT echo "${APPEND_APT_SOURCES_LIST}" \>\> /etc/apt/sources.list
  fi
  ${CI_RETRY_EXE} CI_EXEC_ROOT apt-get update
  ${CI_RETRY_EXE} CI_EXEC_ROOT apt-get install --no-install-recommends --no-upgrade -y "$PACKAGES" "$CI_BASE_PACKAGES"
fi
