#!/usr/bin/env bash
#
# ItCoin
#
# Starts an ephemeral itcoin daemon via Docker, and computes the information
# necessary to perform a first initialization of the system.
#
# REQUIREMENTS:
# - docker
# - jq
# - the itcoin docker image must be available and tagged
#
# USAGE:
#     create-keypair-docker.sh
#
# Author: muxator <antonio.muci@bancaditalia.it>

set -eu

ITCOIN_IMAGE="itcoin:git-bc177a0bd9e4"
EPHEMERAL_DAEMON="itcoin-ephemeral-daemon"
JSON_RPC_REGTEST_PORT=18443

errecho() {
    # prints to stderr
    >&2 echo $@;
}

cleanup() {
    # stop the ephemeral daemon and destroy the temporary wallet.
    errecho "ItCoin daemon: cleaning up (deleting container ${EPHEMERAL_DAEMON})"
    docker stop "${EPHEMERAL_DAEMON}" > /dev/null
}

checkPrerequisites() {
    if ! command -v docker &> /dev/null; then
        echo "Please install docker (https://www.docker.com/)"
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

# 1. run an ephemeral itcoin daemon to build the keys
errecho "ItCoin daemon: trying to start image ${ITCOIN_IMAGE} in container ${EPHEMERAL_DAEMON}"
docker run --name "${EPHEMERAL_DAEMON}" --detach --rm "${ITCOIN_IMAGE}" bitcoind -datadir=/tmp/ -regtest > /dev/null

# wait until the daemon is fully started
errecho "ItCoin daemon: waiting (at most 10 seconds) for ${EPHEMERAL_DAEMON} to warmup"
timeout 10 docker exec "${EPHEMERAL_DAEMON}" bitcoin-cli -datadir=/tmp -regtest -rpcwait -rpcclienttimeout=3 uptime >/dev/null
errecho "ItCoin daemon: warmed up"

# 2. create (and implicitly load) an empty wallet
docker exec "${EPHEMERAL_DAEMON}" bitcoin-cli -datadir=/tmp -regtest createwallet mywallet > /dev/null

# 3. compute the data we need
TMPADDR=$(docker exec "${EPHEMERAL_DAEMON}" bitcoin-cli -datadir=/tmp -regtest getnewaddress)
PRIVKEY=$(docker exec "${EPHEMERAL_DAEMON}" bitcoin-cli -datadir=/tmp -regtest dumpprivkey "${TMPADDR}")
PUBKEY=$(docker exec "${EPHEMERAL_DAEMON}" bitcoin-cli -datadir=/tmp -regtest getaddressinfo "${TMPADDR}" |  jq -r '.pubkey')

# For reference on how the stack-based script language works, see https://en.bitcoin.it/wiki/Script
BLOCKSCRIPT="5121${PUBKEY}51ae"

# 4. print out the results
echo "{ \"privkey\": \"${PRIVKEY}\", \"pubkey\": \"${PUBKEY}\", \"blockscript\": \"${BLOCKSCRIPT}\" }"
