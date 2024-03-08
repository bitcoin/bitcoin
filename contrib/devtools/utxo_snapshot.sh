#!/usr/bin/env bash
#
# Copyright (c) 2019-2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
export LC_ALL=C

set -ueo pipefail

NETWORK_DISABLED=false

if (( $# < 3 )); then
  echo 'Usage: utxo_snapshot.sh <generate-at-height> <snapshot-out-path> <bitcoin-cli-call ...>'
  echo
  echo "  if <snapshot-out-path> is '-', don't produce a snapshot file but instead print the "
  echo "  expected assumeutxo hash"
  echo
  echo 'Examples:'
  echo
  echo "  ./contrib/devtools/utxo_snapshot.sh 570000 utxo.dat ./src/bitcoin-cli -datadir=\$(pwd)/testdata"
  echo '  ./contrib/devtools/utxo_snapshot.sh 570000 - ./src/bitcoin-cli'
  exit 1
fi

GENERATE_AT_HEIGHT="${1}"; shift;
OUTPUT_PATH="${1}"; shift;
# Most of the calls we make take a while to run, so pad with a lengthy timeout.
BITCOIN_CLI_CALL="${*} -rpcclienttimeout=9999999"

# Check if the node is pruned and get the pruned block height
PRUNED=$( ${BITCOIN_CLI_CALL} getblockchaininfo | awk '/pruneheight/ {print $2}' | tr -d ',' )

if (( GENERATE_AT_HEIGHT < PRUNED )); then
  echo "Error: The requested snapshot height (${GENERATE_AT_HEIGHT}) should be greater than the pruned block height (${PRUNED})."
  exit 1
fi

# Early exit if file at OUTPUT_PATH already exists
if [[ -e "$OUTPUT_PATH" ]]; then
  (>&2 echo "Error: $OUTPUT_PATH already exists or is not a valid path.")
  exit 1
fi

# Validate that the path is correct
if [[ "${OUTPUT_PATH}" != "-" && ! -d "$(dirname "${OUTPUT_PATH}")" ]]; then
  (>&2 echo "Error: The directory $(dirname "${OUTPUT_PATH}") does not exist.")
  exit 1
fi

function cleanup {
  (>&2 echo "Restoring chain to original height; this may take a while")
  ${BITCOIN_CLI_CALL} reconsiderblock "${PIVOT_BLOCKHASH}"

  if $NETWORK_DISABLED; then
    (>&2 echo "Restoring network activity")
    ${BITCOIN_CLI_CALL} setnetworkactive true
  fi
}

function early_exit {
  (>&2 echo "Exiting due to Ctrl-C")
  cleanup
  exit 1
}

# Prompt the user to disable network activity
read -p "Do you want to disable network activity (setnetworkactive false) before running invalidateblock? (Y/n): " -r
if [[ "$REPLY" =~ ^[Yy]*$ || -z "$REPLY" ]]; then
  # User input is "Y", "y", or Enter key, proceed with the action
  NETWORK_DISABLED=true
  (>&2 echo "Disabling network activity")
  ${BITCOIN_CLI_CALL} setnetworkactive false
else
  (>&2 echo "Network activity remains enabled")
fi

# Block we'll invalidate/reconsider to rewind/fast-forward the chain.
PIVOT_BLOCKHASH=$($BITCOIN_CLI_CALL getblockhash $(( GENERATE_AT_HEIGHT + 1 )) )

# Trap for normal exit and Ctrl-C
trap cleanup EXIT
trap early_exit INT

(>&2 echo "Rewinding chain back to height ${GENERATE_AT_HEIGHT} (by invalidating ${PIVOT_BLOCKHASH}); this may take a while")
${BITCOIN_CLI_CALL} invalidateblock "${PIVOT_BLOCKHASH}"

if [[ "${OUTPUT_PATH}" = "-" ]]; then
  (>&2 echo "Generating txoutset info...")
  ${BITCOIN_CLI_CALL} gettxoutsetinfo | grep hash_serialized_3 | sed 's/^.*: "\(.\+\)\+",/\1/g'
else
  (>&2 echo "Generating UTXO snapshot...")
  ${BITCOIN_CLI_CALL} dumptxoutset "${OUTPUT_PATH}"
fi
