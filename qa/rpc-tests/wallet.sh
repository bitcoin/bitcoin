#!/usr/bin/env bash

# Test block generation and basic wallet sending

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
$BITCOIND $B1ARGS &
B1PID=$!

D2=${D}/node2
CreateDataDir "$D2" port=11010 rpcport=11011 connect=127.0.0.1:11000
B2ARGS="-datadir=$D2"
$BITCOIND $B2ARGS &
B2PID=$!

D3=${D}/node3
CreateDataDir "$D3" port=11020 rpcport=11021 connect=127.0.0.1:11000
B3ARGS="-datadir=$D3"
$BITCOIND $BITCOINDARGS $B3ARGS &
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

echo "Generating test blockchain..."

# 1 block, 50 XBT each == 50 XBT
$CLI $B1ARGS setgenerate true 1
WaitBlocks
# 101 blocks, 1 mature == 50 XBT
$CLI $B2ARGS setgenerate true 101
WaitBlocks

CheckBalance "$B1ARGS" 50
CheckBalance "$B2ARGS" 50

# Send 21 XBT from 1 to 3. Second
# transaction will be child of first, and
# will require a fee
Send $B1ARGS $B3ARGS 11
Send $B1ARGS $B3ARGS 10

# Have B1 mine a new block, and mature it
# to recover transaction fees
$CLI $B1ARGS setgenerate true 1
WaitBlocks

# Have B2 mine 100 blocks so B1's block is mature:
$CLI $B2ARGS setgenerate true 100
WaitBlocks

# B1 should end up with 100 XBT in block rewards plus fees,
# minus the 21 XBT sent to B3:
CheckBalance "$B1ARGS" "100-21"
CheckBalance "$B3ARGS" "21"

# B1 should have two unspent outputs; create a couple
# of raw transactions to send them to B3, submit them through
# B2, and make sure both B1 and B3 pick them up properly:
RAW1=$(CreateTxn1 $B1ARGS 1 $(Address $B3ARGS "from1" ) )
RAW2=$(CreateTxn1 $B1ARGS 2 $(Address $B3ARGS "from1" ) )
RAWTXID1=$(SendRawTxn "$B2ARGS" $RAW1)
RAWTXID2=$(SendRawTxn "$B2ARGS" $RAW2)

# Have B2 mine a block to confirm transactions:
$CLI $B2ARGS setgenerate true 1
WaitBlocks

# Check balances after confirmation
CheckBalance "$B1ARGS" 0
CheckBalance "$B3ARGS" 100
CheckBalance "$B3ARGS" "100-21" "from1"

$CLI $B3ARGS stop > /dev/null 2>&1
wait $B3PID
$CLI $B2ARGS stop > /dev/null 2>&1
wait $B2PID
$CLI $B1ARGS stop > /dev/null 2>&1
wait $B1PID

echo "Tests successful, cleaning up"
rm -rf $D
exit 0
