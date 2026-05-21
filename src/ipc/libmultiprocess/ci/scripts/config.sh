#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Source CI configuration and output variables needed by the workflow.

export LC_ALL=C

set -o errexit -o nounset -o pipefail -o xtrace

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

source "${SCRIPT_DIR}/ci_helpers.sh"

[ "${CI_CONFIG+x}" ] && source "$CI_CONFIG"

# Resolve the nixpkgs channel to a specific revision for use in cache keys.
if [[ -n "${NIXPKGS_CHANNEL:-}" ]]; then
  rev="$(curl --fail --location --silent --show-error "https://channels.nixos.org/${NIXPKGS_CHANNEL}/git-revision")"
  test -n "${rev}"
  write_output_var nixpkgs_rev "${rev}"
fi

write_output_var cache_nix_store "${CI_CACHE_NIX_STORE:-false}"
