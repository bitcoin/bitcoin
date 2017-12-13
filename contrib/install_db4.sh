#!/bin/sh

# Install libdb4.8 (Berkeley DB).

set -e

if [ -z "${1}" ]; then
  echo "Usage: ./install_db4.sh <base-dir> [<extra-bdb-configure-flag> ...]"
  echo
  echo "Must specify a single argument: the directory in which db5 will be built."
  echo "This is probably \`pwd\` if you're at the root of the bitcoin repository."
  exit 1
fi

expand_path() {
  echo "$(cd "${1}" && pwd -P)"
}

BDB_PREFIX="$(expand_path ${1})/db4"; shift;
BDB_EXTRA_CONFIGURE_FLAGS="${@}"
BDB_VERSION='db-4.8.30.NC'
BDB_HASH='12edc0df75bf9abd7f82f821795bcee50f42cb2e5f76a6a281b85732798364ef'
BDB_URL="https://download.oracle.com/berkeley-db/${BDB_VERSION}.tar.gz"

check_exists() {
  which "$1" >/dev/null 2>&1
}

sha256_check() {
  # Args: <sha256_hash> <filename>
  #
  if check_exists sha256sum; then
    echo "${1}  ${2}" | sha256sum -c
  elif check_exists sha256; then
    echo "${1}  ${2}" | sha256 -c
  else
    echo "${1}  ${2}" | shasum -a 256 -c
  fi
}

http_get() {
  # Args: <url> <filename> <sha256_hash>
  #
  # It's acceptable that we don't require SSL here because we manually verify
  # content hashes below.
  #
  if [ -f "${2}" ]; then
    echo "File ${2} already exists; not downloading again"
  elif check_exists curl; then
    curl --insecure "${1}" -o "${2}"
  else
    wget --no-check-certificate "${1}" -O "${2}"
  fi

  sha256_check "${3}" "${2}"
}

mkdir -p "${BDB_PREFIX}"
http_get "${BDB_URL}" "${BDB_VERSION}.tar.gz" "${BDB_HASH}"
tar -xzvf ${BDB_VERSION}.tar.gz -C "$BDB_PREFIX"
cd "${BDB_PREFIX}/${BDB_VERSION}/"

# Apply a patch when building on OS X to make the build work with Xcode.
#
if [ "$(uname)" = "Darwin" ]; then
  BDB_OSX_ATOMIC_PATCH_URL='https://raw.githubusercontent.com/narkoleptik/os-x-berkeleydb-patch/0007e2846ae3fc9757849f5277018f4179ad17ef/atomic.patch'
  BDB_OSX_ATOMIC_PATCH_HASH='ba0e2b4f53e9cb0ec58f60a979b53b8567b4565f0384886196f1fc1ef111d151'

  http_get "${BDB_OSX_ATOMIC_PATCH_URL}" atomic.patch "${BDB_OSX_ATOMIC_PATCH_HASH}"
  patch -p1 < atomic.patch
fi

cd build_unix/

"${BDB_PREFIX}/${BDB_VERSION}/dist/configure" \
  --enable-cxx --disable-shared --with-pic --prefix="${BDB_PREFIX}" \
  "${BDB_EXTRA_CONFIGURE_FLAGS}"

make install

echo
echo "db4 build complete."
echo
echo 'When compiling bitcoind, run `./configure` in the following way:'
echo
echo "  export BDB_PREFIX='${BDB_PREFIX}'"
echo '  ./configure LDFLAGS="-L${BDB_PREFIX}/lib/" CPPFLAGS="-I${BDB_PREFIX}/include/" ...'
