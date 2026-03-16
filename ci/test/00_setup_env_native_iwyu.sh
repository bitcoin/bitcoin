#!/usr/bin/env bash
#
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CI_IMAGE_NAME_TAG="mirror.gcr.io/debian:trixie"  # To build codegen, CMake must be 3.31 or newer.
export CONTAINER_NAME=ci_native_iwyu
export TIDY_LLVM_V="21"
export APT_LLVM_V="${TIDY_LLVM_V}"
export PACKAGES="clang-${TIDY_LLVM_V} clang-format-${TIDY_LLVM_V} libclang-${TIDY_LLVM_V}-dev llvm-${TIDY_LLVM_V}-dev jq libevent-dev libboost-dev libzmq3-dev systemtap-sdt-dev qt6-base-dev qt6-tools-dev qt6-l10n-tools libqrencode-dev libsqlite3-dev libcapnp-dev capnproto"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=false
export RUN_CHECK_DEPS=false
export RUN_IWYU=true
export GOAL="codegen"
export BITCOIN_CONFIG="\
 --preset dev-mode -DBUILD_GUI=OFF \
 -DCMAKE_C_COMPILER=clang-${TIDY_LLVM_V} \
 -DCMAKE_CXX_COMPILER=clang++-${TIDY_LLVM_V} \
"
