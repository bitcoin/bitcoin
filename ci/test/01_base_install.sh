#!/usr/bin/env bash
#
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -o errexit -o pipefail -o xtrace

if [ "${DANGER_RUN_CI_ON_HOST}" != "1" ]; then
  echo "This script will make unsafe local and global modifications, so it can only be run inside a container and requires DANGER_RUN_CI_ON_HOST=1"
  exit 1
fi

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

if command -v apk >/dev/null 2>&1; then
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
  ${CI_RETRY_EXE} git clone --depth=1 https://github.com/llvm/llvm-project -b "llvmorg-22.1.7" /llvm-project

# LLVM is configured with LIBCXXABI_USE_LLVM_UNWINDER=OFF,
# because libunwind doesn't handle exceptions under MSAN.
# https://github.com/llvm/llvm-project/issues/84348
  cmake -G Ninja -B /cxx_build/ \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" \
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

if [[ ${BARE_METAL_RISCV} == "true" ]]; then
    ${CI_RETRY_EXE} git clone --depth=1 https://github.com/riscv-collab/riscv-gnu-toolchain -b 2026.06.06 /riscv/gcc
    ( cd /riscv/gcc;
      ./configure --prefix=/opt/riscv-ilp32 --with-arch=rv32gc --with-abi=ilp32 --disable-gdb;
      make "$MAKEJOBS"; )
    rm -rf /riscv/gcc
fi

if [[ "${RUN_IWYU}" == true ]]; then
  ${CI_RETRY_EXE} git clone --depth=1 https://github.com/include-what-you-use/include-what-you-use -b clang_"${IWYU_LLVM_V}" /include-what-you-use
  pushd /include-what-you-use
  patch -p1 < /ci_container_base/ci/test/01_iwyu.patch
  patch -p1 < /ci_container_base/ci/test/02_iwyu_hash.patch
  popd
  cmake -B /iwyu-build/ -G 'Unix Makefiles' -DCMAKE_PREFIX_PATH=/usr/lib/llvm-"${IWYU_LLVM_V}" -S /include-what-you-use
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

if [ -n "$NETBSD_VERSION" ] && [ ! -d "${DEPENDS_DIR}/SDKs/${NETBSD_SDK_BASENAME}" ]; then
  mkdir -p "${DEPENDS_DIR}/SDKs/${NETBSD_SDK_BASENAME}"
  for NETBSD_SDK_FILENAME in base.tar.xz comp.tar.xz; do
    NETBSD_SDK_PATH="${DEPENDS_DIR}/sdk-sources/${NETBSD_SDK_FILENAME}"
    if [ ! -f "$NETBSD_SDK_PATH" ]; then
      ${CI_RETRY_EXE} curl --location --fail "https://cdn.netbsd.org/pub/NetBSD/NetBSD-${NETBSD_VERSION}/amd64/binary/sets/${NETBSD_SDK_FILENAME}" -o "$NETBSD_SDK_PATH"
    fi
    tar -C "${DEPENDS_DIR}/SDKs/${NETBSD_SDK_BASENAME}" -xf "$NETBSD_SDK_PATH"
  done
fi

if [ -n "$FREEBSD_VERSION" ] && [ ! -d "${DEPENDS_DIR}/SDKs/${FREEBSD_SDK_BASENAME}" ]; then
  FREEBSD_SDK_FILENAME="base-${FREEBSD_VERSION}.txz"
  FREEBSD_SDK_PATH="${DEPENDS_DIR}/sdk-sources/${FREEBSD_SDK_FILENAME}"
  if [ ! -f "$FREEBSD_SDK_PATH" ]; then
    ${CI_RETRY_EXE} curl --location --fail "https://download.freebsd.org/releases/amd64/${FREEBSD_VERSION}-RELEASE/base.txz" -o "$FREEBSD_SDK_PATH"
  fi
  mkdir -p "${DEPENDS_DIR}/SDKs/${FREEBSD_SDK_BASENAME}"
  tar -C "${DEPENDS_DIR}/SDKs/${FREEBSD_SDK_BASENAME}" -xf "$FREEBSD_SDK_PATH"
fi

if [ -n "$OPENBSD_VERSION" ] && [ ! -d "${DEPENDS_DIR}/SDKs/${OPENBSD_SDK_BASENAME}" ]; then
  mkdir -p "${DEPENDS_DIR}/SDKs/${OPENBSD_SDK_BASENAME}"
  for OPENBSD_SDK_FILENAME in base79.tgz comp79.tgz; do
    OPENBSD_SDK_PATH="${DEPENDS_DIR}/sdk-sources/${OPENBSD_SDK_FILENAME}"
    if [ ! -f "$OPENBSD_SDK_PATH" ]; then
      ${CI_RETRY_EXE} curl --location --fail "https://cdn.openbsd.org/pub/OpenBSD/${OPENBSD_VERSION}/amd64/${OPENBSD_SDK_FILENAME}" -o "$OPENBSD_SDK_PATH"
    fi
    tar -C "${DEPENDS_DIR}/SDKs/${OPENBSD_SDK_BASENAME}" -xf "$OPENBSD_SDK_PATH"
    (
      # The SDK has versioned shared libs, but no unversioned libfoo.so symlink,
      # which breaks linking the kernel with lld. Create the symlinks.
      cd "${DEPENDS_DIR}/SDKs/${OPENBSD_SDK_BASENAME}/usr/lib"
      ln -sf libc++abi.so.*.*  libc++abi.so
      ln -sf libc++.so.*.*     libc++.so
      ln -sf libpthread.so.*.* libpthread.so
    )
  done
fi

echo -n "done" > "${CFG_DONE}"
