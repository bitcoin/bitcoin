#!/usr/bin/env bash
#
# ItCoin
#
# Starts an ephemeral local itcoin daemon, and computes the information
# necessary to perform a first initialization of the system.
# At script exit the temporary data is deleted and the itcoin daemon is killed.
#
# REQUIREMENTS:
# - jq
# - the itcoin source must already be built with /configure-itcoin.sh && make &&
#   make install
#
# USAGE:
#     create-keypair.sh
#
#     If you want to run this inside the itcoin container, do something along
#     the lines of:
#
#     docker run \
#         --rm \
#         arthub.azurecr.io/itcoin-core:git-abcdef \
#         create-keypair.sh
#
# Author: muxator <antonio.muci@bancaditalia.it>

set -eu
shopt -s inherit_errexit

errecho() {
    # prints to stderr
    >&2 echo "${@}"
}

cleanup() {
    # stop the ephemeral daemon
    "${PATH_TO_BINARIES}"/bitcoin-cli -datadir="${TMPDIR}" -regtest stop >/dev/null
    # destroy the temporary wallet.
    errecho "create-keypair: cleaning up (deleting temporary directory ${TMPDIR})"
    rm -rf "${TMPDIR}"
}

checkPrerequisites() {
    if ! command -v mktemp &> /dev/null; then
        echo "Please install mktemp"
        exit 1
    fi
    if ! command -v jq &> /dev/null; then
        echo "Please install jq (https://stedolan.github.io/jq/)"
        exit 1
    fi
}

# Automatically stop the container (wich will also self-remove at script exit
# or if an error occours
trap cleanup EXIT

# Do not run if the required packages are not installed
checkPrerequisites

# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself#246128
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Depending on whether we are running on bare metal or inside a container, the
# path to the itcoin binaries will vary.
#
# Let's check if "${MYDIR}/../target/bin" exists.
# - if it exists, we are running on bare metal, and the binaries will be at
#   "${MYDIR}/../target/bin";
# - if not, we are running inside a container, and the binaries reside in the
#   same directory containing this script.
PATH_TO_BINARIES=$(realpath --quiet --canonicalize-existing "${MYDIR}/../target/bin" || echo "${MYDIR}")

TMPDIR=$(mktemp --directory itcoin-temp-XXXX)

"${PATH_TO_BINARIES}"/bitcoind -daemon -datadir="${TMPDIR}" -regtest >/dev/null

errecho "create-keypair: waiting (at most 10 seconds) for itcoin daemon to warmup"
timeout 10 "${PATH_TO_BINARIES}"/bitcoin-cli -datadir="${TMPDIR}" -regtest -rpcwait -rpcclienttimeout=3 uptime >/dev/null
errecho "create-keypair: warmed up"

"${PATH_TO_BINARIES}"/bitcoin-cli -datadir="${TMPDIR}" -regtest createwallet mywallet >/dev/null

TMPADDR=$("${PATH_TO_BINARIES}"/bitcoin-cli -datadir="${TMPDIR}" -regtest getnewaddress)
PRIVKEY=$("${PATH_TO_BINARIES}"/bitcoin-cli -datadir="${TMPDIR}" -regtest dumpprivkey "${TMPADDR}")
PUBKEY=$("${PATH_TO_BINARIES}"/bitcoin-cli  -datadir="${TMPDIR}" -regtest getaddressinfo "${TMPADDR}" |  jq -r '.pubkey')

# For reference on how the stack-based script language works, see https://en.bitcoin.it/wiki/Script
BLOCKSCRIPT="5121${PUBKEY}51ae"

# 4. print out the results
echo "{ \"privkey\": \"${PRIVKEY}\", \"pubkey\": \"${PUBKEY}\", \"blockscript\": \"${BLOCKSCRIPT}\" }"
