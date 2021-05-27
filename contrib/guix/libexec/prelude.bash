#!/usr/bin/env bash
export LC_ALL=C
set -e -o pipefail

# shellcheck source=../../shell/realpath.bash
source contrib/shell/realpath.bash

# shellcheck source=../../shell/git-utils.bash
source contrib/shell/git-utils.bash

################
# Required non-builtin commands should be invocable
################

check_tools() {
    for cmd in "$@"; do
        if ! command -v "$cmd" > /dev/null 2>&1; then
            echo "ERR: This script requires that '$cmd' is installed and available in your \$PATH"
            exit 1
        fi
    done
}

check_tools cat env readlink dirname basename git

################
# We should be at the top directory of the repository
################

same_dir() {
    local resolved1 resolved2
    resolved1="$(bash_realpath "${1}")"
    resolved2="$(bash_realpath "${2}")"
    [ "$resolved1" = "$resolved2" ]
}

if ! same_dir "${PWD}" "$(git_root)"; then
cat << EOF
ERR: This script must be invoked from the top level of the git repository

Hint: This may look something like:
    env FOO=BAR ./contrib/guix/guix-<blah>

EOF
exit 1
fi

################
# Set common variables
################

VERSION="${VERSION:-$(git_head_version)}"
DISTNAME="${DISTNAME:-bitcoin-${VERSION}}"

version_base_prefix="${PWD}/guix-build-"
VERSION_BASE="${version_base_prefix}${VERSION}"  # TOP

DISTSRC_BASE="${DISTSRC_BASE:-${VERSION_BASE}}"

OUTDIR_BASE="${OUTDIR_BASE:-${VERSION_BASE}/output}"

var_base_basename="var"
VAR_BASE="${VAR_BASE:-${VERSION_BASE}/${var_base_basename}}"

profiles_base_basename="profiles"
PROFILES_BASE="${PROFILES_BASE:-${VAR_BASE}/${profiles_base_basename}}"
