#!/usr/bin/env bash
#
# Copyright (c) 2018-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

${CI_RETRY_EXE} apt-get update
${CI_RETRY_EXE} apt-get install -y curl git gawk jq software-properties-common

${CI_RETRY_EXE} add-apt-repository --yes ppa:deadsnakes/ppa  # For python3.x
${CI_RETRY_EXE} apt-get install -y python3.6-venv
python3.6 -m venv ./lint_env  # Oldest supported version according to doc/dependencies.md

sed -i -e '1 s/^/# shellcheck disable=all\n/' ./lint_env/bin/activate
source ./lint_env/bin/activate

${CI_RETRY_EXE} pip3 install codespell==2.2.1
${CI_RETRY_EXE} pip3 install flake8==4.0.1
${CI_RETRY_EXE} pip3 install mypy==0.942
${CI_RETRY_EXE} pip3 install pyzmq==22.3.0
${CI_RETRY_EXE} pip3 install vulture==2.3

SHELLCHECK_VERSION=v0.8.0
curl -sL "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.x86_64.tar.xz" | tar --xz -xf - --directory /tmp/
export PATH="/tmp/shellcheck-${SHELLCHECK_VERSION}:${PATH}"
