#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_android
export PACKAGES="clang llvm unzip openjdk-8-jdk gradle"

export ANDROID_API_LEVEL=28
export ANDROID_BUILD_TOOLS_VERSION=28.0.3
export ANDROID_NDK_VERSION=21.1.6352462
export ANDROID_TOOLS_URL=https://dl.google.com/android/repository/commandlinetools-linux-6609375_latest.zip

export BITCOIN_CONFIG="--disable-ccache"