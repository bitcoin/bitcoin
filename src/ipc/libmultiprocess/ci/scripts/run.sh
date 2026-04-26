#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -o errexit -o nounset -o pipefail -o xtrace

[ "${CI_CONFIG+x}" ] && source "$CI_CONFIG"

nix develop --ignore-environment --keep CI_CONFIG --keep CI_CLEAN "${NIX_ARGS[@]+"${NIX_ARGS[@]}"}" -f shell.nix --command ci/scripts/ci.sh

# Create a GC root for the shell closure so the cache-nix-action save step
# does not garbage-collect it.
if [ -n "${CI_CACHE_NIX_STORE-}" ]; then
  nix-build shell.nix \
    -o "$CI_DIR/gcroot" \
    "${NIX_ARGS[@]+"${NIX_ARGS[@]}"}"
  # Verify the closure is complete so the cache-nix-action save step has
  # everything it needs.
  nix-store --query --requisites "$CI_DIR/gcroot" >/dev/null
fi
