#!/bin/bash

test -d build || mkdir -p build
cd build

cmake .. \
      -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native" \
      -DCHECK_BENCHMARKS=1 \
      -DBENCHMARK_PARAM="N:1000" -DBENCHMARK_SAMPLES="100"
make benchmarks -j1
ctest -R "benchmark/vector-paper*"

cmake ..\
      -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native" \
      -DCHECK_BENCHMARKS=1 \
      -DBENCHMARK_PARAM="N:100000" -DBENCHMARK_SAMPLES="20"
make benchmarks -j1
ctest -R "benchmark/vector-paper*"

cmake .. \
      -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native" \
      -DCHECK_BENCHMARKS=1 \
      -DBENCHMARK_PARAM="N:10000000" -DBENCHMARK_SAMPLES="3"
make benchmarks -j1
ctest -R "benchmark/vector-paper*"
