#!/usr/bin/env bash
#
# ItCoin
#
# Builds a docker image for itcoin-core from the current working copy.
# The image is tagged (for now) with the fixed hard coded tag
# itcoin:git-bc177a0bd9e, which will be used by subsequent steps.
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

IMAGE_NAME="itcoin"
IMAGE_TAG="git-bc177a0bd9e4"

docker build --tag "${IMAGE_NAME}:${IMAGE_TAG}" "${MYDIR}/.."
