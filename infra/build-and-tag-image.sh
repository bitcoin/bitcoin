#!/usr/bin/env bash
#
# ItCoin
#
# Builds a docker image for itcoin-core from the current working copy.
# The image is named "arthub.azurecr.io/itcoin-core", and tagged with "git-"
# plus a 12 digits long git hash of the current version.
#
# EXAMPLE:
#    arthub.azurecr.io/itcoin-core:git-2a43646f76e4
#
# REQUIREMENTS:
# - docker, version >= 17.06
#
# USAGE:
#     build-and-tag-image.sh
#
# Author: muxator <antonio.muci@bancaditalia.it>

set -eu

errecho() {
    # prints to stderr
    >&2 echo $@;
}

checkPrerequisites() {
    if ! command -v docker &> /dev/null; then
        errecho "Please install docker (https://www.docker.com/)"
        exit 1
    fi
}

# Do not run if the required packages are not installed
checkPrerequisites

# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself#246128
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

IMAGE_NAME="arthub.azurecr.io/itcoin-core"
IMAGE_TAG="git-"$("${MYDIR}"/compute-git-hash.sh)

docker build --tag "${IMAGE_NAME}:${IMAGE_TAG}" "${MYDIR}/.."

# MUXATOR 2020-12-22: as of today, there is no known way to compute the sha256
# digest of an image that was never pushed to a registry. This is odd, since the
# hashes should be dependant on the image contents only.
#
# Since printing the digest is just an experimental functionality, let's ignore
# errors here.
echo "EXPERIMENTAL: the sha256 hash of the container we just built is:"
docker inspect --format='{{index .RepoDigests 0}}' "${IMAGE_NAME}:${IMAGE_TAG}" || true
