#!/usr/bin/env bash
#
# This is an EXAMPLE of how to run itcoin's miner in a container, connecting to
# a bitcoin daemon running on a separate container.
#
# Like bitcoind, this script shows that the container entrypoint is able to
# dynamically configure the software components using environment variables.
#
# USAGE:
#     ./run-miner.sh ADDRESS < --set-block-time -1 | --ongoing >
#
# Author: muxator <antonio.muci@bancaditalia.it>

set -eu

# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself#246128
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

ITCOIN_IMAGE_NAME="arthub.azurecr.io/itcoin-core"

# this uses the current checked out version. If you want to use a different
# version, you'll have to modify this variable, for now.
ITCOIN_IMAGE_TAG="git-"$("${MYDIR}"/compute-git-hash.sh)

CONTAINER_NAME="itcoin-miner"
EXTERNAL_DATADIR="${MYDIR}/datadir"
INTERNAL_DATADIR="/opt/itcoin-core/datadir"

BITCOIN_PORT=38333

# BLOCKSCRIPT is not relevant for miner
BLOCKSCRIPT="NOT_RELEVANT"

RPC_HOST=$(hostname)
RPC_PORT=38332

# ZMQ_PUBHASHTX_PORT is not relevant for miner
ZMQ_PUBHASHTX_PORT="NOT_RELEVANT"

# ZMQ_PUBRAWBLOCK_PORT is not relevant for miner
ZMQ_PUBRAWBLOCK_PORT="NOT_RELEVANT"

errecho() {
    # prints to stderr
    >&2 echo $@;
}

ITCOIN_IMAGE="${ITCOIN_IMAGE_NAME}:${ITCOIN_IMAGE_TAG}"
errecho "Using itcoin docker image ${ITCOIN_IMAGE}"

ADDRESS=$1
shift

docker run \
	--read-only \
	--name "${CONTAINER_NAME}" \
	--user "$(id --user):$(id --group)" \
	--rm \
	--env BITCOIN_PORT="${BITCOIN_PORT}" \
	--env BLOCKSCRIPT="${BLOCKSCRIPT}" \
	--env RPC_HOST="${RPC_HOST}" \
	--env RPC_PORT="${RPC_PORT}" \
	--env ZMQ_PUBHASHTX_PORT="${ZMQ_PUBHASHTX_PORT}" \
	--env ZMQ_PUBRAWBLOCK_PORT="${ZMQ_PUBRAWBLOCK_PORT}" \
	--tmpfs /opt/itcoin-core/configdir \
	--mount type=bind,source="${EXTERNAL_DATADIR}",target="${INTERNAL_DATADIR}",readonly \
	"${ITCOIN_IMAGE}" \
	miner \
		generate \
		--address "${ADDRESS}" \
		--grind-cmd='bitcoin-util grind' \
		--min-nbits \
		"${@}"
