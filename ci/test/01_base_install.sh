#!/usr/bin/env bash
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8


if [ -n "$DPKG_ADD_ARCH" ]; then
  dpkg --add-architecture "$DPKG_ADD_ARCH"
fi

if [[ $CI_IMAGE_NAME_TAG == *centos* ]]; then
  ${CI_RETRY_EXE} bash -c "dnf -y install epel-release"
  ${CI_RETRY_EXE} bash -c "dnf -y --allowerasing install $CI_BASE_PACKAGES $PACKAGES"
elif [ "$CI_USE_APT_INSTALL" != "no" ]; then
  if [[ "${ADD_UNTRUSTED_BPFCC_PPA}" == "true" ]]; then
    # Ubuntu 22.04 LTS and Debian 11 both have an outdated bpfcc-tools packages.
    # The iovisor PPA is outdated as well. The next Ubuntu and Debian releases will contain updated
    # packages. Meanwhile, use an untrusted PPA to install an up-to-date version of the bpfcc-tools
    # package.
    # TODO: drop this once we can use newer images in GCE
    add-apt-repository ppa:hadret/bpfcc
  fi
  if [[ -n "${APPEND_APT_SOURCES_LIST}" ]]; then
    echo "${APPEND_APT_SOURCES_LIST}" >> /etc/apt/sources.list
  fi
  ${CI_RETRY_EXE} apt-get update
  ${CI_RETRY_EXE} bash -c "apt-get install --no-install-recommends --no-upgrade -y $PACKAGES $CI_BASE_PACKAGES"
fi

if [ -n "$PIP_PACKAGES" ]; then
  if [ "$CI_OS_NAME" == "macos" ]; then
    sudo -H pip3 install --upgrade pip
    # shellcheck disable=SC2086
    IN_GETOPT_BIN="$(brew --prefix gnu-getopt)/bin/getopt" ${CI_RETRY_EXE} pip3 install --user $PIP_PACKAGES
  else
    # shellcheck disable=SC2086
    ${CI_RETRY_EXE} pip3 install --user $PIP_PACKAGES
  fi
fi

if [[ ${USE_MEMORY_SANITIZER} == "true" ]]; then
  update-alternatives --install /usr/bin/clang++ clang++ "$(which clang++-12)" 100
  update-alternatives --install /usr/bin/clang clang "$(which clang-12)" 100
  mkdir -p "${BASE_SCRATCH_DIR}"/msan/build/
  git clone --depth=1 https://github.com/llvm/llvm-project -b llvmorg-12.0.0 "${BASE_SCRATCH_DIR}"/msan/llvm-project
  cd "${BASE_SCRATCH_DIR}"/msan/build/ && cmake -DLLVM_ENABLE_PROJECTS='libcxx;libcxxabi' -DCMAKE_BUILD_TYPE=Release -DLLVM_USE_SANITIZER=MemoryWithOrigins -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DLLVM_TARGETS_TO_BUILD=X86 ../llvm-project/llvm/
  cd "${BASE_SCRATCH_DIR}"/msan/build/ && make "$MAKEJOBS" cxx
fi

if [[ "${RUN_TIDY}" == "true" ]]; then
  if [ ! -d "${DIR_IWYU}" ]; then
    mkdir -p "${DIR_IWYU}"/build/
    git clone --depth=1 https://github.com/include-what-you-use/include-what-you-use -b clang_15 "${DIR_IWYU}"/include-what-you-use
    cd "${DIR_IWYU}"/build && cmake -G 'Unix Makefiles' -DCMAKE_PREFIX_PATH=/usr/lib/llvm-15 ../include-what-you-use
    cd "${DIR_IWYU}"/build && make install "$MAKEJOBS"
  fi
fi

mkdir -p "${DEPENDS_DIR}/SDKs" "${DEPENDS_DIR}/sdk-sources"

OSX_SDK_BASENAME="Xcode-${XCODE_VERSION}-${XCODE_BUILD_ID}-extracted-SDK-with-libcxx-headers"

if [ -n "$XCODE_VERSION" ] && [ ! -d "${DEPENDS_DIR}/SDKs/${OSX_SDK_BASENAME}" ]; then
  OSX_SDK_FILENAME="${OSX_SDK_BASENAME}.tar.gz"
  OSX_SDK_PATH="${DEPENDS_DIR}/sdk-sources/${OSX_SDK_FILENAME}"
  if [ ! -f "$OSX_SDK_PATH" ]; then
    curl --location --fail "${SDK_URL}/${OSX_SDK_FILENAME}" -o "$OSX_SDK_PATH"
  fi
  tar -C "${DEPENDS_DIR}/SDKs" -xf "$OSX_SDK_PATH"
fi

if [ -n "$ANDROID_HOME" ] && [ ! -d "$ANDROID_HOME" ]; then
  ANDROID_TOOLS_PATH=${DEPENDS_DIR}/sdk-sources/android-tools.zip
  if [ ! -f "$ANDROID_TOOLS_PATH" ]; then
    curl --location --fail "${ANDROID_TOOLS_URL}" -o "$ANDROID_TOOLS_PATH"
  fi
  mkdir -p "$ANDROID_HOME"
  unzip -o "$ANDROID_TOOLS_PATH" -d "$ANDROID_HOME"
  yes | "${ANDROID_HOME}"/cmdline-tools/bin/sdkmanager --sdk_root="${ANDROID_HOME}" --install "build-tools;${ANDROID_BUILD_TOOLS_VERSION}" "platform-tools" "platforms;android-${ANDROID_API_LEVEL}" "ndk;${ANDROID_NDK_VERSION}"
fi
