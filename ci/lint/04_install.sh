#!/usr/bin/env bash
#
# Copyright (c) 2018-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

${CI_RETRY_EXE} apt-get update
${CI_RETRY_EXE} apt-get install -y curl git gawk jq
${CI_RETRY_EXE} apt-get install -y make build-essential libssl-dev zlib1g-dev \
libbz2-dev libreadline-dev libsqlite3-dev wget curl llvm \
libncursesw5-dev xz-utils tk-dev libxml2-dev libxmlsec1-dev libffi-dev liblzma-dev

export PYENV_ROOT="$HOME/.pyenv"
export PATH="$PYENV_ROOT/bin:$PATH"
${CI_RETRY_EXE} curl https://pyenv.run | bash
eval "$(pyenv init -)"

[[ -d /tmp/python-build ]] && cp -r /tmp/python-build/* "$PYENV_ROOT"/versions/"$(cat .python-version)" && echo "Python cache found, copying..." # Pyenv doesn't like .pyenv being already present, so an intermediary directory is used to interface with Cirrus's CI Caching features
${CI_RETRY_EXE} pyenv install # Uses python version found in CWD's .python-version file, or searches parent directories until root for it
${CI_RETRY_EXE} pyenv local
mkdir -p /tmp/python-build && cp -r "$PYENV_ROOT"/versions/"$(cat .python-version)"/* /tmp/python-build && echo "Copying built python to tmp" # Copy built python to intermediary directory

${CI_RETRY_EXE} pip3 install codespell==2.2.1
${CI_RETRY_EXE} pip3 install flake8==4.0.1
${CI_RETRY_EXE} pip3 install mypy==0.942
${CI_RETRY_EXE} pip3 install pyzmq==22.3.0
${CI_RETRY_EXE} pip3 install vulture==2.3

SHELLCHECK_VERSION=v0.8.0
curl -sL "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.x86_64.tar.xz" | tar --xz -xf - --directory /tmp/
export PATH="/tmp/shellcheck-${SHELLCHECK_VERSION}:${PATH}"
