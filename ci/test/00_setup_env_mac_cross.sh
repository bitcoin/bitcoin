#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export SDK_URL=${SDK_URL:-https://bitcoincore.org/depends-sources/sdks}

export CONTAINER_NAME=ci_macos_cross
export CI_IMAGE_NAME_TAG="docker.io/ubuntu:24.04"
export HOST=x86_64-apple-darwin
export MAC_CROSS_LLVM_V="17"
export DEP_OPTS="CC=clang-${MAC_CROSS_LLVM_V} CXX=clang++-${MAC_CROSS_LLVM_V}"
export PACKAGES="clang-${MAC_CROSS_LLVM_V} lld-${MAC_CROSS_LLVM_V} llvm-${MAC_CROSS_LLVM_V} zip"
export AR=llvm-ar-${MAC_CROSS_LLVM_V}
export RANLIB=llvm-ranlib-${MAC_CROSS_LLVM_V}
export STRIP=llvm-strip-${MAC_CROSS_LLVM_V}
export XCODE_VERSION=15.0
export XCODE_BUILD_ID=15A240d
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export GOAL="deploy"
export BITCOIN_CONFIG="--with-gui --enable-reduce-exports"
