#!/usr/bin/env bash
#
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

set -o errexit -o pipefail -o xtrace

export CI_RETRY_EXE="/ci_retry"

pushd "/"

${CI_RETRY_EXE} apt-get update
# Lint dependencies:
# - cargo (used to run the lint tests)
# - curl/xz-utils (to install shellcheck)
# - git (used in many lint scripts)
# - gpg (used by verify-commits)
# - moreutils (used by scripted-diff)
${CI_RETRY_EXE} apt-get install -y cargo curl xz-utils git gpg moreutils

PYENV_PATH="/pyenv"
if [ ! -d "${PYENV_PATH}/bin" ]; then
  ${CI_RETRY_EXE} git clone --depth=1 https://github.com/pyenv/pyenv.git ${PYENV_PATH}

  # For dependencies see https://github.com/pyenv/pyenv/wiki#suggested-build-environment
  ${CI_RETRY_EXE} apt-get install -y build-essential libssl-dev zlib1g-dev \
    libbz2-dev libreadline-dev libsqlite3-dev curl llvm \
    libncursesw5-dev xz-utils tk-dev libxml2-dev libxmlsec1-dev libffi-dev liblzma-dev \
    clang
fi
export PATH="${PYENV_PATH}/bin:${PATH}"
eval "$(pyenv init - bash --path)"

 # Build the most recent Python patch release matching .python-version
env CC=clang pyenv install --skip-existing "$(cat "/.python-version")"
pyenv global "${PYTHON_VERSION}"
command -v python3
python3 --version

${CI_RETRY_EXE} pip3 install \
  lief==0.16.6 \
  mypy==1.18.2 \
  pyzmq==27.1.0 \
  ruff==0.13.2 \
  vulture==2.14

SHELLCHECK_VERSION=v0.11.0
curl -sL "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.x86_64.tar.xz" | \
    tar --xz -xf - --directory /tmp/
mv "/tmp/shellcheck-${SHELLCHECK_VERSION}/shellcheck" /usr/bin/

MLC_VERSION=v1
MLC_BIN=mlc-x86_64-linux
curl -sL "https://github.com/becheran/mlc/releases/download/${MLC_VERSION}/${MLC_BIN}" -o "/usr/bin/mlc"
chmod +x /usr/bin/mlc

popd || exit
