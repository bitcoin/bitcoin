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

INSTALL_DIR=$(realpath "${MY_DIR}/../target")

# autogen always assumes that the user is calling it from the same directory
# in which the script is placed. This is not always true (like in our case),
# but but we have to adapt, so let's cd one level lower.
cd "${MY_DIR}/.."
"${MY_DIR}"/../autogen.sh

# NB: The `--enable-debug` configure option included here adds
#     `-DDEBUG_LOCKORDER` to the compiler flags. Hence, the flag
#     -DDEBUG_LOCKORDER has to be used also in the compilation of itcoin-pbft,
#     in order to prevent an unpredictable behaviour.
#     See developer-notes.md for more information on DEBUG_LOCKORDER.
"${MY_DIR}"/../configure \
    --prefix="${INSTALL_DIR}" \
    --enable-c++17 \
    --enable-debug \
    --enable-determinism \
    --enable-suppress-external-warnings \
    --enable-wallet \
    --enable-werror \
    --disable-bench \
    --disable-gui-tests \
    --disable-man \
    --disable-tests \
    --with-boost="${MY_DIR}/../../itcoin-pbft/usrlocal" \
    --with-incompatible-bdb \
    --with-zmq \
    --without-gui \
    --without-miniupnpc \
    LDFLAGS="-static" \
    CXX="g++-10" \
    CC="gcc-10"
