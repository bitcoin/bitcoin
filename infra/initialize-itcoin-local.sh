#!/usr/bin/env bash
#
# ItCoin
#
# Starts a local itcoin daemon, computes the information necessary to initialize
# the system for the first time and mines the first block of a new blockchain.
#
# Subsequent blocks can be mined calling continue-mining-local.sh
#
# REQUIREMENTS:
# - jq
# - the itcoin source must already be built with:
#      ./configure-itcoin.sh && \
#          make --jobs=$(nproc --ignore=1) --max-load=$(nproc --ignore=1) && \
#          make install-strip
#
# USAGE:
#     initialize-itcoin-local.sh [TIME_SHIFT]
#
#     TIME_SHIFT: how many minutes in the past should the first block date be.
#                 If you want to use the current time, set this to 0.
#                 Default: 120 minutes.
#
# Author: muxator <antonio.muci@bancaditalia.it>

set -eu

# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself#246128
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Update LD_LIBRARY_PATH to include boost libraries
export LD_LIBRARY_PATH=${MYDIR}/../../itcoin-pbft/usrlocal/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}

# These should probably be taken from cmdline
DATADIR="${MYDIR}/datadir"
export BITCOIN_PORT=38333
export RPC_PORT=38332
export ZMQ_PUBHASHTX_PORT=29009
export ZMQ_PUBRAWBLOCK_PORT=29009
export ZMQ_PUBITCOINBLOCK_PORT=29010

errecho() {
    # prints to stderr
    >&2 echo "${@}"
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

INIT_DATA=$("${MYDIR}/create-initdata.sh")

export BLOCKSCRIPT=$(echo "${INIT_DATA}" | jq --raw-output '.blockscript')
DESCRIPTORS=$(echo        "${INIT_DATA}" | jq --raw-output '.descriptors')

errecho "Creating datadir ${DATADIR}. If it already exists this script will fail"
mkdir "${DATADIR}"

# creare il file di configurazione del demone bitcoin
# Lo facciamo partire su signet e il signetchallenge varrà BLOCKSCRIPT.
"${MYDIR}/render-template.sh" BLOCKSCRIPT BITCOIN_PORT RPC_PORT ZMQ_PUBHASHTX_PORT ZMQ_PUBRAWBLOCK_PORT ZMQ_PUBITCOINBLOCK_PORT < "${MYDIR}/bitcoin.conf.tmpl" > "${DATADIR}/bitcoin.conf"

# Start itcoin daemon
# Different from the wiki: the wallet is not automatically loaded now. It will
# instead be loaded afterwards, through the cli
"${BITCOIND}" -daemon -datadir="${DATADIR}"

# wait until the daemon is fully started
errecho "ItCoin daemon: waiting (at most 10 seconds) for warmup"
timeout 10 "${BITCOIN_CLI}" -datadir="${DATADIR}" -rpcwait -rpcclienttimeout=3 uptime >/dev/null
errecho "ItCoin daemon: warmed up"

# Only the first time: let's create a blank descriptor wallet and call it
# itcoin_signer
errecho "Create a blank descriptor wallet itcoin_signer"
"${BITCOIN_CLI}" -datadir="${DATADIR}" -named createwallet wallet_name=itcoin_signer descriptors=true blank=true >/dev/null
errecho "Wallet itcoin_signer created"

# Now we need to import the private descriptors previously created into
# itcoin_signer
errecho "Import private descriptors into itcoin_signer"
"${BITCOIN_CLI}" -datadir="${DATADIR}" importdescriptors "${DESCRIPTORS}"
errecho "Private descriptors imported into itcoin_signer"

# Generate a bech32m address we'll send bitcoins to.
# Note that since we only imported tr descriptors, bech32m addresses are the
# only ones we can generate
errecho "Generate an address"
ADDR=$("${BITCOIN_CLI}" -datadir="${DATADIR}" getnewaddress -addresstype bech32m)
errecho "Address ${ADDR} generated"

# Ask the miner to send bitcoins to that address. Being the first block in the
# chain, we need to choose a date. We'll use the current time minus the
# TIME_SHIFT command line parameter (or 120 if no value was given).
errecho "Mine the first block"

TIME_SHIFT="${1:-120}"
BLOCK_1_DATE=$(date --date "-${TIME_SHIFT} min" '+%s')

"${MINER}" --cli="${BITCOIN_CLI} -datadir=${DATADIR}" generate --address "${ADDR}" --grind-cmd="${BITCOIN_UTIL} grind" --min-nbits --set-block-time "${BLOCK_1_DATE}"
errecho "First block mined"

cat <<-EOF

	To continue the mining process please run:

	    ./continue-mining-local.sh
EOF
