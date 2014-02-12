#!/usr/bin/env bash

# Test block generation and basic wallet sending

if [ $# -lt 1 ]; then
        echo "Usage: $0 path_to_binaries"
        echo "e.g. $0 ../../src"
        exit 1
fi

BITCOIND=${1}/bitcoind
CLI=${1}/bitcoin-cli

DIR="${BASH_SOURCE%/*}"
SENDANDWAIT="${DIR}/send.sh"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi
. "$DIR/util.sh"

D=$(mktemp -d test.XXXXX)

# Two nodes; one will play the part of merchant, the
# other an evil transaction-mutating miner.

D1=${D}/node1
CreateDataDir $D1 port=11000 rpcport=11001
B1ARGS="-datadir=$D1 -debug"
$BITCOIND $B1ARGS &
B1PID=$!

D2=${D}/node2
CreateDataDir $D2 port=11010 rpcport=11011
B2ARGS="-datadir=$D2 -debug"
$BITCOIND $B2ARGS &
B2PID=$!

trap "kill -9 $B1PID $B2PID; rm -rf $D" EXIT

# Wait until all four nodes are at the same block number
function WaitBlocks {
    while :
    do
        sleep 1
        BLOCKS1=$( GetBlocks $B1ARGS )
        BLOCKS2=$( GetBlocks $B2ARGS )
        if (( $BLOCKS1 == $BLOCKS2 ))
        then
            break
        fi
    done
}

# Wait until node has $N peers
function WaitPeers {
    while :
    do
        PEERS=$( $CLI $1 getconnectioncount )
        if (( "$PEERS" == $2 ))
        then
            break
        fi
        sleep 1
    done
}

# Start with B2 connected to B1:
$CLI $B2ARGS addnode 127.0.0.1:11000 onetry
WaitPeers "$B1ARGS" 1

# 1 block, 50 XBT each == 50 XBT
$CLI $B1ARGS setgenerate true 1

WaitBlocks
# 100 blocks, 0 mature == 0 XBT
$CLI $B2ARGS setgenerate true 100
WaitBlocks

CheckBalance $B1ARGS 50
CheckBalance $B2ARGS 0

# restart B2 with no connection
$CLI $B2ARGS stop > /dev/null 2>&1
wait $B2PID
$BITCOIND $B2ARGS &
B2PID=$!

B2ADDRESS=$( $CLI $B2ARGS getnewaddress )

# Have B1 create two transactions; second will
# spend change from first, since B1 starts with only a single
# 50 bitcoin output:
TXID1=$( $CLI $B1ARGS sendtoaddress $B2ADDRESS 1.0 )
TXID2=$( $CLI $B1ARGS sendtoaddress $B2ADDRESS 2.0 )

# Mutate TXID1 and add it to B2's memory pool:
RAWTX1=$( $CLI $B1ARGS getrawtransaction $TXID1 )
RAWTX2=$( $CLI $B1ARGS getrawtransaction $TXID2 )
# ... mutate RAWTX1:
# RAWTX1 is hex-encoded, serialized transaction. So each
# byte is two characters; we'll prepend the first
# "push" in the scriptsig with OP_PUSHDATA1 (0x4c),
# and add one to the length of the signature.
# Fields are fixed; from the beginning:
# 4-byte version
# 1-byte varint number-of inputs (one in this case)
# 32-byte previous txid
# 4-byte previous output
# 1-byte varint length-of-scriptsig
# 1-byte PUSH this many bytes onto stack
#  ... etc
# So: to mutate, we want to get byte 41 (hex characters 82-83),
# increment it, and insert 0x4c after it.
L=${RAWTX1:82:2}
NEWLEN=$( printf "%x" $(( 16#$L + 1 )) )
MUTATEDTX1=${RAWTX1:0:82}${NEWLEN}4c${RAWTX1:84}
# ... give mutated tx1 to B2:
MUTATEDTXID=$( $CLI $B2ARGS sendrawtransaction $MUTATEDTX1 )

echo "TXID1: " $TXID1
echo "Mutated: " $MUTATEDTXID

# Re-connect nodes, and have B2 mine a block
$CLI $B2ARGS addnode 127.0.0.1:11000 onetry
WaitPeers "$B1ARGS" 1

$CLI $B2ARGS setgenerate true 1
WaitBlocks

$CLI $B2ARGS stop > /dev/null 2>&1
wait $B2PID
$CLI $B1ARGS stop > /dev/null 2>&1
wait $B1PID

trap "" EXIT

echo "Done, bitcoind's shut down. To rerun/poke around:"
echo "${1}/bitcoind -datadir=$D1 -daemon"
echo "${1}/bitcoind -datadir=$D2 -daemon -connect=127.0.0.1:11000"
echo "To cleanup:"
echo "killall bitcoind; rm -rf test.*"
exit 0

echo "Tests successful, cleaning up"
rm -rf $D
exit 0
