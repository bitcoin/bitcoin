#!/usr/bin/env bash
#
# Copyright (c) 2019-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -ex

# The source root dir, usually from git, usually read-only.
# The ci system copies this folder.
BASE_READ_ONLY_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )"/../../ >/dev/null 2>&1 && pwd )
export BASE_READ_ONLY_DIR
# The destination root dir inside the container.
# This folder will also hold any SDKs.
# This folder only exists on the ci guest and will be a copy of BASE_READ_ONLY_DIR
export BASE_ROOT_DIR="${BASE_ROOT_DIR:-/ci_container_base}"
# The depends dir.
# This folder exists only on the ci guest, and on the ci host as a volume.
export DEPENDS_DIR=${DEPENDS_DIR:-$BASE_ROOT_DIR/depends}
# A folder for the ci system to put temporary files (build result, datadirs for tests, ...)
# This folder only exists on the ci guest.
export BASE_SCRATCH_DIR=${BASE_SCRATCH_DIR:-$BASE_ROOT_DIR/ci/scratch}
# A folder for the ci system to put executables.
# This folder only exists on the ci guest.
export BINS_SCRATCH_DIR="${BASE_SCRATCH_DIR}/bins/"

echo "Setting specific values in env"
if [ -n "${FILE_ENV}" ]; then
  set -o errexit;
  # shellcheck disable=SC1090
  source "${FILE_ENV}"
fi

echo "Fallback to default values in env (if not yet set)"
# The number of parallel jobs to pass down to make and test_runner.py
export MAKEJOBS=${MAKEJOBS:--j4}
# Whether to prefer BusyBox over GNU utilities
export USE_BUSY_BOX=${USE_BUSY_BOX:-false}

export RUN_UNIT_TESTS=${RUN_UNIT_TESTS:-true}
export RUN_FUNCTIONAL_TESTS=${RUN_FUNCTIONAL_TESTS:-true}
export RUN_TIDY=${RUN_TIDY:-false}
# By how much to scale the test_runner timeouts (option --timeout-factor).
# This is needed because some ci machines have slow CPU or disk, so sanitizers
# might be slow or a reindex might be waiting on disk IO.
export TEST_RUNNER_TIMEOUT_FACTOR=${TEST_RUNNER_TIMEOUT_FACTOR:-40}
export RUN_FUZZ_TESTS=${RUN_FUZZ_TESTS:-false}

# Randomize test order.
# See https://www.boost.org/doc/libs/1_71_0/libs/test/doc/html/boost_test/utf_reference/rt_param_reference/random.html
export BOOST_TEST_RANDOM=${BOOST_TEST_RANDOM:-1}
# See man 7 debconf
export DEBIAN_FRONTEND=noninteractive
export CCACHE_MAXSIZE=${CCACHE_MAXSIZE:-100M}
export CCACHE_TEMPDIR=${CCACHE_TEMPDIR:-/tmp/.ccache-temp}
export CCACHE_COMPRESS=${CCACHE_COMPRESS:-1}
# The cache dir.
# This folder exists only on the ci guest, and on the ci host as a volume.
export CCACHE_DIR=${CCACHE_DIR:-$BASE_SCRATCH_DIR/.ccache}
# Folder where the build result is put (bin and lib).
export BASE_OUTDIR=${BASE_OUTDIR:-$BASE_SCRATCH_DIR/out}
# The folder for previous release binaries.
# This folder exists only on the ci guest, and on the ci host as a volume.
export PREVIOUS_RELEASES_DIR=${PREVIOUS_RELEASES_DIR:-$BASE_ROOT_DIR/prev_releases}
export CI_BASE_PACKAGES=${CI_BASE_PACKAGES:-build-essential libtool autotools-dev automake pkg-config bsdmainutils curl ca-certificates ccache python3 rsync git procps bison e2fsprogs cmake}
export GOAL=${GOAL:-install}
export DIR_QA_ASSETS=${DIR_QA_ASSETS:-${BASE_SCRATCH_DIR}/qa-assets}
export CI_RETRY_EXE=${CI_RETRY_EXE:-"retry --"}
