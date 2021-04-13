#!/usr/bin/env bash
#
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

if [[ $QEMU_USER_CMD == qemu-s390* ]]; then
  export LC_ALL=C
fi

if [ "$CI_OS_NAME" == "macos" ]; then
  IN_GETOPT_BIN="/usr/local/opt/gnu-getopt/bin/getopt" ${CI_RETRY_EXE} pip3 install --user $PIP_PACKAGES
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
  DOCKER_ADMIN="--cap-add SYS_PTRACE"
fi

export P_CI_DIR="$PWD"

if [ -z "$DANGER_RUN_CI_ON_HOST" ]; then
  echo "Creating $DOCKER_NAME_TAG container to run in"
  ${CI_RETRY_EXE} docker pull "$DOCKER_NAME_TAG"

  if [ -n "${RESTART_CI_DOCKER_BEFORE_RUN}" ] ; then
    echo "Restart docker before run to stop and clear all containers started with --rm"
    systemctl restart docker
  fi

  DOCKER_ID=$(docker run $DOCKER_ADMIN --rm --interactive --detach --tty \
                  --mount type=bind,src=$BASE_ROOT_DIR,dst=/ro_base,readonly \
                  --mount type=bind,src=$CCACHE_DIR,dst=$CCACHE_DIR \
                  --mount type=bind,src=$DEPENDS_DIR,dst=$DEPENDS_DIR \
                  --mount type=bind,src=$PREVIOUS_RELEASES_DIR,dst=$PREVIOUS_RELEASES_DIR \
                  -w $BASE_ROOT_DIR \
                  --env-file /tmp/env \
                  --name $CONTAINER_NAME \
                  $DOCKER_NAME_TAG)
  export DOCKER_CI_CMD_PREFIX="docker exec $DOCKER_ID"
else
  echo "Running on host system without docker wrapper"
fi

DOCKER_EXEC () {
  $DOCKER_CI_CMD_PREFIX bash -c "export PATH=$BASE_SCRATCH_DIR/bins/:\$PATH && cd $P_CI_DIR && $*"
}
export -f DOCKER_EXEC

if [ -n "$DPKG_ADD_ARCH" ]; then
  DOCKER_EXEC dpkg --add-architecture "$DPKG_ADD_ARCH"
fi

if [[ $DOCKER_NAME_TAG == centos* ]]; then
  ${CI_RETRY_EXE} DOCKER_EXEC dnf -y install epel-release
  ${CI_RETRY_EXE} DOCKER_EXEC dnf -y --allowerasing install $DOCKER_PACKAGES $PACKAGES
elif [ "$CI_USE_APT_INSTALL" != "no" ]; then
  ${CI_RETRY_EXE} DOCKER_EXEC apt-get update
  ${CI_RETRY_EXE} DOCKER_EXEC apt-get install --no-install-recommends --no-upgrade -y $PACKAGES $DOCKER_PACKAGES
  if [ -n "$PIP_PACKAGES" ]; then
    ${CI_RETRY_EXE} pip3 install --user $PIP_PACKAGES
  fi
fi

if [ "$CI_OS_NAME" == "macos" ]; then
  top -l 1 -s 0 | awk ' /PhysMem/ {print}'
  echo "Number of CPUs: $(sysctl -n hw.logicalcpu)"
else
  DOCKER_EXEC free -m -h
  DOCKER_EXEC echo "Number of CPUs \(nproc\):" \$\(nproc\)
  DOCKER_EXEC echo $(lscpu | grep Endian)
fi
DOCKER_EXEC echo "Free disk space:"
DOCKER_EXEC df -h

if [ ! -d ${DIR_QA_ASSETS} ]; then
  DOCKER_EXEC git clone --depth=1 https://github.com/bitcoin-core/qa-assets ${DIR_QA_ASSETS}
fi
export DIR_FUZZ_IN=${DIR_QA_ASSETS}/fuzz_seed_corpus/
export DIR_UNIT_TEST_DATA=${DIR_QA_ASSETS}/unit_test_data/

DOCKER_EXEC mkdir -p "${BASE_SCRATCH_DIR}/sanitizer-output/"

if [[ ${USE_MEMORY_SANITIZER} == "true" ]]; then
  DOCKER_EXEC "update-alternatives --install /usr/bin/clang++ clang++ \$(which clang++-9) 100"
  DOCKER_EXEC "update-alternatives --install /usr/bin/clang clang \$(which clang-9) 100"
  DOCKER_EXEC "mkdir -p ${BASE_SCRATCH_DIR}/msan/build/"
  DOCKER_EXEC "git clone --depth=1 https://github.com/llvm/llvm-project -b llvmorg-10.0.0 ${BASE_SCRATCH_DIR}/msan/llvm-project"
  DOCKER_EXEC "cd ${BASE_SCRATCH_DIR}/msan/build/ && cmake -DLLVM_ENABLE_PROJECTS='libcxx;libcxxabi' -DCMAKE_BUILD_TYPE=Release -DLLVM_USE_SANITIZER=Memory -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DLLVM_TARGETS_TO_BUILD=X86 ../llvm-project/llvm/"
  DOCKER_EXEC "cd ${BASE_SCRATCH_DIR}/msan/build/ && make $MAKEJOBS cxx"
fi

if [ -z "$DANGER_RUN_CI_ON_HOST" ]; then
  echo "Create $BASE_ROOT_DIR"
  DOCKER_EXEC rsync -a /ro_base/ $BASE_ROOT_DIR
fi

if [ "$USE_BUSY_BOX" = "true" ]; then
  echo "Setup to use BusyBox utils"
  DOCKER_EXEC mkdir -p $BASE_SCRATCH_DIR/bins/
  # tar excluded for now because it requires passing in the exact archive type in ./depends (fixed in later BusyBox version)
  # find excluded for now because it does not recognize the -delete option in ./depends (fixed in later BusyBox version)
  # ar excluded for now because it does not recognize the -q option in ./depends (unknown if fixed)
  # shellcheck disable=SC1010
  DOCKER_EXEC for util in \$\(busybox --list \| grep -v "^ar$" \| grep -v "^tar$" \| grep -v "^find$"\)\; do ln -s \$\(command -v busybox\) $BASE_SCRATCH_DIR/bins/\$util\; done
  # Print BusyBox version
  DOCKER_EXEC patch --help
fi
