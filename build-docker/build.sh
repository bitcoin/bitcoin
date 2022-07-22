#!/usr/bin/env bash
export LC_ALL=C
set -euo pipefail

cd "$(dirname "$0")"

./build-docker-image.sh

# Install Just
if ! command -v just ; then
   curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash -s -- --to ./
  chmod +x ./just
fi
cd ..

PATH="$PATH:./build-docker"
just -f build-docker/Justfile build-image
just -f build-docker/Justfile setup "${SETUP_OPTS:-""}"
just -f build-docker/Justfile configure "${CONFIGURE_OPTS:-""}"
just -f build-docker/Justfile rebuild
