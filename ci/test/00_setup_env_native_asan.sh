#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:24.04"

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
export APT_LLVM_V="21"
export PACKAGES="systemtap-sdt-dev clang-${APT_LLVM_V} llvm-${APT_LLVM_V} libclang-rt-${APT_LLVM_V}-dev python3-zmq qt6-base-dev qt6-tools-dev qt6-l10n-tools libevent-dev libboost-dev libzmq3-dev libqrencode-dev libsqlite3-dev ${BPFCC_PACKAGE} libcapnp-dev capnproto python3-pip"
export PIP_PACKAGES="--break-system-packages pycapnp"
export NO_DEPENDS=1
export GOAL="install"
export CI_LIMIT_STACK_SIZE=1
export BITCOIN_CONFIG="\
 -DWITH_USDT=ON -DWITH_ZMQ=ON -DBUILD_GUI=ON \
 -DSANITIZERS=address,float-divide-by-zero,integer,undefined \
 -DCMAKE_C_COMPILER=clang \
 -DCMAKE_CXX_COMPILER=clang++ \
 -DCMAKE_C_FLAGS='-ftrivial-auto-var-init=pattern' \
 -DCMAKE_CXX_FLAGS='-ftrivial-auto-var-init=pattern' \
 -DAPPEND_CXXFLAGS='-std=c++23' \
 -DAPPEND_CPPFLAGS='-DARENA_DEBUG -DDEBUG_LOCKORDER' \
"
