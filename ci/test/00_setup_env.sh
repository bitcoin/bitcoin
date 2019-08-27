#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

echo "Setting default values in env"

BASE_ROOT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )"/../../ >/dev/null 2>&1 && pwd )
export BASE_ROOT_DIR

# The number of parallel jobs to pass down to make and test_runner.py
export MAKEJOBS=${MAKEJOBS:--j4}
# A folder for the ci system to put temporary files (ccache, datadirs for tests, ...)
export BASE_SCRATCH_DIR=${BASE_SCRATCH_DIR:-$BASE_ROOT_DIR/ci/scratch/}
export HOST=${HOST:-x86_64-unknown-linux-gnu}
export RUN_UNIT_TESTS=${RUN_UNIT_TESTS:-true}
export RUN_FUNCTIONAL_TESTS=${RUN_FUNCTIONAL_TESTS:-true}
export RUN_FUZZ_TESTS=${RUN_FUZZ_TESTS:-false}
export DOCKER_NAME_TAG=${DOCKER_NAME_TAG:-ubuntu:18.04}
export BOOST_TEST_RANDOM=${BOOST_TEST_RANDOM:-1$TRAVIS_BUILD_ID}
export CCACHE_SIZE=${CCACHE_SIZE:-100M}
export CCACHE_TEMPDIR=${CCACHE_TEMPDIR:-/tmp/.ccache-temp}
export CCACHE_COMPRESS=${CCACHE_COMPRESS:-1}
export CCACHE_DIR=${CCACHE_DIR:-$HOME/.ccache}
export BASE_BUILD_DIR=${BASE_BUILD_DIR:-${TRAVIS_BUILD_DIR:-$BASE_ROOT_DIR}}
export BASE_OUTDIR=${BASE_OUTDIR:-$BASE_BUILD_DIR/out/$HOST}
export SDK_URL=${SDK_URL:-https://bitcoincore.org/depends-sources/sdks}
export WINEDEBUG=${WINEDEBUG:-fixme-all}
export DOCKER_PACKAGES=${DOCKER_PACKAGES:-build-essential libtool autotools-dev automake pkg-config bsdmainutils curl ca-certificates ccache python3}
export GOAL=${GOAL:-install}
export DIR_QA_ASSETS=${DIR_QA_ASSETS:-${BASE_BUILD_DIR}/qa-assets}
export PATH=${BASE_ROOT_DIR}/ci/retry:$PATH
export CI_RETRY_EXE=${CI_RETRY_EXE:retry}

echo "Setting specific values in env"
if [ -n "${FILE_ENV}" ]; then
  set -o errexit;
  # shellcheck disable=SC1090
  source "${FILE_ENV}"
fi
