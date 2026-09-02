#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

export LC_ALL=C.UTF-8

export CI_OS_NAME=freebsd
export BSD_RELEASE=15.1
export PACKAGES="shells/bash devel/bison devel/ccache4 devel/cmake-core devel/gmake devel/ninja curl devel/pkgconf devel/py-pip databases/py-sqlite3 lang/python3 net/py-pyzmq net/rsync sysutils/lsof sysutils/lscpu x11-fonts/fontconfig"
export PIP_PACKAGES="pycapnp"
export MAKE=gmake
export TEST_RUNNER_TIMEOUT_FACTOR=8
export TEST_RUNNER_EXTRA="--extended"
# The VM preparation step sources this file with /bin/sh.
export BITCOIN_CONFIG="--preset=dev-mode -DREDUCE_EXPORTS=ON -DWITH_USDT=OFF"
