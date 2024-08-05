#!/usr/bin/env bash
# Demonstrate the creation and usage of UTXO snapshots.
#
# A server node starts up, IBDs up to a certain height, then generates a UTXO
# snapshot at that point.
#
# The server then downloads more blocks (to create a diff from the snapshot).
#
# We bring a client up, load the UTXO snapshot, and we show the client sync to
# the "network tip" and then start a background validation of the snapshot it
# loaded. We see the background validation chainstate removed after validation
# completes.
#
# The shellcheck rule SC2086 (quoted variables) disablements are necessary
# since this rule needs to be violated in order to get bitcoind to pick up on
# $EARLY_IBD_FLAGS for the script to work.

export LC_ALL=C
set -e

BASE_HEIGHT=${1:-30000}
INCREMENTAL_HEIGHT=20000
FINAL_HEIGHT=$((BASE_HEIGHT + INCREMENTAL_HEIGHT))

SERVER_DATADIR="$(pwd)/utxodemo-data-server-$BASE_HEIGHT"
CLIENT_DATADIR="$(pwd)/utxodemo-data-client-$BASE_HEIGHT"
UTXO_DAT_FILE="$(pwd)/utxo.$BASE_HEIGHT.dat"

# Chosen to try to not interfere with any running bitcoind processes.
SERVER_PORT=8633
SERVER_RPC_PORT=8632

CLIENT_PORT=8733
CLIENT_RPC_PORT=8732

SERVER_PORTS="-port=${SERVER_PORT} -rpcport=${SERVER_RPC_PORT}"
CLIENT_PORTS="-port=${CLIENT_PORT} -rpcport=${CLIENT_RPC_PORT}"

# Ensure the client exercises all indexes to test that snapshot use works
# properly with indexes.
ALL_INDEXES="-txindex -coinstatsindex -blockfilterindex=1"

if ! command -v jq >/dev/null ; then
  echo "This script requires jq to parse JSON RPC output. Please install it."
  echo "(e.g. sudo apt install jq)"
  exit 1
fi

DUMP_OUTPUT="dumptxoutset-output-$BASE_HEIGHT.json"

finish() {
  echo
  echo "Killing server and client PIDs ($SERVER_PID, $CLIENT_PID) and cleaning up datadirs"
  echo
  rm -f "$UTXO_DAT_FILE" "$DUMP_OUTPUT"
  rm -rf "$SERVER_DATADIR" "$CLIENT_DATADIR"
  kill -9 "$SERVER_PID" "$CLIENT_PID"
}

trap finish EXIT

# Need to specify these to trick client into accepting server as a peer
# it can IBD from, otherwise the default values prevent IBD from the server node.
EARLY_IBD_FLAGS="-maxtipage=9223372036854775207 -minimumchainwork=0x00"

server_rpc() {
  ./src/bitcoin-cli -rpcport=$SERVER_RPC_PORT -datadir="$SERVER_DATADIR" "$@"
}
client_rpc() {
  ./src/bitcoin-cli -rpcport=$CLIENT_RPC_PORT -datadir="$CLIENT_DATADIR" "$@"
}
server_sleep_til_boot() {
  while ! server_rpc ping >/dev/null 2>&1; do sleep 0.1; done
}
client_sleep_til_boot() {
  while ! client_rpc ping >/dev/null 2>&1; do sleep 0.1; done
}
server_sleep_til_shutdown() {
  while server_rpc ping >/dev/null 2>&1; do sleep 0.1; done
}

mkdir -p "$SERVER_DATADIR" "$CLIENT_DATADIR"

echo "Hi, welcome to the assumeutxo demo/test"
echo
echo "We're going to"
echo
echo "  - start up a 'server' node, sync it via mainnet IBD to height ${BASE_HEIGHT}"
echo "  - create a UTXO snapshot at that height"
echo "  - IBD ${INCREMENTAL_HEIGHT} more blocks on top of that"
echo
echo "then we'll demonstrate assumeutxo by "
echo
echo "  - starting another node (the 'client') and loading the snapshot in"
echo "    * first you'll have to modify the code slightly (chainparams) and recompile"
echo "    * don't worry, we'll make it easy"
echo "  - observing the client sync ${INCREMENTAL_HEIGHT} blocks on top of the snapshot from the server"
echo "  - observing the client validate the snapshot chain via background IBD"
echo
read -p "Press [enter] to continue" _

echo
echo "-- Starting the demo. You might want to run the two following commands in"
echo "   separate terminal windows:"
echo
echo "   watch -n0.1 tail -n 30 $SERVER_DATADIR/debug.log"
echo "   watch -n0.1 tail -n 30 $CLIENT_DATADIR/debug.log"
echo
read -p "Press [enter] to continue" _

echo
echo "-- IBDing the blocks (height=$BASE_HEIGHT) required to the server node..."
# shellcheck disable=SC2086
./src/bitcoind -logthreadnames=1 $SERVER_PORTS \
    -datadir="$SERVER_DATADIR" $EARLY_IBD_FLAGS -stopatheight="$BASE_HEIGHT" >/dev/null

