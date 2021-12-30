#!/usr/bin/env bash
#
# Copyright (c) 2017-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# This script runs all contrib/devtools/lint-*.sh files, and fails if any exit
# with a non-zero status code.

# This script is intentionally locale dependent by not setting "export LC_ALL=C"
# in order to allow for the executed lint scripts to opt in or opt out of locale
# dependence themselves.

set -u

SCRIPTDIR=$(dirname "${BASH_SOURCE[0]}")
LINTALL=$(basename "${BASH_SOURCE[0]}")

EXIT_CODE=0

if ! command -v parallel > /dev/null; then
  for f in "${SCRIPTDIR}"/lint-*.sh; do
    if [ "$(basename "$f")" != "$LINTALL" ]; then
      if ! "$f"; then
        echo "^---- failure generated from $f"
        EXIT_CODE=1
      fi
    fi
  done
else
  SCRIPTS=()

  for f in "${SCRIPTDIR}"/lint-*.sh; do
    if [ "$(basename "$f")" != "$LINTALL" ]; then
      SCRIPTS+=("$f")
    fi
  done

  if ! parallel --jobs 100% --will-cite --joblog parallel_out.log bash ::: "${SCRIPTS[@]}"; then
    echo "^---- failure generated"
    EXIT_CODE=1
  fi
  column -t parallel_out.log && rm parallel_out.log
fi

exit ${EXIT_CODE}
