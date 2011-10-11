#!/usr/bin/env bash
# 
# Creates a Bitcoin.dmg OSX file from the contrib/BitcoinTemplate.dmg file
#
# Recipe from: http://digital-sushi.org/entry/how-to-create-a-disk-image-installer-for-apple-mac-os-x/
#
# To make a prettier BitcoinTemplate.dmg:
#  + open (mount) BitcoinTemplate.dmg
#  + change the file properties, icon positions, background image, etc
#  + eject, then commit the changed BitcoinTemplate.dmg
#

CWD=$(pwd)

if [ $# -lt 1 ]; then
    if [ $(basename $CWD) == "contrib" ]
    then
        TOP=$(dirname $CWD)
    else
        echo "Usage: $0 /path/to/bitcoin/tree"
        exit 1
    fi
else
    TOP=$1
fi

# Create Bitcoin-Qt.app
cd "$TOP"
if [ ! -e Makefile ]; then qmake bitcoin-qt.pro; fi
make
macdeployqt Bitcoin-Qt.app
# Workaround a bug in macdeployqt: https://bugreports.qt.nokia.com/browse/QTBUG-21913
# (when fixed, this won't be necessary)
cp /opt/local/lib/db48/libdb_cxx-4.8.dylib Bitcoin-Qt.app/Contents/Frameworks/
install_name_tool -id @executable_path/../Frameworks/libdb_cxx-4.8.dylib \
    Bitcoin-Qt.app/Contents/Frameworks/libdb_cxx-4.8.dylib
install_name_tool -change libqt.3.dylib \
        @executable_path/../Frameworks/libqt.3.dylib \
        Bitcoin-Qt.app/Contents/MacOS/Bitcoin-Qt

# Create a .dmg
macdeployqt Bitcoin-Qt.app -dmg

# Compile bitcoind
cd "$TOP/src"
STATIC=1 make -f makefile.osx

