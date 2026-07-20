#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

# This config is experimental, and may not be reproducible, given
# the use of a rolling distro.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_chimera_musl
export CI_IMAGE_NAME_TAG="mirror.gcr.io/chimeralinux/chimera@sha256:29102d7e12a1f464707d7aba19ce53e652d277861838ed4129178d0655444b1a"
export CI_BASE_PACKAGES="ccache chimerautils chimerautils-extra clang cmake curl e2fsprogs git gmake gtar linux-headers procps python-devel python-pip rsync util-linux util-linux-lscpu"
export PIP_PACKAGES="--break-system-packages --require-hashes -r ${BASE_ROOT_DIR}/ci/test/requirements/pycapnp.txt -r ${BASE_ROOT_DIR}/ci/test/requirements/pyzmq.txt"
# NO_QT=1 because Qt needs various patches: https://github.com/chimera-linux/cports/tree/master/main/qt6-qtbase/patches
export DEP_OPTS="build_CC=clang build_CXX=clang++ build_TAR=gtar AR=llvm-ar CC=clang CXX=clang++ NM=llvm-nm RANLIB=llvm-ranlib STRIP=llvm-strip NO_QT=1"
export GOAL="install"
export BITCOIN_CONFIG="\
 --preset=dev-mode \
 -DBUILD_GUI=OFF \
 -DREDUCE_EXPORTS=ON \
 -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
"
