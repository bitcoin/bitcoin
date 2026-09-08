#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -o errexit -o nounset -o pipefail -o xtrace

[ "${CI_CONFIG+x}" ] && source "$CI_CONFIG"

: "${CI_DIR:=build}"
if ! [ -v BUILD_TARGETS ]; then
  BUILD_TARGETS=(all tests mpexamples)
fi

[ -n "${CI_CLEAN-}" ] && rm -rf "${CI_DIR}"

cmake --version
cmake_ver=$(cmake --version | awk '/version/{print $3; exit}')
ver_ge() { [ "$(printf '%s\n' "$2" "$1" | sort -V | head -n1)" = "$2" ]; }

# If CAPNP_CHECKOUT was requested, clone and install requested Cap'n Proto branch or tag
capnp_prefix=
if [ -n "${CAPNP_CHECKOUT-}" ]; then
  capnp_prefix="$PWD/capnp-install"
  [ -e "capnp" ] || git clone -b "${CAPNP_CHECKOUT}" "https://github.com/capnproto/capnproto" capnp
  mkdir -p capnp/build
  (
    cd capnp/build
    git --no-pager log -1 || true
    CXXFLAGS="-std=c++20" cmake .. "-DCMAKE_INSTALL_PREFIX=${capnp_prefix}" -DBUILD_TESTING=OFF -DWITH_OPENSSL=OFF -DWITH_ZLIB=OFF
    cmake --build .
    cmake --install .
  )
  export CMAKE_PREFIX_PATH="${capnp_prefix}:${CMAKE_PREFIX_PATH-}"
fi

src_dir=$PWD
mkdir -p "$CI_DIR"
cd "$CI_DIR"
export CMAKE_BUILD_PARALLEL_LEVEL="$(nproc)"
git --no-pager log -1 || true
cmake "$src_dir" "${CMAKE_ARGS[@]+"${CMAKE_ARGS[@]}"}"
if ver_ge "$cmake_ver" "3.15"; then
  cmake --build . -t "${BUILD_TARGETS[@]}" -- "${BUILD_ARGS[@]+"${BUILD_ARGS[@]}"}"
else
  # Older versions of cmake can only build one target at a time with --target,
  # and do not support -t shortcut
  for t in "${BUILD_TARGETS[@]}"; do
    cmake --build . --target "$t" -- "${BUILD_ARGS[@]+"${BUILD_ARGS[@]}"}"
  done
fi
ctest --output-on-failure
