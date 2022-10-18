#!/usr/bin/env bash
#
# This is an EXAMPLE of how to run bitcoind in a container, with a configuration
# dynamically generated at runtime via environment variables.
#
# The containers are run in host network mode, and thus the "--publish"
# arguments are not relevant. They are kept for documentation purposes, should
# we want to migrate to bridge networking mode.

set -eu

# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself#246128
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

ITCOIN_IMAGE_NAME="arthub.azurecr.io/itcoin-core"

# this uses the current checked out version. If you want to use a different
# version, you'll have to modify this variable, for now.
ITCOIN_IMAGE_TAG="git-"$("${MYDIR}"/compute-git-hash.sh)

CONTAINER_NAME="itcoind"
EXTERNAL_DATADIR="${MYDIR}/datadir"
INTERNAL_DATADIR="/opt/itcoin-core/datadir"

BITCOIN_PORT=38333

# You can use any valid blockscript here.
#
# Just for fun, this is a 1-of-2 signet blockscript generated from this data:
# {
#   "privkey1": "cTQkCqrvsZsATw5pKDFZzrfy7bg2jtrdpLdyHnuq92WotPe6eZif",
#   "pubkey1": "0268b28e68c3263379cc70fd0711daa6647f2b4c86eb5ab9c91c6bbd0a77ab616f",
#   "privkey2": "cN1Gai7mRtaKmRrfKrpB86uGhioh4NKnb236HBx8f1jHgQ5jxB1T",
#   "pubkey2": "02482a952da354c4997e9e59bdb49a6ee2fb0214bb207e637557fc5e5beb201855",
#   "blockscript": "51210268b28e68c3263379cc70fd0711daa6647f2b4c86eb5ab9c91c6bbd0a77ab616f2102482a952da354c4997e9e59bdb49a6ee2fb0214bb207e637557fc5e5beb20185552ae"
# }
#
# Ideally, the BLOCKSCRIPT value should be generated via a proper procedure, and
# accepted as parameter by this script.
BLOCKSCRIPT="51210268b28e68c3263379cc70fd0711daa6647f2b4c86eb5ab9c91c6bbd0a77ab616f2102482a952da354c4997e9e59bdb49a6ee2fb0214bb207e637557fc5e5beb20185552ae"

RPC_HOST="NOT_RELEVANT"
RPC_PORT=38332
ZMQ_PUBHASHTX_PORT=29010
ZMQ_PUBRAWBLOCK_PORT=29009

errecho() {
    # prints to stderr
    >&2 echo "${@}"
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

# Do not run if the required packages are not installed
checkPrerequisites

ITCOIN_IMAGE="${ITCOIN_IMAGE_NAME}:${ITCOIN_IMAGE_TAG}"
errecho "Using itcoin docker image ${ITCOIN_IMAGE}"

# The ZMQ topics do not need to be published on distinct ports. But Docker's
# "--publish" parameter fails if the same port is given multiple times.
# Thus we have to remove duplicates from the set of ZMQ_XXX_PORT variables.
declare -a ZMQ_PARAMS
while IFS= read -r ZMQ_PORT; do
    ZMQ_PARAMS+=("--publish" "${ZMQ_PORT}:${ZMQ_PORT}")
done <<<$(printf '%s\n' "${ZMQ_PUBHASHTX_PORT}" "${ZMQ_PUBRAWBLOCK_PORT}" | sort | uniq )

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
	--network=host \
	--publish "${BITCOIN_PORT}":"${BITCOIN_PORT}" \
	--publish "${RPC_PORT}":"${RPC_PORT}" \
	"${ZMQ_PARAMS[@]}" \
	--tmpfs /opt/itcoin-core/configdir \
	--mount type=bind,source="${EXTERNAL_DATADIR}",target="${INTERNAL_DATADIR}" \
	"${ITCOIN_IMAGE}" \
	bitcoind "${@}"
