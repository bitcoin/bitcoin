#!/usr/bin/env bash
#
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

set -o errexit -o pipefail -o xtrace

export DEBIAN_FRONTEND=noninteractive
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

# Install Python dependencies using uv
export UV_PROJECT_ENVIRONMENT=/python_env
uv sync --project /ci/lint --locked --only-group lint

export PATH="/python_env/bin:${PATH}"
command -v python3
python3 --version

ARCH=$(uname --machine)
case "${ARCH}" in
  x86_64)  SHELLCHECK_SHA256=8c3be12b05d5c177a04c29e3c78ce89ac86f1595681cab149b65b97c4e227198 MLC_SHA256=7a72a93d5b3ee8a554cb840abdfe90aefb709418f225461b52021e3a058238a2 ;;
  aarch64) SHELLCHECK_SHA256=12b331c1d2db6b9eb13cfca64306b1b157a86eb69db83023e261eaa7e7c14588 MLC_SHA256=01ec8e086f3b625616d461b63451be9175a02557de6b591bac7cde6791ab074b ;;
esac

SHELLCHECK_VERSION=v0.11.0
SHELLCHECK_ARCHIVE=/tmp/shellcheck.tar.xz
curl --fail -L "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.${ARCH}.tar.xz" -o "${SHELLCHECK_ARCHIVE}"
sha256sum -c <<<"${SHELLCHECK_SHA256} ${SHELLCHECK_ARCHIVE}"
tar --xz -xf "${SHELLCHECK_ARCHIVE}" --directory /tmp/
mv "/tmp/shellcheck-${SHELLCHECK_VERSION}/shellcheck" /usr/bin/
rm -rf "${SHELLCHECK_ARCHIVE}" "/tmp/shellcheck-${SHELLCHECK_VERSION}"

MLC_VERSION=v1.2.0
MLC_PATH=/usr/bin/mlc
curl --fail -L "https://github.com/becheran/mlc/releases/download/${MLC_VERSION}/mlc-${ARCH}-linux" -o "${MLC_PATH}"
sha256sum -c <<<"${MLC_SHA256} ${MLC_PATH}"
chmod +x "${MLC_PATH}"

popd || exit
