#!/usr/bin/env bash
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

if [[ $QEMU_USER_CMD == qemu-s390* ]]; then
  export LC_ALL=C
fi

# Create folders that are mounted into the docker
mkdir -p "${CCACHE_DIR}"
mkdir -p "${PREVIOUS_RELEASES_DIR}"

export ASAN_OPTIONS="detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1"
export LSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/lsan"
export TSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/tsan:halt_on_error=1:log_path=${BASE_SCRATCH_DIR}/sanitizer-output/tsan"
export UBSAN_OPTIONS="suppressions=${BASE_ROOT_DIR}/test/sanitizer_suppressions/ubsan:print_stacktrace=1:halt_on_error=1:report_error_type=1"
env | grep -E '^(BITCOIN_CONFIG|BASE_|QEMU_|CCACHE_|LC_ALL|BOOST_TEST_RANDOM|DEBIAN_FRONTEND|CONFIG_SHELL|(ASAN|LSAN|TSAN|UBSAN)_OPTIONS|PREVIOUS_RELEASES_DIR)' | tee /tmp/env
if [[ $BITCOIN_CONFIG = *--with-sanitizers=*address* ]]; then # If ran with (ASan + LSan), Docker needs access to ptrace (https://github.com/google/sanitizers/issues/764)
  CI_CONTAINER_CAP="--cap-add SYS_PTRACE"
fi

export P_CI_DIR="$PWD"
export BINS_SCRATCH_DIR="${BASE_SCRATCH_DIR}/bins/"

if [ -z "$DANGER_RUN_CI_ON_HOST" ]; then
  echo "Creating $CI_IMAGE_NAME_TAG container to run in"
  DOCKER_BUILDKIT=1 ${CI_RETRY_EXE} docker build \
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
    systemctl restart docker
  fi

  # shellcheck disable=SC2086
  CI_CONTAINER_ID=$(docker run $CI_CONTAINER_CAP --rm --interactive --detach --tty \
                  --mount type=bind,src=$BASE_ROOT_DIR,dst=/ro_base,readonly \
                  --mount "type=volume,src=${CONTAINER_NAME}_ccache,dst=$CCACHE_DIR" \
                  --mount "type=volume,src=${CONTAINER_NAME}_depends,dst=$DEPENDS_DIR" \
                  --mount "type=volume,src=${CONTAINER_NAME}_previous_releases,dst=$PREVIOUS_RELEASES_DIR" \
                  -w $BASE_ROOT_DIR \
                  --env-file /tmp/env \
                  --name $CONTAINER_NAME \
                  $CONTAINER_NAME)
  export CI_CONTAINER_ID
  export CI_EXEC_CMD_PREFIX="docker exec ${CI_CONTAINER_ID}"
else
  echo "Running on host system without docker wrapper"
fi

CI_EXEC () {
  $CI_EXEC_CMD_PREFIX bash -c "export PATH=${BINS_SCRATCH_DIR}:\$PATH && cd \"$P_CI_DIR\" && $*"
}
export -f CI_EXEC

CI_EXEC rsync --archive --stats --human-readable /ci_base_install/ "${BASE_ROOT_DIR}" || echo "/ci_base_install/ missing"
CI_EXEC "${BASE_ROOT_DIR}/ci/test/01_base_install.sh"
CI_EXEC rsync --archive --stats --human-readable /ro_base/ "${BASE_ROOT_DIR}" || echo "Nothing to copy from ro_base"
# Fixes permission issues when there is a container UID/GID mismatch with the owner
# of the git source code directory.
CI_EXEC git config --global --add safe.directory \"*\"

CI_EXEC mkdir -p "${BINS_SCRATCH_DIR}"

if [ "$CI_OS_NAME" == "macos" ]; then
  top -l 1 -s 0 | awk ' /PhysMem/ {print}'
  echo "Number of CPUs: $(sysctl -n hw.logicalcpu)"
else
  CI_EXEC free -m -h
  CI_EXEC echo "Number of CPUs \(nproc\):" \$\(nproc\)
  CI_EXEC echo "$(lscpu | grep Endian)"
fi
CI_EXEC echo "Free disk space:"
CI_EXEC df -h

if [ "$RUN_FUZZ_TESTS" = "true" ]; then
  export DIR_FUZZ_IN=${DIR_QA_ASSETS}/fuzz_seed_corpus/
  if [ ! -d "$DIR_FUZZ_IN" ]; then
    CI_EXEC git clone --depth=1 https://github.com/bitcoin-core/qa-assets "${DIR_QA_ASSETS}"
  fi
elif [ "$RUN_UNIT_TESTS" = "true" ] || [ "$RUN_UNIT_TESTS_SEQUENTIAL" = "true" ]; then
  export DIR_UNIT_TEST_DATA=${DIR_QA_ASSETS}/unit_test_data/
  if [ ! -d "$DIR_UNIT_TEST_DATA" ]; then
    CI_EXEC mkdir -p "$DIR_UNIT_TEST_DATA"
    CI_EXEC curl --location --fail https://github.com/bitcoin-core/qa-assets/raw/main/unit_test_data/script_assets_test.json -o "${DIR_UNIT_TEST_DATA}/script_assets_test.json"
  fi
fi

CI_EXEC mkdir -p "${BASE_SCRATCH_DIR}/sanitizer-output/"

if [ "$USE_BUSY_BOX" = "true" ]; then
  echo "Setup to use BusyBox utils"
  # tar excluded for now because it requires passing in the exact archive type in ./depends (fixed in later BusyBox version)
  # ar excluded for now because it does not recognize the -q option in ./depends (unknown if fixed)
  # shellcheck disable=SC1010
  CI_EXEC for util in \$\(busybox --list \| grep -v "^ar$" \| grep -v "^tar$" \)\; do ln -s \$\(command -v busybox\) "${BINS_SCRATCH_DIR}/\$util"\; done
  # Print BusyBox version
  CI_EXEC patch --help
fi
