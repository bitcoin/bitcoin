#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
BUILD_DIR=${1:-build}


windows_build()
{
  if [ -d "${SCRIPT_DIR}/../cybozulib_ext"  ]; then
      DOWNLOAD_CYBOZULIB_EXT="OFF"
      CYBOZULIB_EXT_OPTION="-DMCL_CYBOZULIB_EXT_DIR:PATH=${SCRIPT_DIR}/../cybozulib_ext"
  else
      DOWNLOAD_CYBOZULIB_EXT="ON"
      CYBOZULIB_EXT_OPTION=""
  fi

  cmake -E remove_directory ${BUILD_DIR}
  cmake -E make_directory ${BUILD_DIR}
  cmake -H${SCRIPT_DIR} -B${BUILD_DIR} -A x64 \
    -DBUILD_TESTING=ON \
    -DMCL_BUILD_SAMPLE=ON \
    -DCMAKE_INSTALL_PREFIX=${BUILD_DIR}/install \
    -DMCL_DOWNLOAD_SOURCE=${DOWNLOAD_CYBOZULIB_EXT} ${CYBOZULIB_EXT_OPTION}
  cmake --build ${BUILD_DIR} --clean-first --config Release --parallel
}

linux_build()
{
  cmake -E remove_directory ${BUILD_DIR}
  cmake -E make_directory ${BUILD_DIR}
  cmake -H${SCRIPT_DIR} -B${BUILD_DIR} -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON \
    -DMCL_BUILD_SAMPLE=ON \
    -DMCL_USE_LLVM=ON \
    -DMCL_USE_OPENSSL=ON \
    -DCMAKE_INSTALL_PREFIX=${BUILD_DIR}/install
  cmake --build ${BUILD_DIR} --clean-first -- -j
}

osx_build()
{
  OPENSSL_ROOT_DIR="/usr/local/opt/openssl"

  cmake -E remove_directory ${BUILD_DIR}
  cmake -E make_directory ${BUILD_DIR}
  cmake -H${SCRIPT_DIR} -B${BUILD_DIR} -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON \
    -DMCL_BUILD_SAMPLE=ON \
    -DMCL_USE_LLVM=ON \
    -DMCL_USE_OPENSSL=ON \
    -DOPENSSL_ROOT_DIR="${OPENSSL_ROOT_DIR}" \
    -DCMAKE_INSTALL_PREFIX=${BUILD_DIR}/install
  cmake --build ${BUILD_DIR} --clean-first -- -j
}

os=`uname -s`
case "${os}" in
  CYGWIN*|MINGW32*|MSYS*|MINGW*)
    windows_build
    ;;
  Darwin*)
    osx_build
    ;;
  *)
    linux_build
    ;;
esac
