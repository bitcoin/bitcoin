#!/usr/bin/env bash
export LC_ALL=C
set -o errexit -o pipefail

# Usage: outdir_for_host HOST [SUFFIX]
#
#   HOST: The current platform triple we're building for
#
outdir_for_host() {
    echo "${OUTDIR_BASE}/${1}${2:+-${2}}"
}
