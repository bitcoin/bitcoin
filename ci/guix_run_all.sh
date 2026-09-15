#!/usr/bin/env bash
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.
export LC_ALL=C.UTF-8
set -o errexit -o pipefail

source contrib/guix/libexec/prelude.bash

if [ ! -d .git ]; then
    echo "This CI runner requires a standalone checkout, not a linked worktree." >&2
    exit 1
fi

read -ra cache_args <<< "$DOCKER_BUILD_CACHE_ARG"
docker buildx build "${cache_args[@]}" --allow security.insecure \
    --file ci/guix_imagefile --tag "$CONTAINER_NAME" \
    --build-arg guix_version="$GUIX_BINARY_VERSION" \
    --build-arg guix_time_machine_url="$GUIX_TIME_MACHINE_URL" \
    --build-arg guix_time_machine_commit="$GUIX_TIME_MACHINE_COMMIT" .

mkdir -p "$SOURCES_PATH" "$BASE_CACHE" "$SDK_PATH"
container_id="$(docker create --init --privileged --interactive \
    --mount "type=bind,src=$PWD,dst=/source,readonly" \
    --mount "type=bind,src=$SOURCES_PATH,dst=/sources-cache,readonly" \
    --mount "type=bind,src=$BASE_CACHE,dst=/built-cache,readonly" \
    --mount "type=bind,src=$SDK_PATH,dst=/sdk-cache,readonly" \
    --env HOSTS --env SUBSTITUTE_URLS \
    --env SOURCES_PATH=/build/.guix-cache/depends-sources \
    --env BASE_CACHE=/build/.guix-cache/depends-built \
    --env SDK_PATH=/build/.guix-cache/sdk \
    "$CONTAINER_NAME" bash -e)"
trap 'docker rm --force "$container_id" > /dev/null' EXIT

docker start --attach --interactive "$container_id" <<'EOF'
mkdir -p /build
rsync --recursive --links --perms --exclude=/.guix-cache /source/ /build/
mkdir -p "$SOURCES_PATH" "$BASE_CACHE" "$SDK_PATH"
cp -R /sources-cache/. "$SOURCES_PATH/"
cp -R /built-cache/. "$BASE_CACHE/"
cp -R /sdk-cache/. "$SDK_PATH/"
cd /build
git update-index --refresh
source contrib/guix/libexec/prelude.bash

case "$HOSTS" in
    arm64-apple-darwin|x86_64-w64-mingw32)
        export HOST="$HOSTS"
        JOBS="$(nproc)"
        time-machine shell --manifest=contrib/guix/manifest_codesign.scm \
                           --container --pure --no-cwd \
                           --cores="$JOBS" --keep-failed --fallback \
                           ${SUBSTITUTE_URLS:+--substitute-urls="$SUBSTITUTE_URLS"} \
                           -- true
        ;;
esac

./contrib/guix/guix-build
EOF

mkdir -p "$OUTDIR_BASE"
docker cp "$container_id:/build/guix-build-$VERSION/output/." "$OUTDIR_BASE/"
docker cp "$container_id:/build/.guix-cache/depends-sources/." "$SOURCES_PATH/"
docker cp "$container_id:/build/.guix-cache/depends-built/." "$BASE_CACHE/"
