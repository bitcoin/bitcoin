#!/usr/bin/env bash
#
# Copyright (c) 2018-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

${CI_RETRY_EXE} apt-get update
${CI_RETRY_EXE} apt-get install -y python3-pip curl git gawk jq
(
  # Temporary workaround for https://github.com/bitcoin/bitcoin/pull/26130#issuecomment-1260499544
  # Can be removed once the underlying image is bumped to something that includes git2.34 or later
  sed -i -e 's/bionic/jammy/g' /etc/apt/sources.list
  ${CI_RETRY_EXE} apt-get update
  ${CI_RETRY_EXE} apt-get install -y --reinstall git
)

${CI_RETRY_EXE} pip3 install codespell==2.2.1
${CI_RETRY_EXE} pip3 install flake8==4.0.1
${CI_RETRY_EXE} pip3 install mypy==0.942
${CI_RETRY_EXE} pip3 install pyzmq==22.3.0
${CI_RETRY_EXE} pip3 install vulture==2.3

SHELLCHECK_VERSION=v0.8.0
curl -sL "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.x86_64.tar.xz" | tar --xz -xf - --directory /tmp/
export PATH="/tmp/shellcheck-${SHELLCHECK_VERSION}:${PATH}"
