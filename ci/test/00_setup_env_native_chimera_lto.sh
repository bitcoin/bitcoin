#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

# This config is experimental, and may not be reproducible, given
# the use of a rolling distro.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_chimera_musl
export CI_IMAGE_NAME_TAG="mirror.gcr.io/chimeralinux/chimera"
export CI_BASE_PACKAGES="ccache chimerautils chimerautils-extra clang cmake curl e2fsprogs git gmake gtar linux-headers ninja pkgconf procps python-devel python-pip rsync util-linux util-linux-lscpu xz"
export PIP_PACKAGES="--break-system-packages pyzmq pycapnp"
export DEP_OPTS="build_CC=clang build_CXX=clang++ build_TAR=gtar AR=llvm-ar CC=clang CXX=clang++ NM=llvm-nm RANLIB=llvm-ranlib STRIP=llvm-strip NO_QT=1"
export GOAL="install"
export BITCOIN_CONFIG="\
 --preset=dev-mode \
 -DBUILD_GUI=OFF \
 -DREDUCE_EXPORTS=ON \
 -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
"
