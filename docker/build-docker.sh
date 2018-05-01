#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

DOCKER_IMAGE=${DOCKER_IMAGE:-syscoin/syscoind-develop}
DOCKER_TAG=${DOCKER_TAG:-latest}

BUILD_DIR=${BUILD_DIR:-.}

rm docker/bin/*
mkdir docker/bin
cp $BUILD_DIR/src/syscoind docker/bin/
cp $BUILD_DIR/src/syscoin-cli docker/bin/
cp $BUILD_DIR/src/syscoin-tx docker/bin/
strip docker/bin/syscoind
strip docker/bin/syscoin-cli
strip docker/bin/syscoin-tx

docker build --pull -t $DOCKER_IMAGE:$DOCKER_TAG -f docker/Dockerfile docker
