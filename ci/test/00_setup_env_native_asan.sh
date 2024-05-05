#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CI_IMAGE_NAME_TAG="docker.io/ubuntu:24.04"

# Only install BCC tracing packages in CI. Container has to match the host for BCC to work.
if [[ "${INSTALL_BCC_TRACING_TOOLS}" == "true" ]]; then
  # Required for USDT functional tests to run
  BPFCC_PACKAGE="bpfcc-tools linux-headers-$(uname --kernel-release)"
  export CI_CONTAINER_CAP="--privileged -v /sys/kernel:/sys/kernel:rw"
else
  BPFCC_PACKAGE=""
  export CI_CONTAINER_CAP="--cap-add SYS_PTRACE"  # If run with (ASan + LSan), the container needs access to ptrace (https://github.com/google/sanitizers/issues/764)
fi

export CONTAINER_NAME=ci_native_asan
export PACKAGES="systemtap-sdt-dev clang-18 llvm-18 libclang-rt-18-dev python3-zmq qtbase5-dev qttools5-dev qttools5-dev-tools libevent-dev libboost-dev libdb5.3++-dev libminiupnpc-dev libzmq3-dev libqrencode-dev libsqlite3-dev ${BPFCC_PACKAGE}"
export NO_DEPENDS=1
export GOAL="install"
export BITCOIN_CONFIG="\
 -DWITH_USDT=ON -DWITH_ZMQ=ON -DWITH_BDB=ON -DWARN_INCOMPATIBLE_BDB=OFF -DBUILD_GUI=ON \
 -DSANITIZERS=address,float-divide-by-zero,integer,undefined \
 -DCMAKE_C_COMPILER=clang-18 \
 -DCMAKE_CXX_COMPILER=clang++-18 \
 -DCMAKE_C_FLAGS='-ftrivial-auto-var-init=pattern' \
 -DCMAKE_CXX_FLAGS='-ftrivial-auto-var-init=pattern -Wno-error=deprecated-declarations' \
 -DAPPEND_CXXFLAGS='-std=c++23' \
 -DAPPEND_CPPFLAGS='-DARENA_DEBUG -DDEBUG_LOCKORDER' \
"
