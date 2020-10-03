#!/bin/bash 

export XCODE_BASE=/Applications/Xcode.app/Contents
export SIMULATOR_BASEiOS=$XCODE_BASE/Developer/Platforms/iPhoneOS.platform
export FRAMEWORKSiOS=$SIMULATOR_BASEiOS/Developer/SDKs/iPhoneOS.sdk/System/Library/Frameworks/
export ARCHFLAGS="-arch arm64 "
export VERFLAGS="-mios-simulator-version-min=11.0"

export CC=`xcrun --sdk iphoneos --find clang`
export SYSROOT=`xcrun --sdk iphoneos --show-sdk-path`
cmake -DWITH="DV;BN;MD;FP;EP;FPX;EPX;PP;PC;CP" -DCHECK=off -DARITH=easy -DARCH=ARM -DCOLOR=off -DOPSYS=NONE -DFP_PRIME=254 -DFP_QNRES=on -DFP_METHD="INTEG;INTEG;INTEG;MONTY;EXGCD;SLIDE" -DFPX_METHD="INTEG;INTEG;LAZYR" -DPP_METHD="LAZYR;OATEP" -DCOMP="-O3 -funroll-loops -fomit-frame-pointer $INCLUDES -isysroot $SYSROOT $ARCHFLAGS -fembed-bitcode" -DTIMER=HREAL -DWORD=64 -DSTLIB=on -DSHLIB=off $1
