#!/bin/bash 
cmake -DCHECK=off -DARITH=gmp -DFP_PRIME=256 -DFP_QNRES=off -DEC_METHD="PRIME" -DFP_METHD="INTEG;COMBA;COMBA;MONTY;MONTY;SLIDE" -DCOMP="-O2 -funroll-loops -fomit-frame-pointer" $1
