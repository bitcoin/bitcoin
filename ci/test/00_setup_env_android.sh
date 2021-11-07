#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=aarch64-linux-android
export PACKAGES="clang llvm unzip openjdk-8-jdk gradle"
export CONTAINER_NAME=ci_android
export DOCKER_NAME_TAG="ubuntu:focal"

export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false

export ANDROID_API_LEVEL=28
export ANDROID_BUILD_TOOLS_VERSION=28.0.3
export ANDROID_NDK_VERSION=22.1.7171670
export ANDROID_TOOLS_URL=https://dl.google.com/android/repository/commandlinetools-linux-6609375_latest.zip
export ANDROID_HOME="${DEPENDS_DIR}/SDKs/android"
export ANDROID_NDK_HOME="${ANDROID_HOME}/ndk/${ANDROID_NDK_VERSION}"
export DEP_OPTS="ANDROID_SDK=${ANDROID_HOME} ANDROID_NDK=${ANDROID_NDK_HOME} ANDROID_API_LEVEL=${ANDROID_API_LEVEL} ANDROID_TOOLCHAIN_BIN=${ANDROID_NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/bin/"

export BITCOIN_CONFIG="--disable-ccache"
