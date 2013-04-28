#!/bin/bash
# create multiresolution windows icon
ICON_DST=../../src/qt/res/icons/yacoin.ico

convert ../../src/qt/res/icons/yacoin-16.png ../../src/qt/res/icons/yacoin-32.png ../../src/qt/res/icons/yacoin-48.png ${ICON_DST}
