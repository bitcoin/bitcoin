# Analyzing Bitcoin Core

## Static analysis

### Analyzing Bitcoin Core using `clang-tidy`

#### Quickstart guide

```
$ git clone https://github.com/bitcoin/bitcoin
$ cd bitcoin/
$ ./autogen.sh
$ CC=clang CXX=clang++ ./configure --with-incompatible-bdb --disable-ccache
$ git clone https://github.com/rizsotto/bear
$ cd bear/
$ cmake .
$ make all
$ chmod +x bear/bear
$ cd ..
$ bear/bear/bear -l $(pwd)/bear/libear/libear.so make
# Analyze source code files ...
$ clang-tidy src/test/crypto_tests.cpp
```
