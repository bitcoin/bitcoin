#!/usr/bin/env bash
#
# Itcoin
#
# This script configures the build for itcoin-core. It can be used both on bare
# metal and inside a Docker container.
#
# Normally, the system's default C/C++ compilers are used. If you want to use
# a specific compiler, define the environment variables CC and CCX before
# calling the script.
#
# Please note that this script takes care of a bug in secp256k1's configure
# script (at least as of 2022-07-08), that needs to set both CC and
# CC_FOR_BUILD.
#
# EXAMPLE with custom compilers:
#     $ export CC=/path/to/c-compiler
#     $ export CXX=/path/to/c++-compiler
#     $ ./configure-itcoin-core.sh

set -eu

# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself#246128
MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [[ ! -z "${CC-}" ]]; then
    # The user requested a custom C compiler. Let's also set CC_FOR_BUILD to the
    # same value, otherwise secp256k1 does not passes the configure step (tested
    # on itcoin v0.21.x, Fedora 36, gcc 12.1).
    echo "Custom C compiler ${CC} requested. Also setting the CC_FOR_BUILD variable"
    export CC_FOR_BUILD="${CC}"
fi

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
    --disable-bench \
    --disable-gui-tests \
    --disable-man \
    --disable-tests \
    --disable-werror \
    --with-boost="${MY_DIR}/../../itcoin-pbft/build/usrlocal" \
    --with-incompatible-bdb \
    --with-zmq \
    --without-gui \
    --without-miniupnpc \
    LDFLAGS="-static" \