echo
echo "-- Creating snapshot at ~ height $BASE_HEIGHT ($UTXO_DAT_FILE)..."
server_sleep_til_shutdown # wait for stopatheight to be hit
# shellcheck disable=SC2086
./src/bitcoind -logthreadnames=1 $SERVER_PORTS \
    -datadir="$SERVER_DATADIR" $EARLY_IBD_FLAGS -connect=0 -listen=0 >/dev/null &
SERVER_PID="$!"

server_sleep_til_boot
server_rpc dumptxoutset "$UTXO_DAT_FILE" > "$DUMP_OUTPUT"
cat "$DUMP_OUTPUT"
kill -9 "$SERVER_PID"

RPC_BASE_HEIGHT=$(jq -r .base_height < "$DUMP_OUTPUT")
RPC_AU=$(jq -r .txoutset_hash < "$DUMP_OUTPUT")
RPC_NCHAINTX=$(jq -r .nchaintx < "$DUMP_OUTPUT")
RPC_BLOCKHASH=$(jq -r .base_hash < "$DUMP_OUTPUT")

server_sleep_til_shutdown

echo
echo "-- Now: add the following to CMainParams::m_assumeutxo_data"
echo "   in src/kernel/chainparams.cpp, and recompile:"
echo
echo "   {.height = ${RPC_BASE_HEIGHT}, .hash_serialized = AssumeutxoHash{uint256{\"${RPC_AU}\"}}, .m_chain_tx_count = ${RPC_NCHAINTX}, .blockhash = consteval_ctor(uint256{\"${RPC_BLOCKHASH}\"})},"
echo
echo
echo "-- IBDing more blocks to the server node (height=$FINAL_HEIGHT) so there is a diff between snapshot and tip..."
# shellcheck disable=SC2086
./src/bitcoind $SERVER_PORTS -logthreadnames=1 -datadir="$SERVER_DATADIR" \
    $EARLY_IBD_FLAGS -stopatheight="$FINAL_HEIGHT" >/dev/null

echo
echo "-- Starting the server node to provide blocks to the client node..."
# shellcheck disable=SC2086
./src/bitcoind $SERVER_PORTS -logthreadnames=1 -debug=net -datadir="$SERVER_DATADIR" \
    $EARLY_IBD_FLAGS -connect=0 -listen=1 >/dev/null &
SERVER_PID="$!"
server_sleep_til_boot

echo
echo "-- Okay, what you're about to see is the client starting up and activating the snapshot."
echo "   I'm going to display the top 14 log lines from the client on top of an RPC called"
echo "   getchainstates, which is like getblockchaininfo but for both the snapshot and "
echo "   background validation chainstates."
echo
echo "   You're going to first see the snapshot chainstate sync to the server's tip, then"
echo "   the background IBD chain kicks in to validate up to the base of the snapshot."
echo
echo "   Once validation of the snapshot is done, you should see log lines indicating"
echo "   that we've deleted the background validation chainstate."
echo
echo "   Once everything completes, exit the watch command with CTRL+C."
echo
read -p "When you're ready for all this, hit [enter]" _

echo
echo "-- Starting the client node to get headers from the server, then load the snapshot..."
# shellcheck disable=SC2086
./src/bitcoind $CLIENT_PORTS $ALL_INDEXES -logthreadnames=1 -datadir="$CLIENT_DATADIR" \
    -connect=0 -addnode=127.0.0.1:$SERVER_PORT -debug=net $EARLY_IBD_FLAGS >/dev/null &
CLIENT_PID="$!"
client_sleep_til_boot

echo
echo "-- Initial state of the client:"
client_rpc getchainstates

echo
echo "-- Loading UTXO snapshot into client. Calling RPC in a loop..."
while ! client_rpc loadtxoutset "$UTXO_DAT_FILE" ; do sleep 10; done

watch -n 0.3 "( tail -n 14 $CLIENT_DATADIR/debug.log ; echo ; ./src/bitcoin-cli -rpcport=$CLIENT_RPC_PORT -datadir=$CLIENT_DATADIR getchainstates) | cat"

echo
echo "-- Okay, now I'm going to restart the client to make sure that the snapshot chain reloads "
echo "   as the main chain properly..."
echo
echo "   Press CTRL+C after you're satisfied to exit the demo"
echo
read -p "Press [enter] to continue"

client_sleep_til_boot
# shellcheck disable=SC2086
./src/bitcoind $CLIENT_PORTS $ALL_INDEXES -logthreadnames=1 -datadir="$CLIENT_DATADIR" -connect=0 \
    -addnode=127.0.0.1:$SERVER_PORT "$EARLY_IBD_FLAGS" >/dev/null &
CLIENT_PID="$!"
client_sleep_til_boot

watch -n 0.3 "( tail -n 14 $CLIENT_DATADIR/debug.log ; echo ; ./src/bitcoin-cli -rpcport=$CLIENT_RPC_PORT -datadir=$CLIENT_DATADIR getchainstates) | cat"

echo
echo "-- Done!"
