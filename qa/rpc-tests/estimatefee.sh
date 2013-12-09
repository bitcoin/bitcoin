#!/usr/bin/env bash

# Test estimatefee

#set -o xtrace  # Uncomment to debug

if [ $# -lt 1 ]; then
        echo "Usage: $0 path_to_binaries"
        echo "e.g. $0 ../../src"
        exit 1
fi

echo "Tests can take more than 10 minutes to run; be patient."

BITCOIND=${1}/bitcoind
CLI=${1}/bitcoin-cli

DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi
. "$DIR/util.sh"

D=$(mktemp -d test.XXXXX)

# Three nodes:
#  B1 relays between the other two, runs with defaults,
D1=${D}/node1
CreateDataDir $D1 port=11000 rpcport=11001 debug=estimatefee
B1ARGS="-datadir=$D1"
$BITCOIND $B1ARGS &
B1PID=$!

#  B2 runs with -paytxfee=0.001, is also used to send
#  higher/lower priority transactions
D2=${D}/node2
CreateDataDir $D2 port=11010 rpcport=11011 connect=127.0.0.1:11000 paytxfee=0.001 debug=estimatefee
B2ARGS="-datadir=$D2"
$BITCOIND $B2ARGS &
B2PID=$!

#  B3 runs with -paytxfee=0.02
D3=${D}/node3
CreateDataDir $D3 port=11020 rpcport=11021 connect=127.0.0.1:11000 paytxfee=0.02 debug=estimatefee
B3ARGS="-datadir=$D3"
$BITCOIND $BITCOINDARGS $B3ARGS &
B3PID=$!

# trap "kill -9 $B1PID $B2PID $B3PID; rm -rf $D" EXIT

echo "Generating initial blockchain"

# B2 starts with 220 blocks (so 120 old, mature):
$CLI $B2ARGS setgenerate true 220
WaitForBlock $B1ARGS 220

PRI=$($CLI $B1ARGS estimatepriority)
AssertEqual $PRI -1.0

B1ADDRESS=$(Address $B1ARGS)
B2ADDRESS=$(Address $B2ARGS)
B3ADDRESS=$(Address $B3ARGS)

echo "Testing priority estimation"

# Generate 110 very-high-priority (50 BTC with at least 100 confirmations) transactions
# (in two batches, with a block generated in between)
# 100 confirmed transactions is the minimum the estimation code needs.
for i in {1..55} ; do
    RAW=$(CreateTxn1 $B2ARGS 1 $B1ADDRESS)
    RAWTXID=$(SendRawTxn $B2ARGS $RAW)
done
$CLI $B3ARGS setgenerate true 1
WaitForBlock $B1ARGS 221
for i in {1..55} ; do
    RAW=$(CreateTxn1 $B2ARGS 1 $B1ADDRESS)
    RAWTXID=$(SendRawTxn $B2ARGS $RAW)
done
sleep 1
$CLI $B3ARGS setgenerate true 3
WaitForBlock $B1ARGS 224

PRI_HIGH=$($CLI $B3ARGS estimatepriority)
AssertGreater "$PRI_HIGH" 56000000

# Generate 100 very-low-priority free transactions;
# priority estimate should go down

for i in {1..50} ; do
    RAW=$(CreateTxn1 $B1ARGS 1 $B2ADDRESS)
    RAWTXID=$(SendRawTxn $B1ARGS $RAW)
    RAW=$(CreateTxn1 $B1ARGS 1 $B3ADDRESS)
    RAWTXID=$(SendRawTxn $B1ARGS $RAW)
done
$CLI $B3ARGS setgenerate true 3
WaitForBlock $B1ARGS 227

PRI_LOW=$($CLI $B3ARGS estimatepriority)
AssertGreater "$PRI_HIGH" "$PRI_LOW"

echo "Success. Testing fee estimation."

# At this point:
#  B1 has 60 unspent outputs with 6 or 7 confirmations
#  B2 has 17 coinbases with over 100 confirmations
#   and 50 utxos with 3 confirmations
#  B3 has no mature coins/transactions.
#
# So: create 200 1 XBT transactions from B2 to B3,
# using sendtoaddress.  The first 67 will be free, the
# rest will pay a 0.001 XBT fee, and estimatefee
# should give an estimate of over 0.00099 XBT/kilobyte
#  (transactions are less than 1KB big)
for i in {1..200} ; do
    Send $B2ARGS $B3ARGS 1
done
$CLI $B2ARGS setgenerate true 1
WaitForBlock $B3ARGS 228

FEE_LOW=$($CLI $B3ARGS estimatefee)
AssertGreater "$FEE_LOW" "0.00099"
echo "Fee low: $FEE_LOW"

# Now create 500 0.1 XBT transactions
# from B3 to B1, each paying a 0.02 XBT fee.
# At most 200 will have non-zero priority,
# so at least 300 will be zero-priority.
# Median fee estimate should go up.
for i in {1..500} ; do
    Send $B3ARGS $B1ARGS 0.1
done
$CLI $B2ARGS setgenerate true 2
WaitForBlock $B1ARGS 230

FEE_HIGH=$($CLI $B2ARGS estimatefee)
echo "Fee high: $FEE_HIGH"
AssertGreater "$FEE_HIGH" "$FEE_LOW"

# Shut down, clean up

$CLI $B3ARGS stop > /dev/null 2>&1
wait $B3PID
$CLI $B2ARGS stop > /dev/null 2>&1
wait $B2PID
$CLI $B1ARGS stop > /dev/null 2>&1
wait $B1PID

echo "Tests successful, cleaning up"
trap "" EXIT
rm -rf $D
exit 0
