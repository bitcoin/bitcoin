#!/usr/bin/env bash
# Copyright (c) 2013-2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Test -txoutsbyaddressindex

if [ $# -lt 1 ]; then
        echo "Usage: $0 path_to_binaries"
        echo "e.g. $0 ../../src"
        exit 1
fi

set -f

BITCOIND=${1}/bitcoind
CLI=${1}/bitcoin-cli

DIR="${BASH_SOURCE%/*}"
SENDANDWAIT="${DIR}/send.sh"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi
. "$DIR/util.sh"

D=$(mktemp -d test.XXXXX)

D1=${D}/node1
CreateDataDir "$D1" port=11000 rpcport=11001
B1ARGS="-datadir=$D1"
$BITCOIND $B1ARGS -txoutsbyaddressindex -sendfreetransactions=1 &
B1PID=$!

D2=${D}/node2
CreateDataDir "$D2" port=11010 rpcport=11011
B2ARGS="-datadir=$D2"
$BITCOIND $B2ARGS -sendfreetransactions=1 &
B2PID=$!

D3=${D}/node3
CreateDataDir "$D3" port=11020 rpcport=11021
B3ARGS="-datadir=$D3"
$BITCOIND $B3ARGS -sendfreetransactions=1 &
B3PID=$!

# Wait until all three nodes are at the same block number
function WaitBlocks {
    while :
    do
        sleep 1
        declare -i BLOCKS1=$( GetBlocks $B1ARGS )
        declare -i BLOCKS2=$( GetBlocks $B2ARGS )
        declare -i BLOCKS3=$( GetBlocks $B3ARGS )
        if (( BLOCKS1 == BLOCKS2 && BLOCKS2 == BLOCKS3 ))
        then
            break
        fi
    done
}

function WaitBlocks2 {
    while :
    do
        sleep 1
        declare -i BLOCKS1=$( GetBlocks $B1ARGS )
        declare -i BLOCKS2=$( GetBlocks $B2ARGS )
        if (( BLOCKS1 == BLOCKS2 ))
        then
            break
        fi
    done
}

function CleanUp {
$CLI $B3ARGS stop > /dev/null 2>&1
wait $B3PID
$CLI $B2ARGS stop > /dev/null 2>&1
wait $B2PID
$CLI $B1ARGS stop > /dev/null 2>&1
wait $B1PID

rm -rf $D
}

function ErrorAndExit {
echo "$@" 1>&2;
CleanUp
exit 1
}

echo "Generating test blockchain..."

# mining
$CLI $B2ARGS addnode "127.0.0.1:11000" "onetry"
$CLI $B3ARGS addnode "127.0.0.1:11000" "onetry"
$CLI $B1ARGS generate 101
WaitBlocks
CheckBalance "$B1ARGS" 50

# TX1: send from node1 to node2
# - check if txout from tx1 is there
address=$($CLI $B2ARGS getnewaddress)
txid1=$($CLI $B1ARGS sendtoaddress $address 10)
$CLI $B1ARGS generate 1
WaitBlocks
CheckBalance "$B1ARGS" 90
CheckBalance "$B2ARGS" 10
txouts=$($CLI $B1ARGS gettxoutsbyaddress 1 "[\"""$address""\"]")
txid=$(ExtractKey "txid" "$txouts")
if [ -z "$txid" ] || [ $txid != $txid1 ] ; then
   ErrorAndExit "wrong txid1: $txid != $txid1"
fi

# stop node 3
$CLI $B3ARGS stop > /dev/null 2>&1
wait $B3PID

# TX2: send from node2 to node1
# - check if txout from tx1 is gone
# - check if txout from tx2 is there
address2=$($CLI $B1ARGS getnewaddress)
txid2=$($CLI $B2ARGS sendtoaddress $address2 5)
$CLI $B2ARGS generate 1
WaitBlocks2
CheckBalance "$B1ARGS" 145
txouts=$($CLI $B1ARGS gettxoutsbyaddress 1 "[\"""$address""\"]")
txid=$(ExtractKey "txid" "$txouts")
if [ ! -z "$txid" ] ; then
   ErrorAndExit "txid not empty: $txid"
fi
txouts=$($CLI $B1ARGS gettxoutsbyaddress 1 "[\"""$address2""\"]")
txid=$(ExtractKey "txid" "$txouts")
if [ -z "$txid" ] || [ $txid != $txid2 ] ; then
   ErrorAndExit "wrong txid2: $txid != $txid2"
fi

# start node 3
$BITCOIND $B3ARGS &
B3PID=$!

# mine 10 blocks alone to have the longest chain
$CLI $B3ARGS generate 10
$CLI $B1ARGS addnode "127.0.0.1:11020" "onetry"
$CLI $B2ARGS addnode "127.0.0.1:11020" "onetry"
$CLI $B3ARGS generate 1
WaitBlocks

# TX2 must be reverted
# - check if txout from tx1 is there again
# - check if txout from tx2 is gone
CheckBalance "$B1ARGS" 640
txouts=$($CLI $B1ARGS gettxoutsbyaddress 1 "[\"""$address""\"]")
txid=$(ExtractKey "txid" "$txouts")
if [ -z "$txid" ] || [ $txid != $txid1 ] ; then
   ErrorAndExit "wrong txid1 : $txid != $txid1"
fi
txouts=$($CLI $B1ARGS gettxoutsbyaddress 1 "[\"""$address2""\"]")
txid=$(ExtractKey "txid" "$txouts")
if [ ! -z "$txid" ] ; then
   ErrorAndExit "txid is not empty: $txid"
fi

echo "Tests successful, cleaning up"
CleanUp
exit 0
