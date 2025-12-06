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

if [[ "${RUN_IWYU}" == true ]]; then
  ${CI_RETRY_EXE} git clone --depth=1 https://github.com/include-what-you-use/include-what-you-use -b clang_"${TIDY_LLVM_V}" /include-what-you-use
  # Prefer angled brackets over quotes for include directives.
  # See: https://en.cppreference.com/w/cpp/preprocessor/include.html.
  tee >(cd /include-what-you-use && patch -p1) <<'EOF'
--- a/iwyu_path_util.cc
+++ b/iwyu_path_util.cc
@@ -211,7 +211,7 @@ bool IsQuotedInclude(const string& s) {
 }

 string AddQuotes(string include_name, bool angled) {
-  if (angled) {
+  if (true) {
       return "<" + include_name + ">";
   }
   return "\"" + include_name + "\"";
EOF
  # Prefer C++ headers over C counterparts.
  # See: https://github.com/include-what-you-use/include-what-you-use/blob/clang_21/iwyu_include_picker.cc#L587-L629.
  sed -i "s|\"<assert.h>\", kPublic|\"<assert.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<complex.h>\", kPublic|\"<complex.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<ctype.h>\", kPublic|\"<ctype.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<errno.h>\", kPublic|\"<errno.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<fenv.h>\", kPublic|\"<fenv.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<float.h>\", kPublic|\"<float.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<inttypes.h>\", kPublic|\"<inttypes.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<iso646.h>\", kPublic|\"<iso646.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<limits.h>\", kPublic|\"<limits.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<locale.h>\", kPublic|\"<locale.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<math.h>\", kPublic|\"<math.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<setjmp.h>\", kPublic|\"<setjmp.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<signal.h>\", kPublic|\"<signal.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<stdalign.h>\", kPublic|\"<stdalign.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<stdarg.h>\", kPublic|\"<stdarg.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<stdbool.h>\", kPublic|\"<stdbool.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<stddef.h>\", kPublic|\"<stddef.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<stdint.h>\", kPublic|\"<stdint.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<stdio.h>\", kPublic|\"<stdio.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<stdlib.h>\", kPublic|\"<stdlib.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<string.h>\", kPublic|\"<string.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<tgmath.h>\", kPublic|\"<tgmath.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<time.h>\", kPublic|\"<time.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<uchar.h>\", kPublic|\"<uchar.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<wchar.h>\", kPublic|\"<wchar.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc
  sed -i "s|\"<wctype.h>\", kPublic|\"<wctype.h>\", kPrivate|g" /include-what-you-use/iwyu_include_picker.cc

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
