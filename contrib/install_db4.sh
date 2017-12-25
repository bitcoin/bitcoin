#!/bin/sh

# This script downloads, patches, configures and builds Berkeley DB 4.8
# and installes it in a given directory. This enables Bitcoin Core
# to be built with --enable-wallet

set -e

DBVERS="db-4.8.30.NC"
DBFILE="$DBVERS.tar.gz"
DBPAGE="http://download.oracle.com/berkeley-db/$DBFILE"
DBHASH='12edc0df75bf9abd7f82f821795bcee50f42cb2e5f76a6a281b85732798364ef'

err() {
	echo $@ >&2
	exit 1
}

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

if [ -z "${1}" ]; then
  echo "usage: $0 <prefix> [args]"
  echo "<prefix> is a directory where DB will be installed"
  echo "Any further arguments will be passed to DB's ./configure"
  exit 1
fi

DBHOME="`readlink -f $1`"
mkdir -p $DBHOME || err Cannot create $DBHOME ; shift
http_get $DBPAGE $DBFILE $DBHASH || err $DBFILE hash incorrect
tar xzf $DBFILE -C $DBHOME || err "Cannot untar $DBFILE in $DBHOME"
cd $DBHOME/$DBVERS || err Cannot chdir to $DBHOME/$DBVERS

# Apply a patch necessary when building with clang and c++11 (see https://community.oracle.com/thread/3952592)
CLANG_CXX11_PATCH_URL='https://gist.githubusercontent.com/LnL7/5153b251fd525fe15de69b67e63a6075/raw/7778e9364679093a32dec2908656738e16b6bdcb/clang.patch'
CLANG_CXX11_PATCH_HASH='7a9a47b03fd5fb93a16ef42235fa9512db9b0829cfc3bdf90edd3ec1f44d637c'
http_get "${CLANG_CXX11_PATCH_URL}" clang.patch "${CLANG_CXX11_PATCH_HASH}"
patch -p2 < clang.patch || err Cannot apply clang patch

cd build_unix && ../dist/configure					\
  --prefix=${DBHOME} --with-pic --enable-cxx --enable-smallbuild	\
  --disable-shared --disable-java --disable-mingw --disable-tcl		\
  ${@}

test $? -eq 0 || err "Cannot configure in `pwd`"
make install  || err "Cannot make insatll failed in `pwd`"

echo
echo Build of $DBVERS finished
echo Add the following to ./configure when building bitcoin:
echo LDFLAGS=-L$DBHOME/lib/ CPPFLAGS=-I$DBHOME/include/
