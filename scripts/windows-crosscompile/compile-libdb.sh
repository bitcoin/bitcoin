#!/bin/bash -ev

MXE_PATH=/usr/lib/mxe
sed -i "s/WinIoCtl.h/winioctl.h/g" dbinc/win_db.h
mkdir build_mxe
cd build_mxe

CC=$MXE_PATH/usr/bin/i686-w64-mingw32.static-gcc \
CXX=$MXE_PATH/usr/bin/i686-w64-mingw32.static-g++ \
AR=$MXE_PATH/usr/bin/i686-w64-mingw32.static-ar \
RANLIB=$MXE_PATH/usr/bin/i686-w64-mingw32.static-ranlib \
CFLAGS="-DSTATICLIB -I$MXE_PATH/usr/i686-w64-mingw32.static/include" \
../dist/configure \
        --disable-replication \
        --enable-mingw \
        --enable-cxx \
        --host x86 \
        --prefix=$MXE_PATH/usr/i686-w64-mingw32.static

make -j$(nproc)
sudo make install

