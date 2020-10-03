#!/bin/bash
cmake -DWORD=64 \
      -DRAND=UDEV \
      -DSEED=DEV \
      -DSHLIB=OFF \
      -DSTBIN=OFF \
      -DTIMER=CYCLE \
      -DCHECK=off \
      -DVERBS=off \
      -DALLOC=AUTO \
      -DARITH=easy \
      -DFP_PRIME=381 \
      -DFP_METHD="INTEG;INTEG;INTEG;MONTY;LOWER;SLIDE" \
      -DCOMP="-O3 -funroll-loops -fomit-frame-pointer -finline-small-functions -march=native -mtune=native" \
      -DFP_PMERS=off \
      -DFP_QNRES=on \
      -DFPX_METHD="INTEG;INTEG;LAZYR" \
      -DEP_PLAIN=off \
      -DEP_SUPER=off \
      -DPP_METHD="LAZYR;OATEP" \
      $1
