#!/usr/bin/env bash
#
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8
export CI_IMAGE_LABEL="bitcoin-ci-test"

set -o errexit -o pipefail -o xtrace

if [ -z "$DANGER_RUN_CI_ON_HOST" ]; then
  # Export all env vars to avoid missing some.
  # Though, exclude those with newlines to avoid parsing problems.
  python3 -c 'import os; [print(f"{key}={value}") for key, value in os.environ.items() if "\n" not in value and "HOME" != key and "PATH" != key and "USER" != key]' | tee "/tmp/env-$USER-$CONTAINER_NAME"

  # Env vars during the build can not be changed. For example, a modified
  # $MAKEJOBS is ignored in the build process. Use --cpuset-cpus as an
  # approximation to respect $MAKEJOBS somewhat, if cpuset is available.
  MAYBE_CPUSET=""
  if [ "$HAVE_CGROUP_CPUSET" ]; then
    MAYBE_CPUSET="--cpuset-cpus=$( python3 -c "import random;P=$( nproc );M=min(P,int('$MAKEJOBS'.lstrip('-j')));print(','.join(map(str,sorted(random.sample(range(P),M)))))" )"
  fi
  echo "Creating $CI_IMAGE_NAME_TAG container to run in"

  DOCKER_BUILD_CACHE_ARG=""
  DOCKER_BUILD_CACHE_TEMPDIR=""
  DOCKER_BUILD_CACHE_OLD_DIR=""
  DOCKER_BUILD_CACHE_NEW_DIR=""
  # If set, use an `docker build` cache directory on the CI host
  # to cache docker image layers for the CI container image.
  # This cache can be multiple GB in size. Prefixed with DANGER
  # as setting it removes (old cache) files from the host.
  if [ "$DANGER_DOCKER_BUILD_CACHE_HOST_DIR" ]; then
    # Directory where the current cache for this run could be. If not existing
    # or empty, "docker build" will warn, but treat it as cache-miss and continue.
    DOCKER_BUILD_CACHE_OLD_DIR="${DANGER_DOCKER_BUILD_CACHE_HOST_DIR}/${CONTAINER_NAME}"
    # Temporary directory for a newly created cache. We can't write the new
    # cache into OLD_DIR directly, as old cache layers would not be removed.
    # The NEW_DIR contents are moved to OLD_DIR after OLD_DIR has been cleared.
    # This happens after `docker build`. If a task fails or is aborted, the
    # DOCKER_BUILD_CACHE_TEMPDIR might be retained on the host. If the host isn't
    # ephemeral, it has to take care of cleaning old TEMPDIR's up.
    DOCKER_BUILD_CACHE_TEMPDIR="$(mktemp --directory ci-docker-build-cache-XXXXXXXXXX)"
    DOCKER_BUILD_CACHE_NEW_DIR="${DOCKER_BUILD_CACHE_TEMPDIR}/${CONTAINER_NAME}"
    DOCKER_BUILD_CACHE_ARG="--cache-from type=local,src=${DOCKER_BUILD_CACHE_OLD_DIR} --cache-to type=local,dest=${DOCKER_BUILD_CACHE_NEW_DIR},mode=max"
  fi

  # shellcheck disable=SC2086
  DOCKER_BUILDKIT=1 docker build \
      --file "${BASE_READ_ONLY_DIR}/ci/test_imagefile" \
      --build-arg "CI_IMAGE_NAME_TAG=${CI_IMAGE_NAME_TAG}" \
      --build-arg "FILE_ENV=${FILE_ENV}" \
      $MAYBE_CPUSET \
      --platform="${CI_IMAGE_PLATFORM}" \
      --label="${CI_IMAGE_LABEL}" \
      --tag="${CONTAINER_NAME}" \
      $DOCKER_BUILD_CACHE_ARG \
      "${BASE_READ_ONLY_DIR}"

  if [ "$DANGER_DOCKER_BUILD_CACHE_HOST_DIR" ]; then
    if [ -e "${DOCKER_BUILD_CACHE_NEW_DIR}/index.json" ]; then
      echo "Removing the existing docker build cache in ${DOCKER_BUILD_CACHE_OLD_DIR}"
      rm -rf "${DOCKER_BUILD_CACHE_OLD_DIR}"
      echo "Moving the contents of ${DOCKER_BUILD_CACHE_NEW_DIR} to ${DOCKER_BUILD_CACHE_OLD_DIR}"
      mv "${DOCKER_BUILD_CACHE_NEW_DIR}" "${DOCKER_BUILD_CACHE_OLD_DIR}"
    fi
  fi

  docker volume create "${CONTAINER_NAME}_ccache" || true
  docker volume create "${CONTAINER_NAME}_depends" || true
  docker volume create "${CONTAINER_NAME}_depends_sources" || true
  docker volume create "${CONTAINER_NAME}_previous_releases" || true

  CI_CCACHE_MOUNT="type=volume,src=${CONTAINER_NAME}_ccache,dst=$CCACHE_DIR"
  CI_DEPENDS_MOUNT="type=volume,src=${CONTAINER_NAME}_depends,dst=$DEPENDS_DIR/built"
  CI_DEPENDS_SOURCES_MOUNT="type=volume,src=${CONTAINER_NAME}_depends_sources,dst=$DEPENDS_DIR/sources"
  CI_PREVIOUS_RELEASES_MOUNT="type=volume,src=${CONTAINER_NAME}_previous_releases,dst=$PREVIOUS_RELEASES_DIR"
  CI_BUILD_MOUNT=""

  if [ "$DANGER_CI_ON_HOST_FOLDERS" ]; then
    # ensure the directories exist
    mkdir -p "${CCACHE_DIR}"
    mkdir -p "${DEPENDS_DIR}/built"
    mkdir -p "${DEPENDS_DIR}/sources"
    mkdir -p "${PREVIOUS_RELEASES_DIR}"
    mkdir -p "${BASE_BUILD_DIR}"  # Unset by default, must be defined externally

    CI_CCACHE_MOUNT="type=bind,src=${CCACHE_DIR},dst=$CCACHE_DIR"
    CI_DEPENDS_MOUNT="type=bind,src=${DEPENDS_DIR}/built,dst=$DEPENDS_DIR/built"
    CI_DEPENDS_SOURCES_MOUNT="type=bind,src=${DEPENDS_DIR}/sources,dst=$DEPENDS_DIR/sources"
    CI_PREVIOUS_RELEASES_MOUNT="type=bind,src=${PREVIOUS_RELEASES_DIR},dst=$PREVIOUS_RELEASES_DIR"
    CI_BUILD_MOUNT="--mount type=bind,src=${BASE_BUILD_DIR},dst=${BASE_BUILD_DIR}"
  fi

  if [ "$DANGER_CI_ON_HOST_CCACHE_FOLDER" ]; then
    if [ ! -d "${CCACHE_DIR}" ]; then
      echo "Error: Directory '${CCACHE_DIR}' must be created in advance."
      exit 1
    fi
    CI_CCACHE_MOUNT="type=bind,src=${CCACHE_DIR},dst=${CCACHE_DIR}"
  fi

  docker network create --ipv6 --subnet 1111:1111::/112 ci-ip6net || true

  if [ -n "${RESTART_CI_DOCKER_BEFORE_RUN}" ] ; then
    echo "Restart docker before run to stop and clear all containers started with --rm"
    podman container rm --force --all  # Similar to "systemctl restart docker"

    # Still prune everything in case the filtered pruning doesn't work, or if labels were not set
    # on a previous run. Belt and suspenders approach, should be fine to remove in the future.
    # Prune images used by --external containers (e.g. build containers) when
    # using podman.
    echo "Prune all dangling images"
    podman image prune --force --external
  fi
  echo "Prune all dangling $CI_IMAGE_LABEL images"
  # When detecting podman-docker, `--external` should be added.
  docker image prune --force --filter "label=$CI_IMAGE_LABEL"

  # Append $USER to /tmp/env to support multi-user systems and $CONTAINER_NAME
  # to allow support starting multiple runs simultaneously by the same user.
  # shellcheck disable=SC2086
  CI_CONTAINER_ID=$(docker run --cap-add LINUX_IMMUTABLE $CI_CONTAINER_CAP --rm --interactive --detach --tty \
                  --mount "type=bind,src=$BASE_READ_ONLY_DIR,dst=$BASE_READ_ONLY_DIR,readonly" \
                  --mount "${CI_CCACHE_MOUNT}" \
                  --mount "${CI_DEPENDS_MOUNT}" \
                  --mount "${CI_DEPENDS_SOURCES_MOUNT}" \
                  --mount "${CI_PREVIOUS_RELEASES_MOUNT}" \
                  ${CI_BUILD_MOUNT} \
                  --env-file /tmp/env-$USER-$CONTAINER_NAME \
                  --name "$CONTAINER_NAME" \
                  --network ci-ip6net \
                  --platform="${CI_IMAGE_PLATFORM}" \
                  "$CONTAINER_NAME")
  export CI_CONTAINER_ID
  export CI_EXEC_CMD_PREFIX="docker exec ${CI_CONTAINER_ID}"
