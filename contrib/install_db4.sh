#!/bin/sh

# Install libdb4.8 (Berkeley DB).

export LC_ALL=C
set -e

if [ -z "${1}" ]; then
  echo "Usage: ./install_db4.sh <base-dir> [<extra-bdb-configure-flag> ...]"
  echo
  echo "Must specify a single argument: the directory in which db4 will be built."
  echo "This is probably \`pwd\` if you're at the root of the bitcoin repository."
  exit 1
fi

expand_path() {
  echo "$(cd "${1}" && pwd -P)"
}

BDB_PREFIX="$(expand_path ${1})/db4"; shift;
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
    if [ "$(uname)" = "FreeBSD" ]; then
      sha256 -c "${1}" "${2}"
    else
      echo "${1}  ${2}" | sha256 -c
    fi
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

# Apply a patch necessary when building with clang and c++11 (see https://community.oracle.com/thread/3952592)
CLANG_CXX11_PATCH_URL='https://gist.githubusercontent.com/LnL7/5153b251fd525fe15de69b67e63a6075/raw/7778e9364679093a32dec2908656738e16b6bdcb/clang.patch'
CLANG_CXX11_PATCH_HASH='7a9a47b03fd5fb93a16ef42235fa9512db9b0829cfc3bdf90edd3ec1f44d637c'
http_get "${CLANG_CXX11_PATCH_URL}" clang.patch "${CLANG_CXX11_PATCH_HASH}"
patch -p2 < clang.patch

cd build_unix/

"${BDB_PREFIX}/${BDB_VERSION}/dist/configure" \
  --enable-cxx --disable-shared --disable-replication --with-pic --prefix="${BDB_PREFIX}" \
  "${@}"

make install

echo
echo "db4 build complete."
echo
echo 'When compiling bitcoind, run `./configure` in the following way:'
echo
echo "  export BDB_PREFIX='${BDB_PREFIX}'"
echo '  ./configure BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include" ...'
