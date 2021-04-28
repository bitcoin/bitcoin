#!/usr/bin/env bash
#
# This script is meant to be sourced into the actual build script. It contains the build matrix and will set all
# necessary environment variables for the request build target

export LC_ALL=C

export BUILD_TARGET=${BUILD_TARGET:-linux64}
export PULL_REQUEST=${PULL_REQUEST:-false}
export JOB_NUMBER=${JOB_NUMBER:-1}

export BUILDER_IMAGE_NAME="dash-builder-$BUILD_TARGET-$JOB_NUMBER"

export HOST_SRC_DIR=${HOST_SRC_DIR:-$(pwd)}
export HOST_CACHE_DIR=${HOST_CACHE_DIR:-$(pwd)/ci-cache-$BUILD_TARGET}

export SRC_DIR=${SRC_DIR:-$HOST_SRC_DIR}
export BUILD_DIR=$SRC_DIR
export OUT_DIR=$BUILD_DIR/out

export CACHE_DIR=${CACHE_DIR:-$HOST_CACHE_DIR}
export CCACHE_DIR=$CACHE_DIR/ccache

export DOCKER_RUN_VOLUME_ARGS="-v $HOST_SRC_DIR:$SRC_DIR -v $HOST_CACHE_DIR:$CACHE_DIR"
export DOCKER_RUN_ENV_ARGS="-e SRC_DIR=$SRC_DIR -e CACHE_DIR=$CACHE_DIR -e PULL_REQUEST=$PULL_REQUEST -e COMMIT_RANGE=$COMMIT_RANGE -e JOB_NUMBER=$JOB_NUMBER -e BUILD_TARGET=$BUILD_TARGET"
export DOCKER_RUN_ARGS="$DOCKER_RUN_VOLUME_ARGS $DOCKER_RUN_ENV_ARGS"
export DOCKER_RUN_IN_BUILDER="docker run -t --rm -w $SRC_DIR $DOCKER_RUN_ARGS $BUILDER_IMAGE_NAME"

# Default values for targets
export GOAL="install"
export SDK_URL=${SDK_URL:-https://bitcoincore.org/depends-sources/sdks}
export MAKEJOBS="-j4"

export RUN_UNITTESTS=false
export RUN_INTEGRATIONTESTS=false

if [ "$BUILD_TARGET" = "arm-linux" ]; then
  export HOST=arm-linux-gnueabihf
  export CHECK_DOC=1
  # -Wno-psabi is to disable ABI warnings: "note: parameter passing for argument of type ... changed in GCC 7.1"
  # This could be removed once the ABI change warning does not show up by default
  export BITCOIN_CONFIG="--enable-glibc-back-compat --enable-reduce-exports CXXFLAGS=-Wno-psabi"
elif [ "$BUILD_TARGET" = "win32" ]; then
  export HOST=i686-w64-mingw32
  export DPKG_ADD_ARCH="i386"
  export BITCOIN_CONFIG="--enable-gui --enable-reduce-exports --disable-miner"
  export DIRECT_WINE_EXEC_TESTS=true
  export RUN_UNITTESTS=true
elif [ "$BUILD_TARGET" = "win64" ]; then
  export HOST=x86_64-w64-mingw32
  export DPKG_ADD_ARCH="i386"
  export BITCOIN_CONFIG="--enable-gui --enable-reduce-exports --disable-miner"
  export DIRECT_WINE_EXEC_TESTS=true
  export RUN_UNITTESTS=true
elif [ "$BUILD_TARGET" = "linux32" ]; then
  export HOST=i686-pc-linux-gnu
  export BITCOIN_CONFIG="--enable-zmq --enable-glibc-back-compat --enable-reduce-exports --enable-crash-hooks LDFLAGS=-static-libstdc++"
  export USE_SHELL="/bin/dash"
  export PYZMQ=true
  export RUN_UNITTESTS=true
  export RUN_INTEGRATIONTESTS=true
elif [ "$BUILD_TARGET" = "linux64" ]; then
  export HOST=x86_64-unknown-linux-gnu
  export DEP_OPTS="NO_UPNP=1 DEBUG=1"
  export BITCOIN_CONFIG="--enable-zmq --enable-glibc-back-compat --enable-reduce-exports --enable-crash-hooks"
  export CPPFLAGS="-DDEBUG_LOCKORDER -DENABLE_DASH_DEBUG -DARENA_DEBUG"
  export PYZMQ=true
  export RUN_UNITTESTS=true
  export RUN_INTEGRATIONTESTS=true
elif [ "$BUILD_TARGET" = "linux64_cxx17" ]; then
  export HOST=x86_64-unknown-linux-gnu
  export DEP_OPTS="NO_UPNP=1 DEBUG=1"
  export BITCOIN_CONFIG="--enable-zmq --enable-glibc-back-compat --enable-reduce-exports --enable-crash-hooks --enable-c++17"
  export CPPFLAGS="-DDEBUG_LOCKORDER -DENABLE_DASH_DEBUG -DARENA_DEBUG"
  export PYZMQ=true
  export RUN_UNITTESTS=true
  export RUN_INTEGRATIONTESTS=true
elif [ "$BUILD_TARGET" = "linux64_nowallet" ]; then
  export HOST=x86_64-unknown-linux-gnu
  export DEP_OPTS="NO_WALLET=1"
  export BITCOIN_CONFIG="--enable-glibc-back-compat --enable-reduce-exports"
  export RUN_UNITTESTS=true
elif [ "$BUILD_TARGET" = "linux64_release" ]; then
  export HOST=x86_64-unknown-linux-gnu
  export DEP_OPTS="NO_UPNP=1"
  export BITCOIN_CONFIG="--enable-zmq --enable-glibc-back-compat --enable-reduce-exports"
  export PYZMQ=true
  export RUN_UNITTESTS=true
elif [ "$BUILD_TARGET" = "mac" ]; then
  export HOST=x86_64-apple-darwin14
  export BITCOIN_CONFIG="--enable-gui --enable-reduce-exports --disable-miner"
  export OSX_SDK=10.11
  export GOAL="all deploy"
fi
