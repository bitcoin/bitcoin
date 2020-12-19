# Go bindings for Chia BLS signatures library

This is a CGo wrapper around the Chia BLS Signatures implementation in C++.

## Pre-Requisites

You will need to install a modern version of Go (1.12+), which is outside the
scope of this README.

## Install

It is necessary to follow the instructions one level up for building the Chia
library, with a small difference: The repo needs to be cloned and built in
`$GOPATH`.

```sh
# Clone BLS Signatures into $GOPATH and build the C++ library
mkdir -pv $GOPATH/src/github.com/dashpay/
git clone https://github.com/dashpay/bls-signatures.git $GOPATH/src/github.com/dashpay/bls-signatures
cd $GOPATH/src/github.com/dashpay/bls-signatures
git submodule update --init --recursive
mkdir build
cd build
cmake ../
cmake --build . -- -j 6
```

## Testing

A Makefile is included for simplicity. By default, it runs conventional Go
formatting / linter tools and tests.

```sh
cd bls-signatures/go-bindings
make
```

You may need to install the `golint` and `goimports` tools if not already
installed:

```sh
go get golang.org/x/tools/cmd/goimports
go get -u golang.org/x/lint/golint
```
