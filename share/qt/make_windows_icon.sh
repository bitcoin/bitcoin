#!/bin/bash
# create multiresolution windows icon
ICON_SRC=../../src/qt/res/icons/crowncoin.png
ICON_DST=../../src/qt/res/icons/crowncoin.ico
convert ${ICON_SRC} -resize 16x16 crowncoin-16.png
convert ${ICON_SRC} -resize 32x32 crowncoin-32.png
convert ${ICON_SRC} -resize 48x48 crowncoin-48.png
convert crowncoin-16.png crowncoin-32.png crowncoin-48.png ${ICON_DST}

