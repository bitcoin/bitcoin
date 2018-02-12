#!/bin/sh
#
# This script creates testnet and regtest pixmaps from the reference bitcoin
# pixmaps. It uses the same HSL shifting logic that is in
# src/qt/networkstyle.cpp, but has been modified to work with GraphicsMagick.

set -eux

colorshift() {
  local hue=$(("$1"*100/180+100))
  local sat=$((100-"$2"))
  local inp="$3"
  local outp="$4"
  convert -define module:colorspace=HSB "$inp" -modulate "100,${sat},${hue}" "$outp"
}

for f in bitcoin*.png bitcoin*.xpm; do
  colorshift 70 30 "$f" "testnet-$f"
done

for f in bitcoin*.png bitcoin*.xpm; do
  colorshift 160 30 "$f" "regtest-$f"
done
