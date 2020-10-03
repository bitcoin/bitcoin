#!/bin/bash 

# First run /opt/android-ndk/build/tools/make-standalone-toolchain.sh --platform=android-18 --install-dir=/tmp/android-test/

ROOT=/tmp/android-test
export NDK=/opt/android-ndk
SYSROOT=$ROOT/sysroot

PREF=arm-linux-androideabi-

export CC="$ROOT/bin/${PREF}gcc --sysroot=$SYSROOT"

cmake -DWITH="DV;BN;MD;FP;EP;FPX;EPX;PP;PC;CP" -DCHECK=off -DARITH=arm-asm-254 -DARCH=ARM -DCOLOR=off -DOPSYS=DROID -DFP_PRIME=254 -DFP_QNRES=on -DFP_METHD="INTEG;INTEG;INTEG;MONTY;EXGCD;SLIDE" -DFPX_METHD="INTEG;INTEG;LAZYR" -DPP_METHD="LAZYR;OATEP" -DCOMP="-O3 -funroll-loops -fomit-frame-pointer" -DLINK="-L$SYSROOT/usr/lib/gcc/arm-linux-androideabi/4.9.x/ -L$SYSROOT/usr/lib -llog" -DTIMER=HREAL -DWORD=32 -DSTLIB=on -DSHLIB=off $1
