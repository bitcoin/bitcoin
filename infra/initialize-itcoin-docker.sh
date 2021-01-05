#!/usr/bin/env bash
#
# ItCoin
#
# Starts a local itcoin daemon via Docker, computes the information
# necessary to initialize the system for the first time.
# Then starts continuously mining blocks at a regular pace.
#
# REQUIREMENTS:
# - docker
# - jq
# - the itcoin docker image must be available and tagged
#
# USAGE:
#     initialize-itcoin-docker.sh
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
INTERNAL_CONFIGDIR="/opt/itcoin-core/configdir"
EXTERNAL_DATADIR="${MYDIR}/datadir"
INTERNAL_DATADIR="/opt/itcoin-core/datadir"

BITCOIN_PORT=38333

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

ITCOIN_IMAGE="${ITCOIN_IMAGE_NAME}:${ITCOIN_IMAGE_TAG}"
errecho "Using itcoin docker image ${ITCOIN_IMAGE}"

KEYPAIR=$(docker run \
    --rm \
    "${ITCOIN_IMAGE}" \
    create-keypair.sh
)

BLOCKSCRIPT=$(echo "${KEYPAIR}" | jq --raw-output '.blockscript')
PRIVKEY=$(echo     "${KEYPAIR}" | jq --raw-output '.privkey')

errecho "Creating datadir ${EXTERNAL_DATADIR}. If it already exists this script will fail"
mkdir "${EXTERNAL_DATADIR}"

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
	--publish "${BITCOIN_PORT}":"${BITCOIN_PORT}" \
	--tmpfs "${INTERNAL_CONFIGDIR}" \
	--mount type=bind,source="${EXTERNAL_DATADIR}",target="${INTERNAL_DATADIR}" \
	"${ITCOIN_IMAGE}" \
	bitcoind

# wait until the daemon is fully started
errecho "ItCoin daemon: waiting (at most 10 seconds) for warmup"
timeout 10 docker exec "${CONTAINER_NAME}" bitcoin-cli -conf="${INTERNAL_CONFIGDIR}"/bitcoin.conf -datadir="${INTERNAL_DATADIR}" -rpcwait -rpcclienttimeout=3 uptime >/dev/null
errecho "ItCoin daemon: warmed up"

# Only the first time: let's create a wallet and call it itcoin_signer
errecho "Create wallet itcoin_signer"
docker exec "${CONTAINER_NAME}" bitcoin-cli -conf="${INTERNAL_CONFIGDIR}"/bitcoin.conf -datadir="${INTERNAL_DATADIR}" createwallet itcoin_signer >/dev/null
errecho "Wallet itcoin_signer created"

# Now we need to import inside itcoin_signer the private key we generated
# beforehand. This private key will be used to sign blocks.
errecho "Import private key into itcoin_signer"
docker exec "${CONTAINER_NAME}" bitcoin-cli -conf="${INTERNAL_CONFIGDIR}"/bitcoin.conf -datadir="${INTERNAL_DATADIR}" importprivkey "${PRIVKEY}"
errecho "Private key imported into itcoin_signer"

# Generate an address we'll send bitcoins to.
errecho "Generate an address"
ADDR=$(docker exec "${CONTAINER_NAME}" bitcoin-cli -conf="${INTERNAL_CONFIGDIR}"/bitcoin.conf -datadir="${INTERNAL_DATADIR}" getnewaddress)
errecho "Address ${ADDR} generated"

# Ask the miner to send bitcoins to that address. Being the first block in the
# chain, we need to choose a date. We'll use "-1", which means "current time".
errecho "Mine the first block"
docker exec "${CONTAINER_NAME}" miner --cli="bitcoin-cli -conf=${INTERNAL_CONFIGDIR}/bitcoin.conf -datadir=${INTERNAL_DATADIR}" generate --address "${ADDR}" --grind-cmd='bitcoin-util grind' --min-nbits --set-block-time -1
errecho "First block mined"

# Let's start mining continuously. We'll reuse the same ADDR as before.
errecho "Keep mining the following blocks"
docker exec "${CONTAINER_NAME}" miner --cli="bitcoin-cli -conf=${INTERNAL_CONFIGDIR}/bitcoin.conf -datadir=${INTERNAL_DATADIR}" generate --address "${ADDR}" --grind-cmd='bitcoin-util grind' --min-nbits --ongoing
errecho "You should never reach here"
