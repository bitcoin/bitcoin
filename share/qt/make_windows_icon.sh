#!/bin/bash
# create multiresolution windows icon
ICON_DST=../../src/qt/res/icons/olcoin.ico

convert ../../src/qt/res/icons/olcoin-16.png ../../src/qt/res/icons/olcoin-32.png ../../src/qt/res/icons/novacoin.png ${ICON_DST}
