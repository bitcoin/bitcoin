#!/bin/bash
cmake -DWORD=64 \
      -DRAND=UDEV \
      -DSEED=DEV \
      -DSHLIB=OFF \
      -DSTBIN=OFF \
      -DTIMER=CYCLE \
      -DCHECK=OFF \
      -DVERBS=OFF \
      -DALLOC=AUTO \
      -DARITH=gmp \
      -DFP_PRIME=381 \
      -DFP_METHD="INTEG;INTEG;INTEG;MONTY;LOWER;SLIDE" \
      -DCOMP="-O3 -funroll-loops -fomit-frame-pointer -finline-small-functions -march=native -mtune=native" \
      -DFP_PMERS=OFF \
      -DFP_QNRES=OFF \
      -DFPX_METHD="INTEG;INTEG;LAZYR" \
      -DEP_PLAIN=OFF \
      -DEP_SUPER=OFF \
      -DPP_METHD="LAZYR;OATEP" \
      $1
