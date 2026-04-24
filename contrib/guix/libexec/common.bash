#!/usr/bin/env bash
export LC_ALL=C
set -o errexit -o pipefail

# Usage: distsrc_for_host HOST [SUFFIX] [BASE]
#
#   HOST: The current platform triple we're building for
#   BASE: Optional. If provided, replaces ${DISTSRC_BASE}
#
distsrc_for_host() {
    echo "${3:-${DISTSRC_BASE}}/distsrc-${VERSION}-${1}${2:+-${2}}"
}

# Usage: outdir_for_host HOST [SUFFIX] [BASE]
#
#   HOST: The current platform triple we're building for
#   BASE: Optional. If provided, replaces ${OUTDIR_BASE}
#
outdir_for_host() {
    echo "${3:-${OUTDIR_BASE}}/${1}${2:+-${2}}"
}

# Usage: profiledir_for_host HOST [SUFFIX]
#
#   HOST: The current platform triple we're building for
#
profiledir_for_host() {
    echo "${PROFILES_BASE}/${1}${2:+-${2}}"
}
