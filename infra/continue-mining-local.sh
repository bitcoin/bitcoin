#!/usr/bin/env bash
#
# ItCoin
#
# This script is meant for debugging purposes. Assuming
# initialize-itcoin-local.sh has been already executed once, restarts the
# continuous mining process from where it left off. The current datadir is
# reused: there is no need to delete it.
#
# REQUIREMENTS:
# - jq
# - the itcoin source must already be built with <basedir>/configure-itcoin.sh
#   && make && make install
# - initialize-itcoin-local.sh has been started and stopped already
#
# USAGE:
#     continue-mining-local.sh
#
# Author: muxator <antonio.muci@bancaditalia.it>

set -eu

# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself#246128
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# These should probably be taken from cmdline
DATADIR="${MYDIR}/datadir"
WALLET_NAME=itcoin_signer

errecho() {
    # prints to stderr
    >&2 echo $@;
}

cleanup() {
    # stop the itcoin daemon
    "${BITCOIN_CLI}" -datadir="${DATADIR}" stop
}

checkPrerequisites() {
    if ! command -v jq &> /dev/null; then
        errecho "Please install jq (https://stedolan.github.io/jq/)"
        exit 1
    fi
}

# Automatically stop the container (wich will also self-remove at script exit
# or if an error occours
trap cleanup EXIT

# Do not run if the required packages are not installed
checkPrerequisites

PATH_TO_BINARIES=$(realpath "${MYDIR}/../target/bin")

BITCOIND="${PATH_TO_BINARIES}/bitcoind"
BITCOIN_CLI="${PATH_TO_BINARIES}/bitcoin-cli"
BITCOIN_UTIL="${PATH_TO_BINARIES}/bitcoin-util"
MINER="${PATH_TO_BINARIES}/miner"

# Start itcoin daemon
"${BITCOIND}" -daemon -datadir="${DATADIR}"

# wait until the daemon is fully started
errecho "ItCoin daemon: waiting (at most 10 seconds) for warmup"
timeout 10 "${BITCOIN_CLI}" -datadir="${DATADIR}" -rpcwait -rpcclienttimeout=3 uptime >/dev/null
errecho "ItCoin daemon: warmed up"

# Open the wallet WALLET_NAME
errecho "Load wallet ${WALLET_NAME}"
"${BITCOIN_CLI}" -datadir="${DATADIR}" loadwallet "${WALLET_NAME}" >/dev/null
errecho "Wallet ${WALLET_NAME} loaded"

# Retrieve the address of the first transaction in this blockchain
errecho "Retrieve the address of the first transaction we find"
ADDR=$("${BITCOIN_CLI}" -datadir="${DATADIR}" listtransactions | jq --raw-output '.[0].address')
errecho "Address ${ADDR} retrieved"

# Let's start mining continuously. We'll reuse the same ADDR as before.
errecho "Keep mining eternally"
"${MINER}" --cli="${BITCOIN_CLI} -datadir=${DATADIR}" generate --address "${ADDR}" --grind-cmd="${BITCOIN_UTIL} grind" --min-nbits --ongoing
errecho "You should never reach here"
