#!/usr/bin/env bash
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -ex

CFG_DONE="ci.base-install-done"  # Use a global git setting to remember whether this script ran to avoid running it twice

if [ "$(git config --global ${CFG_DONE})" == "true" ]; then
  echo "Skip base install"
  exit 0
fi

if [ -n "$DPKG_ADD_ARCH" ]; then
  dpkg --add-architecture "$DPKG_ADD_ARCH"
fi

if [[ $CI_IMAGE_NAME_TAG == *centos* ]]; then
  bash -c "dnf -y install epel-release"
  bash -c "dnf -y --allowerasing install $CI_BASE_PACKAGES $PACKAGES"
elif [ "$CI_OS_NAME" != "macos" ]; then
  if [[ -n "${APPEND_APT_SOURCES_LIST}" ]]; then
    echo "${APPEND_APT_SOURCES_LIST}" >> /etc/apt/sources.list
  fi
  ${CI_RETRY_EXE} apt-get update
  ${CI_RETRY_EXE} bash -c "apt-get install --no-install-recommends --no-upgrade -y $PACKAGES $CI_BASE_PACKAGES"
fi

if [ -n "$PIP_PACKAGES" ]; then
  # shellcheck disable=SC2086
  ${CI_RETRY_EXE} pip3 install --user $PIP_PACKAGES
fi

if [[ ${USE_MEMORY_SANITIZER} == "true" ]]; then
  ${CI_RETRY_EXE} git clone --depth=1 https://github.com/llvm/llvm-project -b llvmorg-17.0.2 /msan/llvm-project

  cmake -G Ninja -B /msan/clang_build/ \
    -DLLVM_ENABLE_PROJECTS="clang" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_TARGETS_TO_BUILD=Native \
    -DLLVM_ENABLE_RUNTIMES="compiler-rt;libcxx;libcxxabi;libunwind" \
    -S /msan/llvm-project/llvm

  ninja -C /msan/clang_build/ "-j$( nproc )"  # Use nproc, because MAKEJOBS is the default in docker image builds
  ninja -C /msan/clang_build/ install-runtimes

  update-alternatives --install /usr/bin/clang++ clang++ /msan/clang_build/bin/clang++ 100
  update-alternatives --install /usr/bin/clang clang /msan/clang_build/bin/clang 100
  update-alternatives --install /usr/bin/llvm-symbolizer llvm-symbolizer /msan/clang_build/bin/llvm-symbolizer 100

  cmake -G Ninja -B /msan/cxx_build/ \
    -DLLVM_ENABLE_RUNTIMES='libcxx;libcxxabi' \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_USE_SANITIZER=MemoryWithOrigins \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DLLVM_TARGETS_TO_BUILD=Native \
    -DLLVM_ENABLE_PER_TARGET_RUNTIME_DIR=OFF \
    -DLIBCXX_HARDENING_MODE=debug \
    -S /msan/llvm-project/runtimes

  ninja -C /msan/cxx_build/ "-j$( nproc )"  # Use nproc, because MAKEJOBS is the default in docker image builds
fi

if [[ "${RUN_TIDY}" == "true" ]]; then
  ${CI_RETRY_EXE} git clone https://github.com/include-what-you-use/include-what-you-use -b master /include-what-you-use
  git -C /include-what-you-use checkout a138eaac254e5a472464e31d5ec418fe6e6f1fc7
  cmake -B /iwyu-build/ -G 'Unix Makefiles' -DCMAKE_PREFIX_PATH=/usr/lib/llvm-"${TIDY_LLVM_V}" -S /include-what-you-use
  make -C /iwyu-build/ install "-j$( nproc )"  # Use nproc, because MAKEJOBS is the default in docker image builds
fi

mkdir -p "${DEPENDS_DIR}/SDKs" "${DEPENDS_DIR}/sdk-sources"

OSX_SDK_BASENAME="Xcode-${XCODE_VERSION}-${XCODE_BUILD_ID}-extracted-SDK-with-libcxx-headers"

if [ -n "$XCODE_VERSION" ] && [ ! -d "${DEPENDS_DIR}/SDKs/${OSX_SDK_BASENAME}" ]; then
  OSX_SDK_FILENAME="${OSX_SDK_BASENAME}.tar.gz"
  OSX_SDK_PATH="${DEPENDS_DIR}/sdk-sources/${OSX_SDK_FILENAME}"
  if [ ! -f "$OSX_SDK_PATH" ]; then
    ${CI_RETRY_EXE} curl --location --fail "${SDK_URL}/${OSX_SDK_FILENAME}" -o "$OSX_SDK_PATH"
  fi
  tar -C "${DEPENDS_DIR}/SDKs" -xf "$OSX_SDK_PATH"
fi

if [ -n "$ANDROID_HOME" ] && [ ! -d "$ANDROID_HOME" ]; then
  ANDROID_TOOLS_PATH=${DEPENDS_DIR}/sdk-sources/android-tools.zip
  if [ ! -f "$ANDROID_TOOLS_PATH" ]; then
    ${CI_RETRY_EXE} curl --location --fail "${ANDROID_TOOLS_URL}" -o "$ANDROID_TOOLS_PATH"
  fi
  mkdir -p "$ANDROID_HOME"
  unzip -o "$ANDROID_TOOLS_PATH" -d "$ANDROID_HOME"
  yes | "${ANDROID_HOME}"/cmdline-tools/bin/sdkmanager --sdk_root="${ANDROID_HOME}" --install "build-tools;${ANDROID_BUILD_TOOLS_VERSION}" "platform-tools" "platforms;android-31" "platforms;android-${ANDROID_API_LEVEL}" "ndk;${ANDROID_NDK_VERSION}"
fi

git config --global ${CFG_DONE} "true"
