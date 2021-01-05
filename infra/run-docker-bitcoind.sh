#!/usr/bin/env bash
#
# This is an EXAMPLE of how to run bitcoind in a container, with a configuration
# dynamically generated at runtime via environment variables.

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

errecho() {
    # prints to stderr
    >&2 echo $@;
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
	--publish "${BITCOIN_PORT}":"${BITCOIN_PORT}" \
	--tmpfs /opt/itcoin-core/configdir \
	--mount type=bind,source="${EXTERNAL_DATADIR}",target="${INTERNAL_DATADIR}" \
	"${ITCOIN_IMAGE}" \
	bitcoind "${@}"
