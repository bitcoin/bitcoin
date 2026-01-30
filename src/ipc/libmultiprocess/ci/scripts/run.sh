#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -o errexit -o nounset -o pipefail -o xtrace

[ "${CI_CONFIG+x}" ] && source "$CI_CONFIG"

nix develop --ignore-environment --keep CI_CONFIG --keep CI_CLEAN "${NIX_ARGS[@]+"${NIX_ARGS[@]}"}" -f shell.nix --command ci/scripts/ci.sh
