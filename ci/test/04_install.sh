#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

if [ "$TRAVIS_OS_NAME" == "osx" ]; then
  set +o errexit
  pushd /usr/local/Homebrew || exit 1
  git reset --hard origin/master
  popd || exit 1
  set -o errexit
  ${CI_RETRY_EXE} brew update
  # brew upgrade returns an error if any of the packages is already up to date
  # Failure is safe to ignore, unless we really need an update.
  brew upgrade $BREW_PACKAGES || true

  # install new packages (brew install returns an error if already installed)
  for i in $BREW_PACKAGES; do
    if ! brew list | grep -q $i; then
      ${CI_RETRY_EXE} brew install $i
    fi
  done

  export PATH="/usr/local/opt/ccache/libexec:$PATH"

  ${CI_RETRY_EXE} pip3 install $PIP_PACKAGES

fi

mkdir -p "${BASE_SCRATCH_DIR}"
mkdir -p "${CCACHE_DIR}"

export ASAN_OPTIONS="detect_stack_use_after_return=1"
export LSAN_OPTIONS="suppressions=${BASE_BUILD_DIR}/test/sanitizer_suppressions/lsan"
export TSAN_OPTIONS="suppressions=${BASE_BUILD_DIR}/test/sanitizer_suppressions/tsan:log_path=${BASE_BUILD_DIR}/sanitizer-output/tsan"
export UBSAN_OPTIONS="suppressions=${BASE_BUILD_DIR}/test/sanitizer_suppressions/ubsan:print_stacktrace=1:halt_on_error=1"
env | grep -E '^(BITCOIN_CONFIG|CCACHE_|WINEDEBUG|LC_ALL|BOOST_TEST_RANDOM|CONFIG_SHELL|(ASAN|LSAN|TSAN|UBSAN)_OPTIONS)' | tee /tmp/env
if [[ $HOST = *-mingw32 ]]; then
  DOCKER_ADMIN="--cap-add SYS_ADMIN"
elif [[ $BITCOIN_CONFIG = *--with-sanitizers=*address* ]]; then # If ran with (ASan + LSan), Docker needs access to ptrace (https://github.com/google/sanitizers/issues/764)
  DOCKER_ADMIN="--cap-add SYS_PTRACE"
fi

if [ -z "$RUN_CI_ON_HOST" ]; then
  echo "Creating $DOCKER_NAME_TAG container to run in"
  ${CI_RETRY_EXE} docker pull "$DOCKER_NAME_TAG"

  DOCKER_ID=$(docker run $DOCKER_ADMIN -idt \
                  --mount type=bind,src=$BASE_BUILD_DIR,dst=/ro_base,readonly \
                  --mount type=bind,src=$CCACHE_DIR,dst=$CCACHE_DIR \
                  --mount type=bind,src=$BASE_BUILD_DIR/depends,dst=$BASE_BUILD_DIR/depends \
                  -w $BASE_BUILD_DIR \
                  --env-file /tmp/env \
                  $DOCKER_NAME_TAG)

  DOCKER_EXEC () {
    docker exec $DOCKER_ID bash -c "export PATH=$BASE_SCRATCH_DIR/bins/:\$PATH && cd $PWD && $*"
  }
else
  echo "Running on host system without docker wrapper"
  DOCKER_EXEC () {
    bash -c "export PATH=$BASE_SCRATCH_DIR/bins/:\$PATH && cd $PWD && $*"
  }
fi

if [ "$TRAVIS_OS_NAME" == "osx" ]; then
  top -l 1 -s 0 | awk ' /PhysMem/ {print}'
  echo "Number of CPUs: $(sysctl -n hw.logicalcpu)"
else
  DOCKER_EXEC free -m -h
  DOCKER_EXEC echo "Number of CPUs \(nproc\):" \$\(nproc\)
fi

if [ -n "$DPKG_ADD_ARCH" ]; then
  DOCKER_EXEC dpkg --add-architecture "$DPKG_ADD_ARCH"
fi

if [ "$TRAVIS_OS_NAME" != "osx" ]; then
  ${CI_RETRY_EXE} DOCKER_EXEC apt-get update
  ${CI_RETRY_EXE} DOCKER_EXEC apt-get install --no-install-recommends --no-upgrade -y $PACKAGES $DOCKER_PACKAGES
fi

if [ ! -d ${DIR_QA_ASSETS} ]; then
  DOCKER_EXEC git clone https://github.com/bitcoin-core/qa-assets ${DIR_QA_ASSETS}
fi
export DIR_FUZZ_IN=${DIR_QA_ASSETS}/fuzz_seed_corpus/

DOCKER_EXEC mkdir -p "${BASE_BUILD_DIR}/sanitizer-output/"

if [ -z "$RUN_CI_ON_HOST" ]; then
  echo "Create $BASE_BUILD_DIR"
  DOCKER_EXEC rsync -a /ro_base/ $BASE_BUILD_DIR
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
