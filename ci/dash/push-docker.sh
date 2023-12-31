#!/usr/bin/env bash
# Copyright (c) 2021-2023 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"/../.. || exit

DOCKER_IMAGE=${DOCKER_IMAGE:-dashpay/dashd-develop}
DOCKER_TAG=${DOCKER_TAG:-latest}

if [ -n "$DOCKER_REPO" ]; then
  DOCKER_IMAGE_WITH_REPO=$DOCKER_REPO/$DOCKER_IMAGE
else
  DOCKER_IMAGE_WITH_REPO=$DOCKER_IMAGE
fi

docker tag "$DOCKER_IMAGE":"$DOCKER_TAG" "$DOCKER_IMAGE_WITH_REPO":"$DOCKER_TAG"
docker push "$DOCKER_IMAGE_WITH_REPO":"$DOCKER_TAG"
docker rmi "$DOCKER_IMAGE_WITH_REPO":"$DOCKER_TAG"
