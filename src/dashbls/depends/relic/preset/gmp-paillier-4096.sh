#!/bin/sh 
cmake -DCHECK=off -DARITH=gmp -DBN_PRECI=4096 -DALLOC=DYNAMIC -DCFLAGS="-O3 -march=native -mtune=native -fomit-frame-pointer" -DWITH="DV;BN;MD;CP" -DSHLIB=off $1
