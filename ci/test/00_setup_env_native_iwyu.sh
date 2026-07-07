#!/usr/bin/env bash
#
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:26.04"  # To build codegen, CMake must be 3.31 or newer.
export CONTAINER_NAME=ci_native_iwyu
export IWYU_LLVM_V="22"
export APT_LLVM_V="${IWYU_LLVM_V}"
export PACKAGES="clang-${IWYU_LLVM_V} clang-format-${IWYU_LLVM_V} libclang-${IWYU_LLVM_V}-dev llvm-${IWYU_LLVM_V}-dev jq libboost-dev libzmq3-dev systemtap-sdt-dev qt6-base-dev qt6-tools-dev qt6-l10n-tools libqrencode-dev libsqlite3-dev libcapnp-dev capnproto"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=false
export RUN_CHECK_DEPS=false
export RUN_IWYU=true
# Adding non-codegen targets to the build goal is a workaround
# for https://gitlab.kitware.com/cmake/cmake/-/work_items/27862
# and https://github.com/bitcoin-core/libmultiprocess/issues/284.
export GOAL="codegen mp_headers mptest_headers bitcoin_ipc_headers bitcoin_ipc_test_headers bitcoin_ipc_fuzz_headers"
export BITCOIN_CONFIG="\
 --preset dev-mode -DBUILD_GUI=OFF \
 -DCMAKE_C_COMPILER=clang-${IWYU_LLVM_V} \
 -DCMAKE_CXX_COMPILER=clang++-${IWYU_LLVM_V} \
"
