#!/usr/bin/env bash
#
# This is an EXAMPLE of how to run bitcoin-cli in a container, connecting to a
# bitcoin daemon running on a separate container.
#
# The bitcoin daemon container is always assumed to be 127.0.0.1. In production,
# the address exposed by the container orchestration method should be used
# instead.
#
# Like bitcoind, this script shows that the container entrypoint is able to
# dynamically configure the software components using environment variables.
#
# The containers are run in host network mode, and thus the "--publish"
# arguments are not relevant. They are kept for documentation purposes, should
# we want to migrate to bridge networking mode.
#
# USAGE:
#     ./run-bitcoin-cli.sh <COMMAND> [parameters ...]
#
# SAMPLE:
#     ./run-bitcoin-cli.sh -help
#     ./run-bitcoin-cli.sh uptime

set -eu

# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself#246128
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

ITCOIN_IMAGE_NAME="arthub.azurecr.io/itcoin-core"

# this uses the current checked out version. If you want to use a different
# version, you'll have to modify this variable, for now.
ITCOIN_IMAGE_TAG="git-"$("${MYDIR}"/compute-git-hash.sh)

CONTAINER_NAME="itcoin-cli"
EXTERNAL_DATADIR="${MYDIR}/datadir"
INTERNAL_DATADIR="/opt/itcoin-core/datadir"

BITCOIN_PORT=38333

# BLOCKSCRIPT is not relevant for bitcoin-cli
BLOCKSCRIPT="NOT_RELEVANT"

RPC_HOST=127.0.0.1 # localhost would fail if the system is ipv6-only
RPC_PORT=38332

# ZMQ_PUBHASHTX_PORT is not relevant for bitcoin-cli
ZMQ_PUBHASHTX_PORT="NOT_RELEVANT"

# ZMQ_PUBRAWBLOCK_PORT is not relevant for bitcoin-cli
ZMQ_PUBRAWBLOCK_PORT="NOT_RELEVANT"

# ZMQ_PUBITCOINBLOCK_PORT is not relevant for bitcoin-cli
ZMQ_PUBITCOINBLOCK_PORT="NOT_RELEVANT"

errecho() {
    # prints to stderr
    >&2 echo "${@}"
}

ITCOIN_IMAGE="${ITCOIN_IMAGE_NAME}:${ITCOIN_IMAGE_TAG}"
errecho "Using itcoin docker image ${ITCOIN_IMAGE}"

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
	--env ZMQ_PUBITCOINBLOCK_PORT="${ZMQ_PUBITCOINBLOCK_PORT}" \
	--network=host \
	--tmpfs /opt/itcoin-core/configdir \
	--mount type=bind,source="${EXTERNAL_DATADIR}",target="${INTERNAL_DATADIR}",readonly \
	"${ITCOIN_IMAGE}" \
	bitcoin-cli "${@}"
