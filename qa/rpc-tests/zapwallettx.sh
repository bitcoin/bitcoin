#!/usr/bin/env bash

# Test zapping a single wallet transaction

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

echo "Sending coins from B1 to B3..."
for N in 1 2 3
do
    # Send N XBT from 1 to 3
    TXID[$N]=$(Send $B1ARGS $B3ARGS $N)

    # Have B2 mine 10 blocks so B1 transactions are mature:
    $CLI $B2ARGS setgenerate true 10
    WaitBlocks
done

# B1 should end up with 50 XBT in block rewards,
# minus the 6 (1, 2, 3) XBT sent to B3:
CheckBalance "$B1ARGS" "50-1-2-3"
CheckBalance "$B3ARGS" "1+2+3"

# Zap send tx from B1 (TXID[3]), balance now be 47 XBT
echo "Zapping last tx..."
$CLI "$B1ARGS" zapwallettx ${TXID[3]}
CheckBalance "$B1ARGS" "50-1-2"

# Zap send tx from B1 (TXID[1]), balance should again be 50 XBT
# as TDID[2] used an output from TXID[1] and hence also removed
echo "Zapping first tx..."
$CLI "$B1ARGS" zapwallettx ${TXID[1]}
CheckBalance "$B1ARGS" "50"

$CLI $B3ARGS stop > /dev/null 2>&1
wait $B3PID
$CLI $B2ARGS stop > /dev/null 2>&1
wait $B2PID
$CLI $B1ARGS stop > /dev/null 2>&1
wait $B1PID

echo "Tests successful, cleaning up"
rm -rf $D
exit 0
