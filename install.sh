#!/bin/bash
set -e
trap "exit" INT

if [ -f "Makefile" ]; then
    echo "Making clean..."
    make clean > /dev/null 2>&1
fi

echo "Installing Berkeley DB 4.8..."

cd src

if [ ! -d "bdb" ]; then
    if [ ! -f "db-4.8.30.NC.tar.gz" ]; then
        wget http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz
        # echo '12edc0df75bf9abd7f82f821795bcee50f42cb2e5f76a6a281b85732798364ef  db-4.8.30.NC.tar.gz' | sha256sum -c
    fi
    mkdir -p bdb

    tar -xvf db-4.8.30.NC.tar.gz -C bdb --strip-components=1 > /dev/null 2>&1

    cd bdb/build_unix
    mkdir -p build

    export BDB_PREFIX=$(pwd)/build

    ../dist/configure --disable-shared --enable-cxx --with-pic --prefix=$BDB_PREFIX
    make install

    cd ../..
    echo "Done."
else
    export BDB_PREFIX=$(pwd)/bdb/build_unix/build

    echo "Local Berkeley DB installation found."
fi

cd ..

echo "Checking Berkeley DB version..."

g++ bdb_version_check.cpp -I${BDB_PREFIX}/include/ -L${BDB_PREFIX}/lib/ -o bdb_version_check
./bdb_version_check

echo "Installing Bitcoin"

srcdir="$(dirname $0)"
cd "$srcdir"
autoreconf --install --force --prepend-include=${BDB_PREFIX}/include/

./configure CPPFLAGS="-I${BDB_PREFIX}/include/ -O2" LDFLAGS="-L${BDB_PREFIX}/lib/" USE_UPNP=

make
strip src/bitcoind

echo "Done."
