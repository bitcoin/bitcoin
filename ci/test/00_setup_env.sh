#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

# The root dir.
# The ci system copies this folder.
# This is where the depends build is done.
BASE_ROOT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )"/../../ >/dev/null 2>&1 && pwd )
export BASE_ROOT_DIR

echo "Setting specific values in env"
if [ -n "${FILE_ENV}" ]; then
  set -o errexit;
  # shellcheck disable=SC1090
  source "${FILE_ENV}"
fi

echo "Fallback to default values in env (if not yet set)"
# The number of parallel jobs to pass down to make and test_runner.py
export MAKEJOBS=${MAKEJOBS:--j4}
# A folder for the ci system to put temporary files (ccache, datadirs for tests, ...)
# This folder only exists on the ci host.
export BASE_SCRATCH_DIR=${BASE_SCRATCH_DIR:-$BASE_ROOT_DIR/ci/scratch}
# What host to compile for. See also ./depends/README.md
# Tests that need cross-compilation export the appropriate HOST.
# Tests that run natively guess the host
export HOST=${HOST:-$("$BASE_ROOT_DIR/depends/config.guess")}
# Whether to prefer BusyBox over GNU utilities
export USE_BUSY_BOX=${USE_BUSY_BOX:-false}

export RUN_UNIT_TESTS=${RUN_UNIT_TESTS:-true}
export RUN_FUNCTIONAL_TESTS=${RUN_FUNCTIONAL_TESTS:-true}
export RUN_SECURITY_TESTS=${RUN_SECURITY_TESTS:-false}
# By how much to scale the test_runner timeouts (option --timeout-factor).
# This is needed because some ci machines have slow CPU or disk, so sanitizers
# might be slow or a reindex might be waiting on disk IO.
export TEST_RUNNER_TIMEOUT_FACTOR=${TEST_RUNNER_TIMEOUT_FACTOR:-40}
export TEST_RUNNER_ENV=${TEST_RUNNER_ENV:-}
export RUN_FUZZ_TESTS=${RUN_FUZZ_TESTS:-false}
export EXPECTED_TESTS_DURATION_IN_SECONDS=${EXPECTED_TESTS_DURATION_IN_SECONDS:-1000}

export CONTAINER_NAME=${CONTAINER_NAME:-ci_unnamed}
export DOCKER_NAME_TAG=${DOCKER_NAME_TAG:-ubuntu:18.04}
# Randomize test order.
# See https://www.boost.org/doc/libs/1_71_0/libs/test/doc/html/boost_test/utf_reference/rt_param_reference/random.html
export BOOST_TEST_RANDOM=${BOOST_TEST_RANDOM:-1}
# See man 7 debconf
export DEBIAN_FRONTEND=noninteractive
export CCACHE_SIZE=${CCACHE_SIZE:-100M}
export CCACHE_TEMPDIR=${CCACHE_TEMPDIR:-/tmp/.ccache-temp}
export CCACHE_COMPRESS=${CCACHE_COMPRESS:-1}
# The cache dir.
# This folder exists on the ci host and ci guest. Changes are propagated back and forth.
export CCACHE_DIR=${CCACHE_DIR:-$BASE_SCRATCH_DIR/.ccache}
# The depends dir.
# This folder exists on the ci host and ci guest. Changes are propagated back and forth.
export DEPENDS_DIR=${DEPENDS_DIR:-$BASE_ROOT_DIR/depends}
# Folder where the build result is put (bin and lib).
export BASE_OUTDIR=${BASE_OUTDIR:-$BASE_SCRATCH_DIR/out/$HOST}
# Folder where the build is done (dist and out-of-tree build).
export BASE_BUILD_DIR=${BASE_BUILD_DIR:-$BASE_SCRATCH_DIR/build}
export PREVIOUS_RELEASES_DIR=${PREVIOUS_RELEASES_DIR:-$BASE_ROOT_DIR/releases/$HOST}
export SDK_URL=${SDK_URL:-https://widecoincore.org/depends-sources/sdks}
export DOCKER_PACKAGES=${DOCKER_PACKAGES:-build-essential libtool autotools-dev automake pkg-config bsdmainutils curl ca-certificates ccache python3 rsync git procps}
export GOAL=${GOAL:-install}
export DIR_QA_ASSETS=${DIR_QA_ASSETS:-${BASE_SCRATCH_DIR}/qa-assets}
export PATH=${BASE_ROOT_DIR}/ci/retry:$PATH
export CI_RETRY_EXE=${CI_RETRY_EXE:-"retry --"}
