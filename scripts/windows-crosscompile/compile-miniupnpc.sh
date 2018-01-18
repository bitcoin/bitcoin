#!/bin/bash -ev

MXE_PATH=/usr/lib/mxe

CC=$MXE_PATH/usr/bin/i686-w64-mingw32.static-gcc \
AR=$MXE_PATH/usr/bin/i686-w64-mingw32.static-ar \
CFLAGS="-DSTATICLIB -I$MXE_PATH/usr/i686-w64-mingw32.static/include" \
LDFLAGS="-L$MXE_PATH/usr/i686-w64-mingw32.static/lib" \
make libminiupnpc.a

sudo mkdir $MXE_PATH/usr/i686-w64-mingw32.static/include/miniupnpc
sudo cp *.h $MXE_PATH/usr/i686-w64-mingw32.static/include/miniupnpc
sudo cp libminiupnpc.a $MXE_PATH/usr/i686-w64-mingw32.static/lib


