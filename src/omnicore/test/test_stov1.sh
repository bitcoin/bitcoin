#!/bin/bash

SRCDIR=./src/
NUL=/dev/null
PASS=0
FAIL=0
clear
printf "Preparing a test environment...\n"
printf "   * Starting a fresh regtest daemon\n"
rm -r ~/.bitcoin/regtest
$SRCDIR/omnicored --regtest --server --daemon --omniactivationallowsender=any >$NUL
sleep 10
printf "   * Preparing some mature testnet BTC\n"
$SRCDIR/omnicore-cli --regtest setgenerate true 102 >$NUL
printf "   * Obtaining a master address to work with\n"
ADDR=$($SRCDIR/omnicore-cli --regtest getnewaddress OMNIAccount)
printf "   * Funding the address with some testnet BTC for fees\n"
$SRCDIR/omnicore-cli --regtest sendtoaddress $ADDR 20 >$NUL
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "   * Participating in the Exodus crowdsale to obtain some OMNI\n"
JSON="{\"moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP\":10,\""$ADDR"\":4}"
$SRCDIR/omnicore-cli --regtest sendmany OMNIAccount $JSON >$NUL
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "   * Creating an indivisible test property\n"
$SRCDIR/omnicore-cli --regtest omni_sendissuancefixed $ADDR 1 1 0 "Z_TestCat" "Z_TestSubCat" "Z_IndivisTestProperty" "Z_TestURL" "Z_TestData" 100 >$NUL
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "   * Creating a divisible test property\n"
$SRCDIR/omnicore-cli --regtest omni_sendissuancefixed $ADDR 1 2 0 "Z_TestCat" "Z_TestSubCat" "Z_DivisTestProperty" "Z_TestURL" "Z_TestData" 10000 >$NUL
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "   * Generating addresses to use as STO recipients\n"
ADDRESS=()
for i in {1..11}
do
   ADDRESS=("${ADDRESS[@]}" $($SRCDIR/omnicore-cli --regtest getnewaddress))
done
printf "   * Seeding a total of 100 SP#3\n"
printf "   * Seeding %s with 5%% = 5 SP#3\n" ${ADDRESS[1]}
$SRCDIR/omnicore-cli --regtest omni_send $ADDR ${ADDRESS[1]} 3 5 >$NUL
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "   * Seeding %s with 10%% = 10 SP#3\n" ${ADDRESS[2]}
$SRCDIR/omnicore-cli --regtest omni_send $ADDR ${ADDRESS[2]} 3 10 >$NUL
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "   * Seeding %s with 15%% = 15 SP#3\n" ${ADDRESS[3]}
$SRCDIR/omnicore-cli --regtest omni_send $ADDR ${ADDRESS[3]} 3 15 >$NUL
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "   * Seeding %s with 20%% = 20 SP#3\n" ${ADDRESS[4]}
$SRCDIR/omnicore-cli --regtest omni_send $ADDR ${ADDRESS[4]} 3 20 >$NUL
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "   * Seeding %s with 25%% = 25 SP#3\n" ${ADDRESS[5]}
$SRCDIR/omnicore-cli --regtest omni_send $ADDR ${ADDRESS[5]} 3 25 >$NUL
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "   * Seeding %s with 25%% = 25 SP#3\n" ${ADDRESS[6]}
$SRCDIR/omnicore-cli --regtest omni_send $ADDR ${ADDRESS[6]} 3 25 >$NUL
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "\nTesting a cross property (v1) STO, distributing 1000.00 SPT #4 to holders of SPT #3\n"
printf "   * Executing the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendsto $ADDR 4 1000 "" 3)
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "   * Verifiying the results\n"
printf "     # Checking the STO transaction was invalid (feature not yet activated)... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "false," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "\nActivating cross property (v1) Send To Owners...\n"
printf "   * Sending the activation\n"
BLOCKS=$($SRCDIR/omnicore-cli --regtest getblockcount)
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendactivation $ADDR 10 $(($BLOCKS + 8)) 999)
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "     # Checking the activation transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Mining 10 blocks to forward past the activation block\n"
$SRCDIR/omnicore-cli --regtest setgenerate true 10 >$NUL
printf "     # Checking the activation went live as expected... "
FEATUREID=$($SRCDIR/omnicore-cli --regtest omni_getactivations | grep -A 10 completed | grep featureid | cut -c27-28)
if [ $FEATUREID == "10" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $FEATUREID
    FAIL=$((FAIL+1))
fi
printf "\nTesting a cross property (v1) STO, distributing 1000.00 SPT #4 to holders of SPT #3\n"
printf "   * Executing the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendsto $ADDR 4 1000 "" 3)
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "   * Verifiying the results\n"
printf "     # Checking the STO transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the fee cache now has 0.00006000 fee cached for OMNI... "
CACHEDFEE=$($SRCDIR/omnicore-cli --regtest omni_getfeecache 1 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "0.00006000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "     # Checking %s received 5%% of the distribution (50.00 SPT #4)... " ${ADDRESS[1]}
BALANCE=$($SRCDIR/omnicore-cli --regtest omni_getbalance ${ADDRESS[1]} 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "50.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "     # Checking %s received 10%% of the distribution (100.00 SPT #4)... " ${ADDRESS[2]}
BALANCE=$($SRCDIR/omnicore-cli --regtest omni_getbalance ${ADDRESS[2]} 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "100.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "     # Checking %s received 15%% of the distribution (150.00 SPT #4)... " ${ADDRESS[3]}
BALANCE=$($SRCDIR/omnicore-cli --regtest omni_getbalance ${ADDRESS[3]} 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "150.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "     # Checking %s received 20%% of the distribution (200.00 SPT #4)... " ${ADDRESS[4]}
BALANCE=$($SRCDIR/omnicore-cli --regtest omni_getbalance ${ADDRESS[4]} 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "200.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "     # Checking %s received 25%% of the distribution (250.00 SPT #4)... " ${ADDRESS[5]}
BALANCE=$($SRCDIR/omnicore-cli --regtest omni_getbalance ${ADDRESS[5]} 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "250.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "     # Checking %s received 25%% of the distribution (250.00 SPT #4)... " ${ADDRESS[6]}
BALANCE=$($SRCDIR/omnicore-cli --regtest omni_getbalance ${ADDRESS[6]} 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "250.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi

printf "\n"
printf "####################\n"
printf "#  Summary:        #\n"
printf "#    Passed = %d   #\n" $PASS
printf "#    Failed = %d    #\n" $FAIL
printf "####################\n"
printf "\n"

$SRCDIR/omnicore-cli --regtest stop

