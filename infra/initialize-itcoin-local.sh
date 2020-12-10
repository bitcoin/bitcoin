#!/usr/bin/env bash
#
# ItCoin
#
# Starts a local itcoin daemon, computes the information necessary to initialize
# the system for the first time.
# Then starts continuously mining blocks at a regular pace.
#
# REQUIREMENTS:
# - jq
# - the itcoin source must already be built with /configure-itcoin.sh && make &&
#   make install
#
# USAGE:
#     initialize-itcoin-local.sh
#
# Author: muxator <antonio.muci@bancaditalia.it>

set -eu

# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself#246128
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# These should probably be taken from cmdline
DATADIR="${MYDIR}/datadir"
TARGET_DELAY=60

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
MINER=$(realpath "${MYDIR}/../contrib/signet/miner")

KEYPAIR=$("${MYDIR}/create-keypair-local.sh")

BLOCKSCRIPT=$(echo "${KEYPAIR}" | jq --raw-output '.blockscript')
PRIVKEY=$(echo     "${KEYPAIR}" | jq --raw-output '.privkey')

errecho "Creating datadir ${DATADIR}. If it already exists this script will fail"
mkdir "${DATADIR}"

# creare il file di configurazione del demone bitcoin
# Lo facciamo partire su signet e il signetchallenge varrà BLOCKSCRIPT.
cat << EOF > "${DATADIR}/bitcoin.conf"
# ITCOIN configuration

signet=1

[signet]
signetchallenge = ${BLOCKSCRIPT}
EOF

# Start itcoin daemon
# Different from the wiki: the wallet is not automatically loaded now. It will
# instead be loaded afterwards, through the cli
"${BITCOIND}" -daemon -datadir="${DATADIR}"

# wait until the daemon is fully started
errecho "ItCoin daemon: waiting (at most 10 seconds) for warmup"
timeout 10 "${BITCOIN_CLI}" -datadir="${DATADIR}" -rpcwait -rpcclienttimeout=3 uptime >/dev/null
errecho "ItCoin daemon: warmed up"

# Only the first time: let's create a wallet and call it itcoin_signer
errecho "Create wallet itcoin_signer"
"${BITCOIN_CLI}" -datadir="${DATADIR}" createwallet itcoin_signer >/dev/null
errecho "Wallet itcoin_signer created"

# Now we need to import inside itcoin_signer the private key we generated
# beforehand. This private key will be used to sign blocks.
errecho "Import private key into itcoin_signer"
"${BITCOIN_CLI}" -datadir="${DATADIR}" importprivkey "${PRIVKEY}"
errecho "Private key imported into itcoin_signer"

# Estimate the value for nbit parameter which is necessary to generate blocks at
# 5 seconds pace on the hardware we are running now.
# The NBITS variable will be an empty string if the regex does not match.
errecho "Calibrating difficulty for target delay ${TARGET_DELAY} seconds"
NBITS=$("${MINER}" calibrate --grind-cmd="${BITCOIN_UTIL} grind" --seconds "${TARGET_DELAY}" | sed --regexp-extended --quiet 's/nbits=(.*) for (.*)s average mining time/\1/p')
errecho "Difficulty for target delay ${TARGET_DELAY} seconds calibrated. nbits=${NBITS}"

# Generate an address we'll send bitcoins to.
errecho "Generate an address"
ADDR=$("${BITCOIN_CLI}" -datadir="${DATADIR}" getnewaddress)
errecho "Address ${ADDR} generated"

# Ask the miner to send bitcoins to that address. Being the first block in the
# chain, we need to choose a date. We'll use "-1", which means "current time".
errecho "Mine the first block"
"${MINER}" --cli="${BITCOIN_CLI} -datadir=${DATADIR}" generate --address "${ADDR}" --grind-cmd="${BITCOIN_UTIL} grind" --nbits="${NBITS}" --set-block-time -1
errecho "First block mined"

# Let's start mining continuously. We'll reuse the same ADDR as before.
errecho "Keep mining the following blocks"
"${MINER}" --cli="${BITCOIN_CLI} -datadir=${DATADIR}" generate --address "${ADDR}" --grind-cmd="${BITCOIN_UTIL} grind" --nbits="${NBITS}" --ongoing
errecho "You should never reach here"
