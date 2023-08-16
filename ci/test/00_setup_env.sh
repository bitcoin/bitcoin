#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

# The root dir.
# The ci system copies this folder.
# This is where the build is done (depends and dist).
BASE_ROOT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )"/../../ >/dev/null 2>&1 && pwd )
export BASE_ROOT_DIR

echo "Setting specific values in env"
if [ -n "${FILE_ENV}" ]; then
  set -o errexit;
  # shellcheck disable=SC1090
  source "${FILE_ENV}"
fi

export BUILD_TARGET=${BUILD_TARGET:-linux64}
export PULL_REQUEST=${PULL_REQUEST:-false}
export JOB_NUMBER=${JOB_NUMBER:-1}

echo "Fallback to default values in env (if not yet set)"
# The number of parallel jobs to pass down to make and test_runner.py
MAKEJOBS="-j$(nproc)"
export MAKEJOBS
# A folder for the ci system to put temporary files (ccache, datadirs for tests, ...)
# This folder only exists on the ci host.
export BASE_SCRATCH_DIR=${BASE_SCRATCH_DIR:-$BASE_ROOT_DIR/ci/scratch/}
# What host to compile for. See also ./depends/README.md
# Tests that need cross-compilation export the appropriate HOST.
# Tests that run natively guess the host
export HOST=${HOST:-$("$BASE_ROOT_DIR/depends/config.guess")}
# Whether to prefer BusyBox over GNU utilities
export USE_BUSY_BOX=${USE_BUSY_BOX:-false}
export RUN_UNIT_TESTS=${RUN_UNIT_TESTS:-true}
export RUN_INTEGRATION_TESTS=${RUN_INTEGRATION_TESTS:-true}
export RUN_SECURITY_TESTS=${RUN_SECURITY_TESTS:-false}
export RUN_FUZZ_TESTS=${RUN_FUZZ_TESTS:-false}
export RUN_SYMBOL_TESTS=${RUN_SYMBOL_TESTS:-true}
export CONTAINER_NAME=${CONTAINER_NAME:-ci_unnamed}
export DOCKER_NAME_TAG=${DOCKER_NAME_TAG:-ubuntu:focal}
# Randomize test order.
# See https://www.boost.org/doc/libs/1_71_0/libs/test/doc/html/boost_test/utf_reference/rt_param_reference/random.html
export BOOST_TEST_RANDOM=${BOOST_TEST_RANDOM:-1}
export HOST_CACHE_DIR=${HOST_CACHE_DIR:-$BASE_ROOT_DIR/ci-cache-$BUILD_TARGET}
export CACHE_DIR=${CACHE_DIR:-$HOST_CACHE_DIR}
export CCACHE_SIZE=${CCACHE_SIZE:-100M}
export CCACHE_TEMPDIR=${CCACHE_TEMPDIR:-/tmp/.ccache-temp}
export CCACHE_COMPRESS=${CCACHE_COMPRESS:-1}
# The cache dir.
# This folder exists on the ci host and ci guest. Changes are propagated back and forth.
export CCACHE_DIR=${CCACHE_DIR:-$CACHE_DIR/ccache}
# The depends dir.
# This folder exists on the ci host and ci guest. Changes are propagated back and forth.
export DEPENDS_DIR=${DEPENDS_DIR:-$BASE_ROOT_DIR/depends}
# Folder where the build is done (bin and lib).
export BASE_OUTDIR=${BASE_OUTDIR:-$BASE_SCRATCH_DIR/out/$HOST}
export SDK_URL=${SDK_URL:-https://bitcoincore.org/depends-sources/sdks}
export WINEDEBUG=${WINEDEBUG:-fixme-all}
export DOCKER_PACKAGES=${DOCKER_PACKAGES:-build-essential libtool autotools-dev automake pkg-config bsdmainutils curl ca-certificates ccache python3 rsync git procps}
export GOAL=${GOAL:-install}
export DIR_QA_ASSETS=${DIR_QA_ASSETS:-${BASE_SCRATCH_DIR}/qa-assets}
export PATH=${BASE_ROOT_DIR}/ci/retry:$PATH
export CI_RETRY_EXE=${CI_RETRY_EXE:-"retry --"}
# Dash's Docker-specifics
export BUILDER_IMAGE_NAME="dash-builder-$BUILD_TARGET-$JOB_NUMBER"
export DOCKER_RUN_VOLUME_ARGS="-v $BASE_ROOT_DIR:$BASE_ROOT_DIR -v $HOST_CACHE_DIR:$CACHE_DIR"
export DOCKER_RUN_ENV_ARGS="-e BASE_ROOT_DIR=$BASE_ROOT_DIR -e CACHE_DIR=$CACHE_DIR -e PULL_REQUEST=$PULL_REQUEST -e COMMIT_RANGE=$COMMIT_RANGE -e JOB_NUMBER=$JOB_NUMBER -e BUILD_TARGET=$BUILD_TARGET"
export DOCKER_RUN_ARGS="$DOCKER_RUN_VOLUME_ARGS $DOCKER_RUN_ENV_ARGS"
export DOCKER_RUN_IN_BUILDER="docker run -t --rm -w $BASE_ROOT_DIR $DOCKER_RUN_ARGS $BUILDER_IMAGE_NAME"
