#!/bin/bash

output_file=$1
cur_dir=$(pwd)

cd ../../
echo "HEAD: $(git rev-parse --short HEAD)" > "$cur_dir/$output_file.log"
make clean
./autogen.sh
./configure --enable-experimental --enable-module-batch --enable-module-schnorrsig >> "$cur_dir/$output_file.log"
make
./bench schnorrsig > "$cur_dir/$output_file"
./bench extrakeys >> "$cur_dir/$output_file"