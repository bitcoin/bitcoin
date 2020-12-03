#!/bin/bash
#
# ItCoin
#
# This script configures the build for itcoin-core. It can be used both on bare
# metal and inside a Docker container.
#
# Author: Antonio Muci <antonio.muci@bancaditalia.it>

set -eu

# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself#246128
MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

INSTALL_DIR="${MY_DIR}/target"

"${MY_DIR}"/autogen.sh

"${MY_DIR}"/configure \
    --prefix="${INSTALL_DIR}" \
    --enable-determinism \
    --enable-wallet \
    --enable-werror \
    --enable-c++17 \
    --disable-tests \
    --disable-gui-tests \
    --disable-bench \
    --disable-man \
    --with-incompatible-bdb \
    --with-zmq \
    --without-miniupnpc \
    LDFLAGS="-static" \
    CXX="g++-10" \
    CC="gcc-10"
