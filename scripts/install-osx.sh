#!/bin/bash -ev

patch -p1 < contrib/homebrew/makefile.osx.patch

export CFLAGS=-I`pwd`/db4/include
pushd src
make -f makefile.osx
