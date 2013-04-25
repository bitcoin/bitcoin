#!/bin/bash
# create multiresolution windows icon
ICON_DST=../../src/qt/res/icons/bitbar.ico

convert ../../src/qt/res/icons/bitbar-16.png ../../src/qt/res/icons/bitbar-32.png ../../src/qt/res/icons/bitbar-48.png ${ICON_DST}
