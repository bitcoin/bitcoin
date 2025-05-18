#!/usr/bin/env bash
#
# Copyright (c) The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

export LC_ALL=C

# Fixes permission issues when there is a container UID/GID mismatch with the owner
# of the mounted tortoisecoin src dir.
git config --global --add safe.directory /tortoisecoin

export PATH="/python_build/bin:${PATH}"
export LINT_RUNNER_PATH="/lint_test_runner"

if [ -z "$1" ]; then
  bash -ic "./ci/lint/06_script.sh"
else
  exec "$@"
fi
