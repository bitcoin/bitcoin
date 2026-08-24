#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_netbsd_cross
export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:26.04"
export APT_LLVM_V="22"
export HOST=x86_64-unknown-netbsd
export NETBSD_VERSION=11.0
export NETBSD_SDK_BASENAME="netbsd-${HOST}-${NETBSD_VERSION}"
export NETBSD_SDK_SHA512SUMS="\
e8871bbedb8c3e0f696cc2596ced0c1e6497939f725fb3495b8d2c168430325907550f5f840f4dd0e3c73e6090394747c5e54762f2737de81177b984403522a8  base.tar.xz\n\
d8df6c07e9142dd8189292b769ac312f86185a6a278a752c18c840f7cd3a8dd3c535f9b0c8e06b62d556b2c75b97a01d786184e8a18f5e080ccd213591c8628f  comp.tar.xz"
export NETBSD_GCC_PACKAGE_DIR="https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/x86_64/${NETBSD_VERSION}/All"
export NETBSD_GCC_SHA512SUMS="\
00509af9aee23b1b7cd503a2c984c0339f344a7202f134f9279d142060bd75e7746601e868bf732b95d942f78a3b5c8831ddf197404b227dfbf5e17f1a37a40e  gcc14-14.3.0nb1.tgz\n\
8d2d02aa4002cbedf4223bb709c6303059ec26c2d2e69005b0fb256783a2d7d13853a79a8cc6942866b40f95ce826d5f02fd1b27802b2e3f90d38ab7c4f70cca  gcc14-libs-14.3.0nb2.tgz"
export PACKAGES="clang-${APT_LLVM_V} llvm-${APT_LLVM_V} lld-${APT_LLVM_V}"
export SYSROOT="--sysroot=${DEPENDS_DIR}/SDKs/${NETBSD_SDK_BASENAME}"
export NETBSD_GCC_ROOT="${DEPENDS_DIR}/SDKs/${NETBSD_SDK_BASENAME}/usr/pkg/gcc14"
export NETBSD_GCC_CXXFLAGS="-nostdinc++ -isystem ${NETBSD_GCC_ROOT}/include/c++ -isystem ${NETBSD_GCC_ROOT}/include/c++/x86_64--netbsd -isystem ${NETBSD_GCC_ROOT}/include/c++/backward"
export DEP_OPTS="build_CC=clang build_CXX=clang++ \
 CC='clang --target=${HOST} ${SYSROOT}' \
 CXX='clang++ --target=${HOST} ${SYSROOT} ${NETBSD_GCC_CXXFLAGS}' \
 LDFLAGS='-fuse-ld=lld -stdlib=libstdc++ -L${NETBSD_GCC_ROOT}/lib -lgcc_s' \
 AR=llvm-ar-${APT_LLVM_V} \
 NM=llvm-nm-${APT_LLVM_V} \
 OBJCOPY=llvm-objcopy-${APT_LLVM_V} \
 OBJDUMP=llvm-objdump-${APT_LLVM_V} \
 RANLIB=llvm-ranlib-${APT_LLVM_V} \
 STRIP=llvm-strip-${APT_LLVM_V}"
export GOAL="install"
printf -v BITCOIN_CONFIG "%q " \
 --preset=dev-mode \
 -DBUILD_GUI=OFF \
 -DREDUCE_EXPORTS=ON \
 -DWITH_USDT=OFF
export BITCOIN_CONFIG
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
