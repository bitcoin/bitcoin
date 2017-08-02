#!/bin/bash
# create multiresolution windows icon
ICON_SRC=../../src/qt/res/icons/peercoin.png
ICON_DST=../../src/qt/res/icons/peercoin.ico
convert ${ICON_SRC} -resize 16x16 peercoin-16.png
convert ${ICON_SRC} -resize 32x32 peercoin-32.png
convert ${ICON_SRC} -resize 48x48 peercoin-48.png
convert peercoin-48.png peercoin-32.png peercoin-16.png ${ICON_DST}

