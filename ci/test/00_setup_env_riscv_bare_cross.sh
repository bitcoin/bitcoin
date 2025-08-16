#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_riscv_bare

export GOAL="bitcoin_consensus bitcoin_crypto secp256k1"
export CI_IMAGE_NAME_TAG="docker.io/ubuntu:24.04"
export HOST="riscv32-unknown-elf-gcc"
export PACKAGES="autoconf automake autotools-dev curl python3 python3-pip libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build git cmake libglib2.0-dev libslirp-dev"
export BITCOIN_CONFIG="-DCMAKE_C_COMPILER=/opt/riscv-ilp32/bin/riscv32-unknown-elf-gcc \
 -DCMAKE_CXX_COMPILER=/opt/riscv-ilp32/bin/riscv32-unknown-elf-g++ \
 -DBUILD_KERNEL_LIB=OFF \
 -DBUILD_UTIL_CHAINSTATE=OFF \
 -DBUILD_TESTS=OFF \
 -DBUILD_BENCH=OFF \
 -DBUILD_FUZZ_BINARY=OFF \
 -DBUILD_DAEMON=OFF \
 -DBUILD_TX=OFF \
 -DBUILD_UTIL=OFF \
 -DBUILD_CLI=OFF \
 -DBUILD_BITCOIN_BIN=OFF \
 -DENABLE_WALLET=OFF \
 -DENABLE_EXTERNAL_SIGNER=OFF \
 -DCMAKE_SYSTEM_NAME=Generic \
 -DIFADDR_LINKS_WITHOUT_LIBSOCKET=ON \
 "

export BARE_METAL_RISCV="true"
export RUN_UNIT_TESTS="false"
export RUN_FUNCTIONAL_TESTS="false"
export NO_DEPENDS="true"
