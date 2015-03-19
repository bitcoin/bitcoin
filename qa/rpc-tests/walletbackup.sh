#!/usr/bin/env bash
# Copyright (c) 2014 The Bitcoin Core developers
# Copyright (c) 2014-2015 The Dash developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Test wallet backup / dump / restore functionality

# Test case is:
# 4 nodes. 1 2 3 and send transactions between each other,
# fourth node is a miner.
# 1 2 3 and each mine a block to start, then
# miner creates 100 blocks so 1 2 3 each have 50 mature
# coins to spend.
# Then 5 iterations of 1/2/3 sending coins amongst
# themselves to get transactions in the wallets,
# and the miner mining one block.
#
# Wallets are backed up using dumpwallet/backupwallet.
# Then 5 more iterations of transactions, then block.
#
# Miner then generates 101 more blocks, so any
# transaction fees paid mature.
#
# Sanity checks done:
#   Miner balance >= 150*50
#   Sum(1,2,3,4 balances) == 153*150
#
# 1/2/3 are shutdown, and their wallets erased.
# Then restore using wallet.dat backup. And
# confirm 1/2/3/4 balances are same as before.
#
# Shutdown again, restore using importwallet,
# and confirm again balances are correct.
#

if [ $# -lt 1 ]; then
        echo "Usage: $0 path_to_binaries"
        echo "e.g. $0 ../../src"
        exit 1
fi

BITCOIND=${1}/dashd
CLI=${1}/dash-cli

DIR="${BASH_SOURCE%/*}"
SENDANDWAIT="${DIR}/send.sh"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi
. "$DIR/util.sh"

D=$(mktemp -d test.XXXXX)

echo "Starting nodes..."

# "Miner":
D4=${D}/node4
CreateDataDir $D4 port=11030 rpcport=11031
B4ARGS="-datadir=$D4"
$BITCOIND $BITCOINDARGS $B4ARGS &
B4PID=$!

# Want default keypool for 1/2/3, and
# don't need send-and-wait functionality,
# so don't use CreateDataDir:
function CreateConfDir {
  DIR=$1
  mkdir -p $DIR
  CONF=$DIR/dash.conf
  echo "regtest=1" >> $CONF
  echo "rpcuser=rt" >> $CONF
  echo "rpcpassword=rt" >> $CONF
  echo "rpcwait=1" >> $CONF
  shift
  while (( "$#" )); do
      echo $1 >> $CONF
      shift
  done
}

# "Spenders" 1/2/3
D1=${D}/node1
CreateConfDir $D1 port=11000 rpcport=11001 addnode=127.0.0.1:11030
B1ARGS="-datadir=$D1"
$BITCOIND $B1ARGS &
B1PID=$!
D2=${D}/node2
CreateConfDir $D2 port=11010 rpcport=11011 addnode=127.0.0.1:11030
B2ARGS="-datadir=$D2"
$BITCOIND $B2ARGS &
B2PID=$!
D3=${D}/node3
CreateConfDir $D3 port=11020 rpcport=11021 addnode=127.0.0.1:11030 addnode=127.0.0.1:11000
B3ARGS="-datadir=$D3"
$BITCOIND $BITCOINDARGS $B3ARGS &
B3PID=$!

# Wait until all nodes are at the same block number
function WaitBlocks {
    while :
    do
        sleep 1
        BLOCKS1=$( GetBlocks "$B1ARGS" )
        BLOCKS2=$( GetBlocks "$B2ARGS" )
        BLOCKS3=$( GetBlocks "$B3ARGS" )
        BLOCKS4=$( GetBlocks "$B4ARGS" )
        if (( BLOCKS1 == BLOCKS4 && BLOCKS2 == BLOCKS4 && BLOCKS3 == BLOCKS4 ))
        then
            break
        fi
    done
}

# Wait until all nodes have the same txns in
# their memory pools
function WaitMemPools {
    while :
    do
        sleep 1
        MEMPOOL1=$( $CLI "$B1ARGS" getrawmempool | sort | shasum )
        MEMPOOL2=$( $CLI "$B2ARGS" getrawmempool | sort | shasum )
        MEMPOOL3=$( $CLI "$B3ARGS" getrawmempool | sort | shasum )
        MEMPOOL4=$( $CLI "$B4ARGS" getrawmempool | sort | shasum )
        if [[ $MEMPOOL1 = $MEMPOOL4 && $MEMPOOL2 = $MEMPOOL4 && $MEMPOOL3 = $MEMPOOL4 ]]
        then
            break
        fi
    done
}

echo "Generating initial blockchain..."

# 1 block, 50 XBT each == 50 BTC
$CLI $B1ARGS setgenerate true 1
WaitBlocks
$CLI $B2ARGS setgenerate true 1
WaitBlocks
$CLI $B3ARGS setgenerate true 1
WaitBlocks

# 100 blocks, 0 mature
$CLI $B4ARGS setgenerate true 100
WaitBlocks

CheckBalance "$B1ARGS" 50
CheckBalance "$B2ARGS" 50
CheckBalance "$B3ARGS" 50
CheckBalance "$B4ARGS" 0

echo "Creating transactions..."

function S {
  TXID=$( $CLI -datadir=${D}/node${1} sendtoaddress ${2} "${3}" 0 )
  if [[ $TXID == "" ]] ; then
      echoerr "node${1}: error sending ${3} btc"
      echo -n "node${1} balance: "
      $CLI -datadir=${D}/node${1} getbalance "*" 0
      exit 1
  fi
}

function OneRound {
  A1=$( $CLI $B1ARGS getnewaddress )
  A2=$( $CLI $B2ARGS getnewaddress )
  A3=$( $CLI $B3ARGS getnewaddress )
  if [[ $(( $RANDOM%2 )) < 1 ]] ; then
      N=$(( $RANDOM % 9 + 1 ))
      S 1 $A2 "0.$N"
  fi
  if [[ $(( $RANDOM%2 )) < 1 ]] ; then
      N=$(( $RANDOM % 9 + 1 ))
      S 1 $A3 "0.0$N"
  fi
  if [[ $(( $RANDOM%2 )) < 1 ]] ; then
      N=$(( $RANDOM % 9 + 1 ))
      S 2 $A1 "0.$N"
  fi
  if [[ $(( $RANDOM%2 )) < 1 ]] ; then
      N=$(( $RANDOM % 9 + 1 ))
      S 2 $A3 "0.$N"
  fi
  if [[ $(( $RANDOM%2 )) < 1 ]] ; then
      N=$(( $RANDOM % 9 + 1 ))
      S 3 $A1 "0.$N"
  fi
  if [[ $(( $RANDOM%2 )) < 1 ]] ; then
      N=$(( $RANDOM % 9 + 1 ))
      S 3 $A2 "0.0$N"
  fi
  $CLI "$B4ARGS" setgenerate true 1
}

for i in {1..5}; do OneRound ; done

echo "Backing up..."

$CLI "$B1ARGS" backupwallet "$D1/wallet.bak"
$CLI "$B1ARGS" dumpwallet "$D1/wallet.dump"
$CLI "$B2ARGS" backupwallet "$D2/wallet.bak"
$CLI "$B2ARGS" dumpwallet "$D2/wallet.dump"
$CLI "$B3ARGS" backupwallet "$D3/wallet.bak"
$CLI "$B3ARGS" dumpwallet "$D3/wallet.dump"

echo "More transactions..."
for i in {1..5}; do OneRound ; done

WaitMemPools

# Generate 101 more blocks, so any fees paid
# mature
$CLI "$B4ARGS" setgenerate true 101

BALANCE1=$( $CLI "$B1ARGS" getbalance )
BALANCE2=$( $CLI "$B2ARGS" getbalance )
BALANCE3=$( $CLI "$B3ARGS" getbalance )
BALANCE4=$( $CLI "$B4ARGS" getbalance )

TOTAL=$( dc -e "$BALANCE1 $BALANCE2 $BALANCE3 $BALANCE4 + + + p" )

AssertEqual $TOTAL 5700.00000000

function StopThree {
  $CLI $B1ARGS stop > /dev/null 2>&1
  $CLI $B2ARGS stop > /dev/null 2>&1
  $CLI $B3ARGS stop > /dev/null 2>&1
  wait $B1PID
  wait $B2PID
  wait $B3PID
}
function EraseThree {
  rm $D1/regtest/wallet.dat
  rm $D2/regtest/wallet.dat
  rm $D3/regtest/wallet.dat
}
function StartThree {
  $BITCOIND $BITCOINDARGS $B1ARGS &
  B1PID=$!
  $BITCOIND $BITCOINDARGS $B2ARGS &
  B2PID=$!
  $BITCOIND $BITCOINDARGS $B3ARGS &
  B3PID=$!
}

echo "Restoring using wallet.dat"

StopThree
EraseThree

# Start node3 with no chain
rm -rf $D3/regtest/blocks
rm -rf $D3/regtest/chainstate
rm -rf $D3/regtest/database

cp $D1/wallet.bak $D1/regtest/wallet.dat
cp $D2/wallet.bak $D2/regtest/wallet.dat
cp $D3/wallet.bak $D3/regtest/wallet.dat

StartThree
WaitBlocks

AssertEqual $BALANCE1 $( $CLI "$B1ARGS" getbalance )
AssertEqual $BALANCE2 $( $CLI "$B2ARGS" getbalance )
AssertEqual $BALANCE3 $( $CLI "$B3ARGS" getbalance )

echo "Restoring using dumped wallet"

StopThree
EraseThree

# Start node3 with no chain
rm -rf $D3/regtest/blocks
rm -rf $D3/regtest/chainstate
rm -rf $D3/regtest/database

StartThree

AssertEqual 0 $( $CLI "$B1ARGS" getbalance )
AssertEqual 0 $( $CLI "$B2ARGS" getbalance )
AssertEqual 0 $( $CLI "$B3ARGS" getbalance )

$CLI "$B1ARGS" importwallet $D1/wallet.dump
$CLI "$B2ARGS" importwallet $D2/wallet.dump
$CLI "$B3ARGS" importwallet $D3/wallet.dump

WaitBlocks

AssertEqual $BALANCE1 $( $CLI "$B1ARGS" getbalance )
AssertEqual $BALANCE2 $( $CLI "$B2ARGS" getbalance )
AssertEqual $BALANCE3 $( $CLI "$B3ARGS" getbalance )

StopThree
$CLI $B4ARGS stop > /dev/null 2>&1
wait $B4PID

echo "Tests successful, cleaning up"
trap "" EXIT
rm -rf $D
exit 0
