#!/bin/bash -ev

patch -p1 < contrib/homebrew/makefile.osx.patch

./contrib/install_db4.sh `pwd`
export CFLAGS=-I`pwd`/db4/include

pushd src
make -f makefile.osx -j$(sysctl -n hw.ncpu)
