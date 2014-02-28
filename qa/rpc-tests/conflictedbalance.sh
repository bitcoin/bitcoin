#!/usr/bin/env bash

# Test marking of spent outputs

# Create a transaction graph with four transactions,
# A/B/C/D
# C spends A
# D spends B and C

# Then simulate C being mutated, to create C'
#  that is mined.
# A is still (correctly) considered spent.
# B should be treated as unspent

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

# Two nodes; one will play the part of merchant, the
# other an evil transaction-mutating miner.

D1=${D}/node1
CreateDataDir $D1 port=11000 rpcport=11001
B1ARGS="-datadir=$D1 -debug=mempool"
$BITCOIND $B1ARGS &
B1PID=$!

D2=${D}/node2
CreateDataDir $D2 port=11010 rpcport=11011
B2ARGS="-datadir=$D2 -debug=mempool"
$BITCOIND $B2ARGS &
B2PID=$!

# Wait until all four nodes are at the same block number
function WaitBlocks {
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

# Wait until node has $N peers
function WaitPeers {
    while :
    do
        declare -i PEERS=$( $CLI $1 getconnectioncount )
        if (( PEERS == "$2" ))
        then
            break
        fi
        sleep 1
    done
}

echo "Generating test blockchain..."

# Start with B2 connected to B1:
$CLI $B2ARGS addnode 127.0.0.1:11000 onetry
WaitPeers "$B1ARGS" 1

# 2 block, 50 XBT each == 100 XBT
# These will be transactions "A" and "B"
$CLI $B1ARGS setgenerate true 2

WaitBlocks
# 100 blocks, 0 mature == 0 XBT
$CLI $B2ARGS setgenerate true 100
WaitBlocks

CheckBalance "$B1ARGS" 100
CheckBalance "$B2ARGS" 0

# restart B2 with no connection
$CLI $B2ARGS stop > /dev/null 2>&1
wait $B2PID
$BITCOIND $B2ARGS &
B2PID=$!

B1ADDRESS=$( $CLI $B1ARGS getnewaddress )
B2ADDRESS=$( $CLI $B2ARGS getnewaddress )

# Transaction C: send-to-self, spend A
TXID_C=$( $CLI $B1ARGS sendtoaddress $B1ADDRESS 50.0)

# Transaction D: spends B and C
TXID_D=$( $CLI $B1ARGS sendtoaddress $B2ADDRESS 100.0)

CheckBalance "$B1ARGS" 0

# Mutate TXID_C and add it to B2's memory pool:
RAWTX_C=$( $CLI $B1ARGS getrawtransaction $TXID_C )

# ... mutate C to create C'
L=${RAWTX_C:82:2}
NEWLEN=$( printf "%x" $(( 16#$L + 1 )) )
MUTATEDTX_C=${RAWTX_C:0:82}${NEWLEN}4c${RAWTX_C:84}
# ... give mutated tx1 to B2:
MUTATEDTXID=$( $CLI $B2ARGS sendrawtransaction $MUTATEDTX_C )

echo "TXID_C: " $TXID_C
echo "Mutated: " $MUTATEDTXID

# Re-connect nodes, and have both nodes mine some blocks:
$CLI $B2ARGS addnode 127.0.0.1:11000 onetry
WaitPeers "$B1ARGS" 1

# Having B2 mine the next block puts the mutated
# transaction C in the chain:
$CLI $B2ARGS setgenerate true 1
WaitBlocks

# B1 should still be able to spend 100, because D is conflicted
# so does not count as a spend of B
CheckBalance "$B1ARGS" 100

$CLI $B2ARGS stop > /dev/null 2>&1
wait $B2PID
$CLI $B1ARGS stop > /dev/null 2>&1
wait $B1PID

echo "Tests successful, cleaning up"
rm -rf $D
exit 0
