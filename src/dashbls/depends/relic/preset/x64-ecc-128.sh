#!/bin/sh 
cmake -DCHECK=off -DARITH=x64-asm-4l -DFP_PRIME=255 -DFP_QNRES=off -DEC_METHD="EDDIE" -DFP_METHD="INTEG;INTEG;INTEG;MONTY;LOWER;SLIDE" -DCFLAGS="-O3 -funroll-loops -fomit-frame-pointer -march=native -mtune=native" $1
