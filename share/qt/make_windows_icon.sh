#!/bin/bash
# create multiresolution windows icon
ICON_SRC=../../src/qt/res/icons/primecoin.png
ICON_DST=../../src/qt/res/icons/primecoin.ico
convert ${ICON_SRC} -resize 16x16 primecoin-16.png
convert ${ICON_SRC} -resize 32x32 primecoin-32.png
convert ${ICON_SRC} -resize 48x48 primecoin-48.png
convert ${ICON_SRC} -resize 64x64 primecoin-64.png
convert primecoin-32.png ${ICON_SRC} primecoin-64.png primecoin-48.png primecoin-32.png primecoin-16.png ${ICON_DST}

