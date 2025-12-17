#!/usr/bin/env bash
#
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -o errexit -o pipefail -o xtrace

CFG_DONE="${BASE_ROOT_DIR}/ci.base-install-done"  # Use a global setting to remember whether this script ran to avoid running it twice

if [ "$( cat "${CFG_DONE}" || true )" == "done" ]; then
  echo "Skip base install"
  exit 0
fi

MAKEJOBS="-j$( nproc )"  # Use nproc, because MAKEJOBS is the default in docker image builds.

if [ -n "$DPKG_ADD_ARCH" ]; then
  dpkg --add-architecture "$DPKG_ADD_ARCH"
fi

if [ -n "${APT_LLVM_V}" ]; then
  ${CI_RETRY_EXE} apt-get update
  ${CI_RETRY_EXE} apt-get install curl -y
  curl "https://apt.llvm.org/llvm-snapshot.gpg.key" | tee "/etc/apt/trusted.gpg.d/apt.llvm.org.asc"
  (
    # shellcheck disable=SC2034
    source /etc/os-release
    echo "deb http://apt.llvm.org/${VERSION_CODENAME}/ llvm-toolchain-${VERSION_CODENAME}-${APT_LLVM_V} main" > "/etc/apt/sources.list.d/llvm-toolchain-${VERSION_CODENAME}-${APT_LLVM_V}.list"
  )
fi

if [[ $CI_IMAGE_NAME_TAG == *alpine* ]]; then
  ${CI_RETRY_EXE} apk update
  # shellcheck disable=SC2086
  ${CI_RETRY_EXE} apk add --no-cache $CI_BASE_PACKAGES $PACKAGES
elif [ "$CI_OS_NAME" != "macos" ]; then
  if [[ -n "${APPEND_APT_SOURCES_LIST}" ]]; then
    echo "${APPEND_APT_SOURCES_LIST}" >> /etc/apt/sources.list
  fi
  ${CI_RETRY_EXE} apt-get update
  # shellcheck disable=SC2086
  ${CI_RETRY_EXE} apt-get install --no-install-recommends --no-upgrade -y $PACKAGES $CI_BASE_PACKAGES
fi

if [ -n "${APT_LLVM_V}" ]; then
  update-alternatives --install /usr/bin/clang++ clang++ "/usr/bin/clang++-${APT_LLVM_V}" 100
  update-alternatives --install /usr/bin/clang clang "/usr/bin/clang-${APT_LLVM_V}" 100
  update-alternatives --install /usr/bin/llvm-symbolizer llvm-symbolizer "/usr/bin/llvm-symbolizer-${APT_LLVM_V}" 100
fi

if [ -n "$PIP_PACKAGES" ]; then
  # shellcheck disable=SC2086
  ${CI_RETRY_EXE} pip3 install --user $PIP_PACKAGES
fi

if [[ -n "${USE_INSTRUMENTED_LIBCPP}" ]]; then
  ${CI_RETRY_EXE} git clone --depth=1 https://github.com/llvm/llvm-project -b "llvmorg-21.1.5" /llvm-project

  cmake -G Ninja -B /cxx_build/ \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_USE_SANITIZER="${USE_INSTRUMENTED_LIBCPP}" \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DLLVM_TARGETS_TO_BUILD=Native \
    -DLLVM_ENABLE_PER_TARGET_RUNTIME_DIR=OFF \
    -DLIBCXXABI_USE_LLVM_UNWINDER=OFF \
    -DLIBCXX_ABI_DEFINES="_LIBCPP_ABI_BOUNDED_ITERATORS;_LIBCPP_ABI_BOUNDED_ITERATORS_IN_STD_ARRAY;_LIBCPP_ABI_BOUNDED_ITERATORS_IN_STRING;_LIBCPP_ABI_BOUNDED_ITERATORS_IN_VECTOR;_LIBCPP_ABI_BOUNDED_UNIQUE_PTR" \
    -DLIBCXX_HARDENING_MODE=debug \
    -S /llvm-project/runtimes

  ninja -C /cxx_build/ "$MAKEJOBS"

  # Clear no longer needed source folder
  du -sh /llvm-project
  rm -rf /llvm-project
fi

if [[ "${RUN_TIDY}" == "true" ]]; then
  ${CI_RETRY_EXE} git clone --depth=1 https://github.com/include-what-you-use/include-what-you-use -b clang_"${TIDY_LLVM_V}" /include-what-you-use
  cmake -B /iwyu-build/ -G 'Unix Makefiles' -DCMAKE_PREFIX_PATH=/usr/lib/llvm-"${TIDY_LLVM_V}" -S /include-what-you-use
  make -C /iwyu-build/ install "$MAKEJOBS"
fi

mkdir -p "${DEPENDS_DIR}/SDKs" "${DEPENDS_DIR}/sdk-sources"

OSX_SDK_BASENAME="Xcode-${XCODE_VERSION}-${XCODE_BUILD_ID}-extracted-SDK-with-libcxx-headers"

if [ -n "$XCODE_VERSION" ] && [ ! -d "${DEPENDS_DIR}/SDKs/${OSX_SDK_BASENAME}" ]; then
  OSX_SDK_FILENAME="${OSX_SDK_BASENAME}.tar"
  OSX_SDK_PATH="${DEPENDS_DIR}/sdk-sources/${OSX_SDK_FILENAME}"
  if [ ! -f "$OSX_SDK_PATH" ]; then
    ${CI_RETRY_EXE} curl --location --fail "${SDK_URL}/${OSX_SDK_FILENAME}" -o "$OSX_SDK_PATH"
  fi
  tar -C "${DEPENDS_DIR}/SDKs" -xf "$OSX_SDK_PATH"
fi

echo -n "done" > "${CFG_DONE}"
