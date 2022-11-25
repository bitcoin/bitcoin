#!/usr/bin/env bash
#
# Copyright (c) 2018-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

${CI_RETRY_EXE} apt-get update
${CI_RETRY_EXE} apt-get install -y --no-install-recommends \
    curl git gawk jq build-essential clang libbz2-dev \
    libffi-dev liblzma-dev libncursesw5-dev libreadline-dev \
    libsqlite3-dev libssl-dev zlib1g-dev

INSTALL_PYENV=$([[ ! -d "${PYENV_ROOT}" ]] && echo true || echo false)
[[ "${INSTALL_PYENV}" == 'true' ]] && ${CI_RETRY_EXE} curl -sL https://pyenv.run | bash
export PATH="${PYENV_ROOT}/bin:${PATH}"
eval "$(pyenv init -)"
# Python version is determined by the .python-version file in the project root.
[[ "${INSTALL_PYENV}" == 'true' ]] && CC=clang ${CI_RETRY_EXE} pyenv install
pyenv local

${CI_RETRY_EXE} pip3 install codespell==2.2.1
${CI_RETRY_EXE} pip3 install flake8==4.0.1
${CI_RETRY_EXE} pip3 install mypy==0.942
${CI_RETRY_EXE} pip3 install pyzmq==22.3.0
${CI_RETRY_EXE} pip3 install vulture==2.3

SHELLCHECK_VERSION=v0.8.0
curl -sL "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.x86_64.tar.xz" | tar --xz -xf - --directory /tmp/
export PATH="/tmp/shellcheck-${SHELLCHECK_VERSION}:${PATH}"
