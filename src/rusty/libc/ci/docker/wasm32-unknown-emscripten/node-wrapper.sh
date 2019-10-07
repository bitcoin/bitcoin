#!/bin/sh

set -e

me=$1
shift
dir=$(dirname $me)
file=$(basename $me)

if echo $file | grep -q wasm; then
  exit 0 # FIXME(rust-lang/cargo#4750)
fi

cd $dir
exec node $file "$@"
