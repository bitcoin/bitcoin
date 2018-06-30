#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

DOCKER_IMAGE=${DOCKER_IMAGE:-dashpay/dashd-develop}
DOCKER_TAG=${DOCKER_TAG:-latest}

BUILD_DIR=${BUILD_DIR:-.}

rm docker/bin/*
mkdir docker/bin
cp $BUILD_DIR/src/dashd docker/bin/
cp $BUILD_DIR/src/dash-cli docker/bin/
cp $BUILD_DIR/src/dash-tx docker/bin/
strip docker/bin/dashd
strip docker/bin/dash-cli
strip docker/bin/dash-tx

docker build --pull -t $DOCKER_IMAGE:$DOCKER_TAG -f docker/Dockerfile docker
