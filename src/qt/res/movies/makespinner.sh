# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
FRAMEDIR=$(dirname $0)
for i in {0..35}
do
    frame=$(printf "%03d" $i)
    angle=$(($i * 10))
    convert $FRAMEDIR/../src/spinner.png -background "rgba(0,0,0,0.0)" -distort SRT $angle $FRAMEDIR/spinner-$frame.png
done
