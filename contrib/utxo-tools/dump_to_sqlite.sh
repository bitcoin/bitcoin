#!/usr/bin/env bash
# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
set -e

if [ $# -ne 2 ]; then
    echo "Usage: $0 <bitcoin-cli-path> <output-file>"
    exit 1
fi

BITCOIN_CLI=$1
OUTPUT_FILE=$2
UTXO_TO_SQLITE=$(dirname "$0")/utxo_to_sqlite.py

# create named pipe in unique temporary folder
TEMPPATH=$(mktemp -d)
FIFOPATH=$TEMPPATH/utxos.fifo
mkfifo "$FIFOPATH"

# start dumping UTXO set to the pipe in background
$BITCOIN_CLI dumptxoutset "$FIFOPATH" latest &
BITCOIN_CLI_PID=$!

# start UTXO to SQLite conversion tool, reading from pipe
$UTXO_TO_SQLITE "$FIFOPATH" "$OUTPUT_FILE"

# wait and cleanup
wait $BITCOIN_CLI_PID
rm -r "$TEMPPATH"
