#!/bin/bash -ev

export CFLAGS=-I`pwd`/db4/include
pushd src
make -f makefile.osx
