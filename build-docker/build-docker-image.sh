#!/usr/bin/env bash
export LC_ALL=C
set -euo pipefail

cd "$(dirname "$0")"

IMAGE_NAME=${IMAGE_NAME:-"bitcoin-builder"}
grep -e "^$(whoami)" /etc/group  > group
grep -e "^$(whoami)" /etc/passwd > passwd
docker build . -t "${IMAGE_NAME}" "$@"

