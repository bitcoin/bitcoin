#!/bin/bash
# create multiresolution windows icon
ICON_DST=../../src/qt/res/icons/42.ico

convert ../../src/qt/res/icons/42-16.png ../../src/qt/res/icons/42-32.png ../../src/qt/res/icons/42-48.png ${ICON_DST}
