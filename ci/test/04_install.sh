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
  OPENSSL_PKG_CONFIG_PATH="/usr/local/opt/openssl@1.1/lib/pkgconfig"
  export PKG_CONFIG_PATH=$OPENSSL_PKG_CONFIG_PATH:$PKG_CONFIG_PATH

  ${CI_RETRY_EXE} pip3 install $PIP_PACKAGES

fi

mkdir -p "${BASE_SCRATCH_DIR}"
ccache echo "Creating ccache dir if it didn't already exist"

if [ ! -d ${DIR_QA_ASSETS} ]; then
  git clone https://github.com/bitcoin-core/qa-assets ${DIR_QA_ASSETS}
fi
export DIR_FUZZ_IN=${DIR_QA_ASSETS}/fuzz_seed_corpus/

mkdir -p "${BASE_BUILD_DIR}/sanitizer-output/"
export ASAN_OPTIONS=""
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

  DOCKER_ID=$(docker run $DOCKER_ADMIN -idt --mount type=bind,src=$BASE_BUILD_DIR,dst=$BASE_BUILD_DIR --mount type=bind,src=$CCACHE_DIR,dst=$CCACHE_DIR -w $BASE_BUILD_DIR --env-file /tmp/env $DOCKER_NAME_TAG)

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


if [ "$TRAVIS_OS_NAME" != "osx" ]; then
  ${CI_RETRY_EXE} DOCKER_EXEC apt-get update
  ${CI_RETRY_EXE} DOCKER_EXEC apt-get install --no-install-recommends --no-upgrade -y $PACKAGES $DOCKER_PACKAGES
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
