#!/usr/bin/env bash
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

# Create folders that are mounted into the docker
mkdir -p "${CCACHE_DIR}"
mkdir -p "${PREVIOUS_RELEASES_DIR}"

export ASAN_OPTIONS="detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1"
export LSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/lsan"
export TSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/tsan:halt_on_error=1:log_path=${BASE_SCRATCH_DIR}/sanitizer-output/tsan"
export UBSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/ubsan:print_stacktrace=1:halt_on_error=1:report_error_type=1"

if [ -z "$DANGER_RUN_CI_ON_HOST" ]; then
  # Export all env vars to avoid missing some.
  # Though, exclude those with newlines to avoid parsing problems.
  python3 -c 'import os; [print(f"{key}={value}") for key, value in os.environ.items() if "\n" not in value and "HOME" != key and "PATH" != key and "USER" != key]' | tee /tmp/env
  # System-dependent env vars must be kept as is. So read them from the container.
  docker run --rm "${CI_IMAGE_NAME_TAG}" bash -c "env | grep --extended-regexp '^(HOME|PATH|USER)='" | tee --append /tmp/env
  echo "Creating $CI_IMAGE_NAME_TAG container to run in"
  DOCKER_BUILDKIT=1 docker build \
      --file "${BASE_ROOT_DIR}/ci/test_imagefile" \
      --build-arg "CI_IMAGE_NAME_TAG=${CI_IMAGE_NAME_TAG}" \
      --build-arg "FILE_ENV=${FILE_ENV}" \
      --tag="${CONTAINER_NAME}" \
      "${BASE_ROOT_DIR}"
  docker volume create "${CONTAINER_NAME}_ccache" || true
  docker volume create "${CONTAINER_NAME}_depends" || true
  docker volume create "${CONTAINER_NAME}_previous_releases" || true

  if [ -n "${RESTART_CI_DOCKER_BEFORE_RUN}" ] ; then
    echo "Restart docker before run to stop and clear all containers started with --rm"
    podman container stop --all  # Similar to "systemctl restart docker"
    echo "Prune all dangling images"
    docker image prune --force
  fi

  # shellcheck disable=SC2086
  CI_CONTAINER_ID=$(docker run $CI_CONTAINER_CAP --rm --interactive --detach --tty \
                  --mount type=bind,src=$BASE_ROOT_DIR,dst=/ro_base,readonly \
                  --mount "type=volume,src=${CONTAINER_NAME}_ccache,dst=$CCACHE_DIR" \
                  --mount "type=volume,src=${CONTAINER_NAME}_depends,dst=$DEPENDS_DIR" \
                  --mount "type=volume,src=${CONTAINER_NAME}_previous_releases,dst=$PREVIOUS_RELEASES_DIR" \
                  --env-file /tmp/env \
                  --name $CONTAINER_NAME \
                  $CONTAINER_NAME)
  export CI_CONTAINER_ID
  export CI_EXEC_CMD_PREFIX="docker exec ${CI_CONTAINER_ID}"
else
  echo "Running on host system without docker wrapper"
fi

CI_EXEC () {
  $CI_EXEC_CMD_PREFIX bash -c "export PATH=${BINS_SCRATCH_DIR}:${BASE_ROOT_DIR}/ci/retry:\$PATH && cd \"${BASE_ROOT_DIR}\" && $*"
}
export -f CI_EXEC

CI_EXEC rsync --archive --stats --human-readable /ci_base_install/ "${BASE_ROOT_DIR}" || echo "/ci_base_install/ missing"
CI_EXEC "${BASE_ROOT_DIR}/ci/test/01_base_install.sh"
CI_EXEC rsync --archive --stats --human-readable /ro_base/ "${BASE_ROOT_DIR}" || echo "Nothing to copy from ro_base"
# Fixes permission issues when there is a container UID/GID mismatch with the owner
# of the git source code directory.
CI_EXEC git config --global --add safe.directory \"*\"

CI_EXEC mkdir -p "${BINS_SCRATCH_DIR}"
