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

CONTRIB=$TOP/contrib
BUILD_DIR=/tmp/bitcoin_osx_build

# First, compile bitcoin and bitcoind
cd "$TOP/src"
if [ ! -e bitcoin ]; then make -f makefile.osx bitcoin; fi
if [ ! -e bitcoind ]; then make -f makefile.osx bitcoind; fi
strip bitcoin bitcoind

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

rm -f Bitcoin.sparseimage
hdiutil convert "$CONTRIB/BitcoinTemplate.dmg" -format UDSP -o Bitcoin
hdiutil mount Bitcoin.sparseimage

# Copy over placeholders in /Volumes/Bitcoin
cp "$TOP/src/bitcoind" /Volumes/Bitcoin/
cp "$TOP/src/bitcoin" /Volumes/Bitcoin/Bitcoin.app/Contents/MacOS/

# Create source code .zip
cd "$TOP"
git archive -o /Volumes/Bitcoin/bitcoin.zip $(git branch 2>/dev/null|grep -e ^* | cut -d ' ' -f 2)

# Fix permissions
chmod -Rf go-w /Volumes/Bitcoin

cd "$BUILD_DIR"
hdiutil eject /Volumes/Bitcoin
rm -f "$CWD/Bitcoin.dmg"
hdiutil convert Bitcoin.sparseimage -format UDBZ -o "$CWD/Bitcoin.dmg"

cd "$CWD"
rm -rf "$BUILD_DIR"
