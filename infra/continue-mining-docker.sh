#!/usr/bin/env bash
#
# ItCoin
#
# This script is meant for debugging purposes. Assuming
# initialize-itcoin-docker.sh has been already executed once, restarts the
# continuous mining process from where it left off. The current datadir is
# reused: there is no need to delete it.
#
# Please note that you need to pass the BLOCKSCRIPT value via command line.
# You'll need to save it during the inizialization.
#
# REQUIREMENTS:
# - docker
# - jq
# - the itcoin docker image must be available and tagged
# - initialize-itcoin-docker.sh has been started and stopped already
#
# USAGE:
#     continue-mining-docker.sh <BLOCKSCRIPT>
#
# Author: muxator <antonio.muci@bancaditalia.it>

set -eu

# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself#246128
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# These should probably be taken from cmdline
ITCOIN_IMAGE_NAME="arthub.azurecr.io/itcoin-core"

# this uses the current checked out version. If you want to use a different
# version, you'll have to modify this variable, for now.
ITCOIN_IMAGE_TAG="git-"$("${MYDIR}"/compute-git-hash.sh)

CONTAINER_NAME="itcoin-node"
EXTERNAL_DATADIR="${MYDIR}/datadir"
INTERNAL_DATADIR="/opt/itcoin-core/datadir"

BITCOIN_PORT=38333
RPC_HOST=localhost
RPC_PORT=38332
ZMQ_PUBHASHTX_PORT=29010
ZMQ_PUBRAWBLOCK_PORT=29009

WALLET_NAME=itcoin_signer

errecho() {
    # prints to stderr
    >&2 echo $@;
}

cleanup() {
    # stop the itcoin daemon
    errecho "ItCoin daemon: cleaning up (deleting container ${CONTAINER_NAME})"
    docker stop "${CONTAINER_NAME}" > /dev/null
}

checkPrerequisites() {
    if ! command -v docker &> /dev/null; then
        errecho "Please install docker (https://www.docker.com/)"
        exit 1
    fi
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

# You will need to save the BLOCKSCRIPT value at first run in order to pass it here.
BLOCKSCRIPT=$1
shift

ITCOIN_IMAGE="${ITCOIN_IMAGE_NAME}:${ITCOIN_IMAGE_TAG}"
errecho "Using itcoin docker image ${ITCOIN_IMAGE}"

# Start itcoin daemon
# Different from the wiki: the wallet is not automatically loaded now. It will
# instead be loaded afterwards, through the cli
docker run \
	--read-only \
	--name "${CONTAINER_NAME}" \
	--user "$(id --user):$(id --group)" \
	--detach \
	--rm \
	--env BITCOIN_PORT="${BITCOIN_PORT}" \
	--env BLOCKSCRIPT="${BLOCKSCRIPT}" \
	--env RPC_HOST="${RPC_HOST}" \
	--env RPC_PORT="${RPC_PORT}" \
	--env ZMQ_PUBHASHTX_PORT="${ZMQ_PUBHASHTX_PORT}" \
	--env ZMQ_PUBRAWBLOCK_PORT="${ZMQ_PUBRAWBLOCK_PORT}" \
	--publish "${BITCOIN_PORT}":"${BITCOIN_PORT}" \
	--publish "${RPC_PORT}":"${RPC_PORT}" \
	--publish "${ZMQ_PUBHASHTX_PORT}":"${ZMQ_PUBHASHTX_PORT}" \
	--publish "${ZMQ_PUBRAWBLOCK_PORT}":"${ZMQ_PUBRAWBLOCK_PORT}" \
	--tmpfs /opt/itcoin-core/configdir \
	--mount type=bind,source="${EXTERNAL_DATADIR}",target="${INTERNAL_DATADIR}" \
	"${ITCOIN_IMAGE}" \
	bitcoind

# Open the wallet WALLET_NAME
errecho "Load wallet ${WALLET_NAME}"
"${MYDIR}/run-docker-bitcoin-cli.sh" loadwallet "${WALLET_NAME}" >/dev/null
errecho "Wallet ${WALLET_NAME} loaded"

# Retrieve the address of the first transaction in this blockchain
errecho "Retrieve the address of the first transaction we find"
ADDR=$("${MYDIR}/run-docker-bitcoin-cli.sh" listtransactions | jq --raw-output '.[0].address')
errecho "Address ${ADDR} retrieved"

# Let's start mining continuously. We'll reuse the same ADDR as before.
errecho "Keep mining eternally"
"${MYDIR}/run-docker-miner.sh" "${ADDR}" --ongoing
errecho "You should never reach here"
