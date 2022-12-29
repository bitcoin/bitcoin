#!/bin/sh 
cmake -DCHECK=off -DARITH=gmp -DFP_PRIME=255 -DFP_QNRES=off -DEC_METHD="EDDIE" -DFP_METHD="INTEG;COMBA;COMBA;MONTY;MONTY;SLIDE" -DCFLAGS="-O3 -funroll-loops -fomit-frame-pointer -march=native -mtune=native" $1
