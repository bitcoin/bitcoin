#!/usr/bin/env bash
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

#
# Take the hex formatted line separated transactions in reorg-list.txt where every even transaction is put into the even set, and every odd transaction is put into the odd set.
# That is, transactions on line 1, 3, 5, 7, ... go into the odd set, and transactions on line 2, 4, 6, 8, ... go into the even set.
#
# - Generate two blocks A and B, where A contains all or some of the odd set, and B contains the corresponding even set.
# - Sign, grind, and broadcast block A
# - Wait a random amount of time (1-10 mins)
# - Invalidate block A
# - Sign, grind, and broadcast block B
#

bcli=$1
shift

function log()
{
    echo "- $(date +%H:%M:%S): $*"
}

if [ ! -e "reorg-list.txt" ]; then
    echo "reorg-list.txt not found"
    exit 1
fi

# get address for coinbase output
addr=$($bcli "$@" getnewaddress)

# create blocks A and B
$bcli "$@" getnewblockhex $addr > $PWD/block-a
cp block-a block-b

odd=1
while read -r line; do
    if [ "$line" = "" ]; then continue; fi
    echo $line > tx
    if [ $odd -eq 1 ]; then blk="block-a"; else blk="block-b"; fi
    ./addtxtoblock.py $blk tx 100 > t # note: we are throwing away all fees above 100 satoshis for now; should figure out a way to determine fees
    mv t $blk
    (( odd=1-odd ))
done < reorg-list.txt

rm reorg-list.txt

log "mining block A (to-orphan block)"
while true; do
    $bcli "$@" signblock $PWD/block-a > signed-a
    blockhash_a=$($bcli "$@" grindblock $PWD/signed-a 1000000000)
    if [ "$blockhash_a" != "false" ]; then break; fi
done
log "mined block with hash $blockhash_a"
(( waittime=RANDOM%570 ))
(( waittime=30+waittime ))
log "waiting for $waittime s"
sleep $waittime
log "invalidating $blockhash_a"
$bcli "$@" invalidateblock $blockhash_a

log "mining block B (replace block)"
while true; do
    $bcli "$@" signblock $PWD/block-b > signed-b
    blockhash_b=$($bcli "$@" grindblock $PWD/signed-b 1000000000)
    if [ "$blockhash_b" != "false" ]; then break; fi
done

echo "mined $blockhash_b"
echo "cleaning up"
rm block-b signed-b block-a signed-a
