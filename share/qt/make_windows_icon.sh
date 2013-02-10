#!/bin/bash
# create multiresolution windows icon

#ICON_SRC=../../src/qt/res/icons/novacoin.png

ICON_DST=../../src/qt/res/icons/novacoin.ico

#convert ${ICON_SRC} -resize 16x16 novacoin-16.png
#convert ${ICON_SRC} -resize 32x32 novacoin-32.png
#convert ${ICON_SRC} -resize 48x48 novacoin-48.png

convert ../../src/qt/res/icons/novacoin-16.png ../../src/qt/res/icons/novacoin-32.png ../../src/qt/res/icons/novacoin-48.png ${ICON_DST}

