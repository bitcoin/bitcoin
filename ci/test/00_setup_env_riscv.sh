#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=riscv64-linux-gnu
export QEMU_USER_CMD="qemu-riscv64 -L /usr/riscv64-linux-gnu/"
export PACKAGES="python3 g++-riscv64-linux-gnu qemu-user"
export RUN_UNIT_TESTS=true
export RUN_FUNCTIONAL_TESTS=false
export GOAL="install"
export BITCOIN_CONFIG="--enable-glibc-back-compat --enable-reduce-exports"
