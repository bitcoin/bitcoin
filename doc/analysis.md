# Analyzing Bitcoin Core

## Static analysis

### Analyzing Bitcoin Core using Clang Static Analyzer

#### Quickstart guide

```
$ git clone https://github.com/bitcoin/bitcoin
$ cd bitcoin/
$ ./autogen.sh
$ CC=clang CXX=clang++ ./configure --with-incompatible-bdb --disable-ccache
$ scan-build --use-cc=clang --use-c++=clang++ make
```

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

### Analyzing Bitcoin Core using `cppcheck`

#### Quickstart guide

```
$ git clone https://github.com/bitcoin/bitcoin
$ cd bitcoin/
$ git clone https://github.com/danmar/cppcheck
$ cd cppcheck/
$ cmake -DFILESDIR=$(pwd)/ .
$ make
$ cd ..
# Analyze source code files ...
$ cppcheck/bin/cppcheck --language=c++ -D__cplusplus -DCLIENT_VERSION_BUILD \
    -DCLIENT_VERSION_IS_RELEASE -DCLIENT_VERSION_MAJOR -DCLIENT_VERSION_MINOR \
    -DCLIENT_VERSION_REVISION -DCOPYRIGHT_YEAR -DDEBUG
    -DBOOST_FIXTURE_TEST_SUITE -I src/ -q \
    src/net_processing.cpp
```
