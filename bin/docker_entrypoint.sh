#!/usr/bin/env bash

set -e

#
# Docker entrypoint 
#   (see: https://docs.docker.com/engine/reference/builder/#entrypoint)
#
# This script is for use with the Docker container built by `../Dockerfile`
# and run with `docker-compose` (see `docker-compose.yml`).
#
# Recognized arguments:
#
#   - `test`: Run all tests. 
#      E.g. `docker-compose run --rm bitcoind test`.
#
#   - `test-cpp`: Run all C++-based tests. 
#      E.g. `docker-compose run --rm bitcoind test-cpp`.
#
#   - `test-python`: Run all Python-based tests. 
#      E.g. `docker-compose run --rm bitcoind test-python --coverage`.
#
#   - `[no args]`: Start bitcoind.
#      E.g. `docker-compose up bitcoind`.
#
# Any arguments that aren't the above are executed in the container image as
# normal.

[ -z "${MAKE_JOBS}" ] && MAKE_JOBS=$(nproc)
[ -z "${CC}" ] && export CC=/usr/bin/clang
if [ -z "${CXX}" ]; then
  export CXX=/usr/bin/clang++
  export CXXFLAGS="-std=c++11 ${CXXFLAGS}"
fi
 
run_make() {
  make -j $MAKE_JOBS "$@"
}
 
# If we haven't run libtool, do so.
[ ! -f "configure" ] && ./autogen.sh

# Configure Bitcoin Core to use our own-built instance of BDB.
[ ! -f "config.status" ] && ./configure \
  LDFLAGS="-L${BDB_PREFIX}/lib/" \
  CPPFLAGS="-I${BDB_PREFIX}/include/" \
  CXXFLAGS="${CXXFLAGS}"
[ ! -f "src/bitcoind" ] && run_make

test_cpp() { 
  run_make check "$@" 
}

test_python() { 
  ./test/functional/test_runner.py "$@" 
}

case "${1}" in
  "test")
    shift;
    run_make
    test_cpp
    test_python --coverage
    ;;

  "test-cpp")
    shift;
    run_make
    test_cpp $EXTRA_ARGS
    ;;

  "test-python")
    shift;
    run_make
    [ -z "${EXTRA_ARGS}" ] && EXTRA_ARGS='--coverage'
    test_python $EXTRA_ARGS
    ;;

  *)
    exec "$@"
    ;;
esac
