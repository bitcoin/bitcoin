#!/usr/bin/env bash
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

#
# Generate a block
#

if [ $# -lt 1 ]; then
    >&2 echo "syntax: $0 <bitcoin-cli path> [<bitcoin-cli args>]" ; exit 1
fi

bcli=$1
shift

if ! [ -e "$bcli" ]; then
    command -v "$bcli" >/dev/null 2>&1 || { echo >&2 "error: unable to find bitcoin binary: $bcli"; exit 1; }
fi

# get address for coinbase output
addr=$($bcli "$@" getnewaddress)
# start looping; we re-create the block every time we fail to grind as that resets the nonce and gives us an updated
# version of the block
while true; do
    # create an unsigned, un-PoW'd block
    $bcli "$@" getnewblockhex $addr > $PWD/unsigned
    # sign it
    $bcli "$@" signblock $PWD/unsigned > $PWD/signed
    # grind proof of work; this ends up broadcasting the block, if successful (akin to "generatetoaddress")
    blockhash=$($bcli "$@" grindblock $PWD/signed 10000000)
    if [ "$blockhash" != "false" ]; then break; fi
done

echo $blockhash
