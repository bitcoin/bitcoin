#!/usr/bin/env bash
# Copyright (c) 2021-2023 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"/../..

DOCKER_IMAGE=${DOCKER_IMAGE:-dashpay/dashd-develop}
DOCKER_TAG=${DOCKER_TAG:-latest}
DOCKER_RELATIVE_PATH=contrib/containers/deploy

rm -rf "${DOCKER_RELATIVE_PATH:?}/bin"
mkdir -p "$DOCKER_RELATIVE_PATH/bin"
cp src/dashd    "$DOCKER_RELATIVE_PATH/bin/"
cp src/dash-cli "$DOCKER_RELATIVE_PATH/bin/"
cp src/dash-tx  "$DOCKER_RELATIVE_PATH/bin/"
strip "$DOCKER_RELATIVE_PATH/bin/dashd"
strip "$DOCKER_RELATIVE_PATH/bin/dash-cli"
strip "$DOCKER_RELATIVE_PATH/bin/dash-tx"

docker build --pull -t "$DOCKER_IMAGE":"$DOCKER_TAG" -f "$DOCKER_RELATIVE_PATH/Dockerfile" "$DOCKER_RELATIVE_PATH"
