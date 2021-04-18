#!/usr/bin/env bash
#
# Copyright (c) 2019 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
export LC_ALL=C

set -ueo pipefail

if (( $# < 3 )); then
  echo 'Usage: utxo_snapshot.sh <generate-at-height> <snapshot-out-path> <widecoin-cli-call ...>'
  echo
  echo "  if <snapshot-out-path> is '-', don't produce a snapshot file but instead print the "
  echo "  expected assumeutxo hash"
  echo
  echo 'Examples:'
  echo
  echo "  ./contrib/devtools/utxo_snapshot.sh 570000 utxo.dat ./src/widecoin-cli -datadir=\$(pwd)/testdata"
  echo '  ./contrib/devtools/utxo_snapshot.sh 570000 - ./src/widecoin-cli'
  exit 1
fi

GENERATE_AT_HEIGHT="${1}"; shift;
OUTPUT_PATH="${1}"; shift;
# Most of the calls we make take a while to run, so pad with a lengthy timeout.
WIDECOIN_CLI_CALL="${*} -rpcclienttimeout=9999999"

# Block we'll invalidate/reconsider to rewind/fast-forward the chain.
PIVOT_BLOCKHASH=$($WIDECOIN_CLI_CALL getblockhash $(( GENERATE_AT_HEIGHT + 1 )) )

(>&2 echo "Rewinding chain back to height ${GENERATE_AT_HEIGHT} (by invalidating ${PIVOT_BLOCKHASH}); this may take a while")
${WIDECOIN_CLI_CALL} invalidateblock "${PIVOT_BLOCKHASH}"

if [[ "${OUTPUT_PATH}" = "-" ]]; then
  (>&2 echo "Generating txoutset info...")
  ${WIDECOIN_CLI_CALL} gettxoutsetinfo | grep hash_serialized_2 | sed 's/^.*: "\(.\+\)\+",/\1/g'
else
  (>&2 echo "Generating UTXO snapshot...")
  ${WIDECOIN_CLI_CALL} dumptxoutset "${OUTPUT_PATH}"
fi

(>&2 echo "Restoring chain to original height; this may take a while")
${WIDECOIN_CLI_CALL} reconsiderblock "${PIVOT_BLOCKHASH}"
