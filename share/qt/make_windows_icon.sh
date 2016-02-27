#!/bin/bash
# create multiresolution windows icon
ICON_SRC=../../src/qt/res/icons/bitcoin.png
ICON_DST=../../src/qt/res/icons/bitcoin.ico
convert ${ICON_SRC} -resize 16x16 bitcoin-16.png
convert ${ICON_SRC} -resize 32x32 bitcoin-32.png
convert ${ICON_SRC} -resize 48x48 bitcoin-48.png
convert ${ICON_SRC} -resize 64x64 bitcoin-64.png
convert ${ICON_SRC} -resize 96x96 bitcoin-96.png
convert ${ICON_SRC} -resize 128x128 bitcoin-128.png
convert bitcoin-16.png bitcoin-32.png bitcoin-48.png bitcoin-64.png bitcoin-96.png bitcoin-128.png ${ICON_DST}

