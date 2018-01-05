#!/bin/bash -ev


export PATH=/opt/mxe/usr/bin:$PATH


MXE_INCLUDE_PATH=/opt/mxe/usr/i686-w64-mingw32.static/include
MXE_LIB_PATH=/opt/mnt/mxe/usr/i686-w64-mingw32.static/lib

i686-w64-mingw32.static-qmake-qt5 \
        BOOST_LIB_SUFFIX=-mt \
        BOOST_THREAD_LIB_SUFFIX=_win32-mt \
        BOOST_INCLUDE_PATH=$MXE_INCLUDE_PATH/boost \
        BOOST_LIB_PATH=$MXE_LIB_PATH \
        OPENSSL_INCLUDE_PATH=$MXE_INCLUDE_PATH/openssl \
        OPENSSL_LIB_PATH=$MXE_LIB_PATH \
        BDB_INCLUDE_PATH=$MXE_INCLUDE_PATH \
        BDB_LIB_PATH=$MXE_LIB_PATH \
        MINIUPNPC_INCLUDE_PATH=$MXE_INCLUDE_PATH \
        MINIUPNPC_LIB_PATH=$MXE_LIB_PATH \
        QMAKE_LRELEASE=/opt/mxe/usr/i686-w64-mingw32.static/qt5/bin/lrelease bitcoin-qt.pro

#make -j$(nproc) -f Makefile.Release
make -j$(nproc) -f Makefile

