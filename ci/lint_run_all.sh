#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -eo pipefail

if [ -n "$CIRRUS_PR" ]; then
  # For CI runs
  cp "./ci/retry/retry" "/ci_retry"
  cp "./.python-version" "/.python-version"
  mkdir --parents "/test/lint"
  cp --recursive "./test/lint/test_runner" "/test/lint/"
  set -o errexit
  # shellcheck source=/dev/null
  source ./ci/lint/04_install.sh
  set -o errexit
  ./ci/lint/06_script.sh
else
  # For local runs
  error() {
    echo "ERROR: $1" >&2
    exit 1
  }


  echo "Building Docker image..."
  DOCKER_BUILDKIT=1 docker build \
      -t bitcoin-linter \
      --file "./ci/lint_imagefile" \
      ./ || error "Docker build failed"

  echo "Running linter..."
  DOCKER_ARGS=(
    --rm
    -v "$(pwd):/bitcoin"
    -it
  )

  # Check if we are working in a worktree
  GIT_DIR=$(git rev-parse --git-dir)
  if [[ "$GIT_DIR" == *".git/worktrees/"* ]]; then
    # If we are, mount the git toplevel dir to the expected location.
    # This is required both so the linter knows which dir to run lint tests from.
    # A writeable mount is required so test/lint/commit-script-check.sh can
    # write to the index.
    echo "Detected git worktree..."
    WORKTREE_ROOT=$(echo "$GIT_DIR" | sed 's/\.git\/worktrees\/.*/\.git/')
    echo "Worktree root: $WORKTREE_ROOT"
    DOCKER_ARGS+=(--mount "type=bind,src=$WORKTREE_ROOT,dst=$WORKTREE_ROOT")
  fi

  RUST_BACKTRACE=1 docker run "${DOCKER_ARGS[@]}" bitcoin-linter
fi
