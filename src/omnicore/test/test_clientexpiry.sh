#!/bin/bash

SRCDIR=./src/
NUL=/dev/null
PASS=0
FAIL=0
clear
printf "Preparing a test environment...\n"
printf "   * Starting a fresh regtest daemon\n"
rm -r ~/.bitcoin/regtest
$SRCDIR/omnicored --regtest --server --daemon --omnialertallowsender=any >$NUL
sleep 10
printf "   * Preparing some mature testnet BTC\n"
$SRCDIR/omnicore-cli --regtest generate 102 >$NUL
printf "   * Obtaining a master address to work with\n"
ADDR=$($SRCDIR/omnicore-cli --regtest getnewaddress OMNIAccount)
printf "   * Funding the address with some testnet BTC for fees\n"
$SRCDIR/omnicore-cli --regtest sendtoaddress $ADDR 20 >$NUL
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   * Sending an alert with client expiry version of 999999999\n"
$SRCDIR/omnicore-cli --regtest omni_sendalert $ADDR 3 999999999 "Client version out of date test." >$NUL
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   * The client should now be shutting down, sleeping 5 seconds and then attempting to get block height - this should FAIL.\n"
sleep 5
printf "\nWe should have shut down now, if we can still query the block height, something is wrong.\n"
BLOCKFINAL=$($SRCDIR/omnicore-cli --regtest getblockcount)
printf "   * The current block height is %d\n" $BLOCKFINAL
