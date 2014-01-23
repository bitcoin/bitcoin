#!/bin/bash
# create multiresolution windows icon
ICON_SRC=../../src/qt/res/icons/ppcoin.png
ICON_DST=../../src/qt/res/icons/ppcoin.ico
convert ${ICON_SRC} -resize 16x16 ppcoin-16.png
convert ${ICON_SRC} -resize 32x32 ppcoin-32.png
convert ${ICON_SRC} -resize 48x48 ppcoin-48.png
convert ppcoin-48.png ppcoin-32.png ppcoin-16.png ${ICON_DST}

