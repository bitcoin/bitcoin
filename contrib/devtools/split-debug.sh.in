#!/bin/sh
set -e
if [ $# -ne 3 ];
    then echo "usage: $0 <input> <stripped-binary> <debug-binary>"
fi

@OBJCOPY@ --enable-deterministic-archives -p --only-keep-debug $1 $3
@OBJCOPY@ --enable-deterministic-archives -p --strip-debug $1 $2
@STRIP@ --enable-deterministic-archives -p -s $2
@OBJCOPY@ --enable-deterministic-archives -p --add-gnu-debuglink=$3 $2
