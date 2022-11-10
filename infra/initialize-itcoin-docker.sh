#!/usr/bin/env bash
#
# ItCoin
#
# Starts a local itcoin daemon via Docker, computes the information
# necessary to initialize the system for the first time, prints on stdout the
# signet challenge (=blockscript) of the new blockchain and then mines the
# first block.
#
# Subsequent blocks can be mined calling continue-mining-docker.sh <BLOCKSTRIPT>
#
# The containers are run in host network mode, and thus the "--publish"
# arguments are not relevant. They are kept for documentation purposes, should
# we want to migrate to bridge networking mode.
#
# REQUIREMENTS:
# - docker
# - the itcoin docker image must be available and tagged
#
# USAGE:
#     initialize-itcoin-docker.sh [TIME_SHIFT]
#
#     TIME_SHIFT: how many minutes in the past should the first block date be.
#                 If you want to use the current time, set this to 0.
#                 Default: 120 minutes.
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
RPC_HOST=127.0.0.1 # localhost would fail if the system is ipv6-only
RPC_PORT=38332
ZMQ_PUBHASHTX_PORT=29010
ZMQ_PUBRAWBLOCK_PORT=29009

errecho() {
    # prints to stderr
    >&2 echo "${@}"
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
    if ! command -v sort &> /dev/null; then
        errecho "The sort command is not available"
        exit 1
    fi
    if ! command -v uniq &> /dev/null; then
        errecho "The uniq command is not available"
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

INIT_DATA=$(docker run \
    --rm \
    "${ITCOIN_IMAGE}" \
    create-initdata.sh
)

BLOCKSCRIPT=$(echo "${INIT_DATA}" | docker run --rm --interactive "${ITCOIN_IMAGE}" jq --raw-output '.blockscript')
DESCRIPTORS=$(echo "${INIT_DATA}" | docker run --rm --interactive "${ITCOIN_IMAGE}" jq --raw-output '.descriptors')

errecho "Creating datadir ${EXTERNAL_DATADIR}. If it already exists this script will fail"
mkdir "${EXTERNAL_DATADIR}"

# The ZMQ topics do not need to be published on distinct ports. But Docker's
# "--publish" parameter fails if the same port is given multiple times.
# Thus we have to remove duplicates from the set of ZMQ_XXX_PORT variables.
declare -a ZMQ_PARAMS
while IFS= read -r ZMQ_PORT; do
    ZMQ_PARAMS+=("--publish" "${ZMQ_PORT}:${ZMQ_PORT}")
done <<<$(printf '%s\n' "${ZMQ_PUBHASHTX_PORT}" "${ZMQ_PUBRAWBLOCK_PORT}" | sort | uniq )

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
	--network=host \
	--publish "${BITCOIN_PORT}":"${BITCOIN_PORT}" \
	--publish "${RPC_PORT}":"${RPC_PORT}" \
	"${ZMQ_PARAMS[@]}" \
	--tmpfs /opt/itcoin-core/configdir \
	--mount type=bind,source="${EXTERNAL_DATADIR}",target="${INTERNAL_DATADIR}" \
	"${ITCOIN_IMAGE}" \
	bitcoind

# Only the first time: let's create a blank descriptor wallet and call it
# itcoin_signer
errecho "Create a blank descriptor wallet itcoin_signer"
"${MYDIR}/run-docker-bitcoin-cli.sh" -named createwallet wallet_name=itcoin_signer descriptors=true blank=true >/dev/null
errecho "Wallet itcoin_signer created"

# Now we need to import the private descriptors previously created into
# itcoin_signer
errecho "Import private descriptors into itcoin_signer"
"${MYDIR}/run-docker-bitcoin-cli.sh" importdescriptors "${DESCRIPTORS}"
errecho "Private descriptors imported into itcoin_signer"

# Generate a bech32m address we'll send bitcoins to.
# Note that since we only imported tr descriptors, bech32m addresses are the
# only ones we can generate
errecho "Generate an address"
ADDR=$("${MYDIR}/run-docker-bitcoin-cli.sh" getnewaddress -addresstype bech32m)
errecho "Address ${ADDR} generated"

# Ask the miner to send bitcoins to that address. Being the first block in the
# chain, we need to choose a date. We'll use the current time minus the
# TIME_SHIFT command line parameter (or 120 if no value was given).
errecho "Mine the first block"

TIME_SHIFT="${1:-120}"
BLOCK_1_DATE=$(date --date "-${TIME_SHIFT} min" '+%s')

"${MYDIR}/run-docker-miner.sh" "${ADDR}" --set-block-time "${BLOCK_1_DATE}"
errecho "First block mined"

cat <<-EOF

	To continue the mining process please run:

	    ./continue-mining-docker.sh ${BLOCKSCRIPT}

	Remember to keep in a safe place the value of BLOCKSCRIPT (${BLOCKSCRIPT}),
	because it won't be saved anywhere.
EOF
