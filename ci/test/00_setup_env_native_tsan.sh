#!/usr/bin/env bash
#
# Copyright (c) 2019-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_tsan
case "$(uname -m)" in
  aarch64)
    export HOST=aarch64-linux-gnu
    ;;
  x86_64)
    export HOST=x86_64-pc-linux-gnu
    ;;
  *)
    if command -v dpkg >/dev/null 2>&1; then
      arch="$(dpkg --print-architecture)"
      if [ "${arch}" = "arm64" ]; then
        export HOST=aarch64-linux-gnu
      elif [ "${arch}" = "amd64" ]; then
        export HOST=x86_64-pc-linux-gnu
      fi
    fi
    ;;
esac
export PACKAGES="clang-19 llvm-19 libclang-rt-19-dev libc++abi-19-dev libc++-19-dev python3-zmq"
export DEP_OPTS="CC=clang-19 CXX='clang++-19 -stdlib=libc++'"
export TEST_RUNNER_EXTRA="--extended --exclude feature_pruning,feature_dbcrash,wallet_multiwallet.py" # Temporarily suppress ASan heap-use-after-free (see issue #14163)
export TEST_RUNNER_EXTRA="${TEST_RUNNER_EXTRA} --timeout-factor=4"  # Increase timeout because sanitizers slow down
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --with-sanitizers=thread CC=clang-19 CXX=clang++-19 CXXFLAGS='-g'"
export CPPFLAGS="-DARENA_DEBUG -DDEBUG_LOCKORDER -DDEBUG_LOCKCONTENTION"
export PYZMQ=true
