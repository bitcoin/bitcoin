#!/usr/bin/env bash

# This script is meant to be sourced into the actual build script. It contains the build matrix and will set all
# necessary environment variables for the request build target

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
export DOCKER_RUN_ENV_ARGS="-e SRC_DIR=$SRC_DIR -e CACHE_DIR=$CACHE_DIR -e PULL_REQUEST=$PULL_REQUEST -e JOB_NUMBER=$JOB_NUMBER -e BUILD_TARGET=$BUILD_TARGET"
export DOCKER_RUN_ARGS="$DOCKER_RUN_VOLUME_ARGS $DOCKER_RUN_ENV_ARGS"
export DOCKER_RUN_IN_BUILDER="docker run -t --rm -w $SRC_DIR $DOCKER_RUN_ARGS $BUILDER_IMAGE_NAME"

# Default values for targets
export GOAL="install"
export SDK_URL=${SDK_URL:-https://bitcoincore.org/depends-sources/sdks}
export PYTHON_DEBUG=1
export MAKEJOBS="-j4"

if [ "$BUILD_TARGET" = "arm-linux" ]; then
  export HOST=arm-linux-gnueabihf
  export PACKAGES="g++-arm-linux-gnueabihf"
  export DEP_OPTS="NO_QT=1"
  export CHECK_DOC=1
  export BITCOIN_CONFIG="--enable-glibc-back-compat --enable-reduce-exports"
elif [ "$BUILD_TARGET" = "win32" ]; then
  export HOST=i686-w64-mingw32
  export DPKG_ADD_ARCH="i386"
  export DEP_OPTS="NO_QT=1"
  export PACKAGES="python3 nsis g++-mingw-w64-i686 wine-stable wine32 bc"
  export BITCOIN_CONFIG="--enable-gui --enable-reduce-exports"
  export DIRECT_WINE_EXEC_TESTS=true
  export RUN_TESTS=true
elif [ "$BUILD_TARGET" = "win64" ]; then
  export HOST=x86_64-w64-mingw32
  export DPKG_ADD_ARCH="i386"
  export DEP_OPTS="NO_QT=1"
  export PACKAGES="python3 nsis g++-mingw-w64-x86-64 wine-stable wine64 bc"
  export BITCOIN_CONFIG="--enable-gui --enable-reduce-exports"
  export DIRECT_WINE_EXEC_TESTS=true
  export RUN_TESTS=true
elif [ "$BUILD_TARGET" = "linux32" ]; then
  export HOST=i686-pc-linux-gnu
  export PACKAGES="g++-multilib bc python3-zmq"
  export DEP_OPTS="NO_QT=1"
  export BITCOIN_CONFIG="--enable-zmq --enable-glibc-back-compat --enable-reduce-exports LDFLAGS=-static-libstdc++"
  export USE_SHELL="/bin/dash"
  export PYZMQ=true
  export RUN_TESTS=true
elif [ "$BUILD_TARGET" = "linux64" ]; then
  export HOST=x86_64-unknown-linux-gnu
  export PACKAGES="bc python3-zmq"
  export DEP_OPTS="NO_QT=1 NO_UPNP=1 DEBUG=1"
  export BITCOIN_CONFIG="--enable-zmq --enable-glibc-back-compat --enable-reduce-exports"
  export CPPFLAGS="-DDEBUG_LOCKORDER -DENABLE_DASH_DEBUG"
  export PYZMQ=true
  export RUN_TESTS=true
elif [ "$BUILD_TARGET" = "linux64_nowallet" ]; then
  export HOST=x86_64-unknown-linux-gnu
  export PACKAGES="python3"
  export DEP_OPTS="NO_WALLET=1"
  export BITCOIN_CONFIG="--enable-glibc-back-compat --enable-reduce-exports"
elif [ "$BUILD_TARGET" = "linux64_release" ]; then
  export HOST=x86_64-unknown-linux-gnu
  export PACKAGES="bc python3-zmq"
  export DEP_OPTS="NO_QT=1 NO_UPNP=1"
  export BITCOIN_CONFIG="--enable-zmq --enable-glibc-back-compat --enable-reduce-exports"
  export PYZMQ=true
elif [ "$BUILD_TARGET" = "mac" ]; then
  export HOST=x86_64-apple-darwin11
  export PACKAGES="cmake imagemagick libcap-dev librsvg2-bin libz-dev libbz2-dev libtiff-tools"
  export BITCOIN_CONFIG="--enable-gui --enable-reduce-exports"
  export OSX_SDK=10.11
  export GOAL="deploy"
fi