else
  echo "Running on host system without docker wrapper"
  echo "Create missing folders"
  mkdir -p "${CCACHE_DIR}"
  mkdir -p "${PREVIOUS_RELEASES_DIR}"
fi

if [ "$CI_OS_NAME" == "macos" ]; then
  IN_GETOPT_BIN="$(brew --prefix gnu-getopt)/bin/getopt"
  export IN_GETOPT_BIN
fi

CI_EXEC () {
  $CI_EXEC_CMD_PREFIX bash -c "export PATH=\"/path_with space:${BINS_SCRATCH_DIR}:${BASE_ROOT_DIR}/ci/retry:\$PATH\" && cd \"${BASE_ROOT_DIR}\" && $*"
}
export -f CI_EXEC

# Normalize all folders to BASE_ROOT_DIR
CI_EXEC rsync --recursive --perms --stats --human-readable "${BASE_READ_ONLY_DIR}/" "${BASE_ROOT_DIR}" || echo "Nothing to copy from ${BASE_READ_ONLY_DIR}/"
CI_EXEC "${BASE_ROOT_DIR}/ci/test/01_base_install.sh"

CI_EXEC mkdir -p "${BINS_SCRATCH_DIR}"

CI_EXEC "${BASE_ROOT_DIR}/ci/test/03_test_script.sh"

if [ -z "$DANGER_RUN_CI_ON_HOST" ]; then
  echo "Stop and remove CI container by ID"
  docker container kill "${CI_CONTAINER_ID}"
fi
