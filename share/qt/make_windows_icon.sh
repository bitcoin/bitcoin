#!/bin/bash
# create multiresolution windows icon
<<<<<<< HEAD
#mainnet
ICON_SRC=../../src/qt/res/icons/bitcoin.png
ICON_DST=../../src/qt/res/icons/bitcoin.ico
convert ${ICON_SRC} -resize 16x16 bitcoin-16.png
convert ${ICON_SRC} -resize 32x32 bitcoin-32.png
convert ${ICON_SRC} -resize 48x48 bitcoin-48.png
convert bitcoin-16.png bitcoin-32.png bitcoin-48.png ${ICON_DST}
#testnet
ICON_SRC=../../src/qt/res/icons/bitcoin_testnet.png
ICON_DST=../../src/qt/res/icons/bitcoin_testnet.ico
convert ${ICON_SRC} -resize 16x16 bitcoin-16.png
convert ${ICON_SRC} -resize 32x32 bitcoin-32.png
convert ${ICON_SRC} -resize 48x48 bitcoin-48.png
convert bitcoin-16.png bitcoin-32.png bitcoin-48.png ${ICON_DST}
rm bitcoin-16.png bitcoin-32.png bitcoin-48.png
=======
ICON_SRC=../../src/qt/res/icons/crowncoin.png
ICON_DST=../../src/qt/res/icons/crowncoin.ico
convert ${ICON_SRC} -resize 16x16 crowncoin-16.png
convert ${ICON_SRC} -resize 32x32 crowncoin-32.png
convert ${ICON_SRC} -resize 48x48 crowncoin-48.png
convert crowncoin-16.png crowncoin-32.png crowncoin-48.png ${ICON_DST}

>>>>>>> origin/dirty-merge-dash-0.11.0
