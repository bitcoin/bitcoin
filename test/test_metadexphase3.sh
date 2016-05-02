#!/bin/bash

PASS=0
FAIL=0
clear
printf "Preparing a test environment...\n"
printf "   * Starting a fresh regtest daemon with open activation sender\n"
rm -r ~/.bitcoin/regtest
./src/omnicored --regtest --server --daemon --omniactivationallowsender=any >nul
sleep 3
printf "   * Preparing some mature testnet BTC\n"
./src/omnicore-cli --regtest setgenerate true 102 >null
printf "   * Obtaining a master address to work with\n"
ADDR=$(./src/omnicore-cli --regtest getnewaddress OMNIAccount)
printf "   * Funding the address with some testnet BTC for fees\n"
./src/omnicore-cli --regtest sendtoaddress $ADDR 20 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Participating in the Exodus crowdsale to obtain some OMNI\n"
JSON="{\"moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP\":10,\""$ADDR"\":4}"
./src/omnicore-cli --regtest sendmany OMNIAccount $JSON >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Creating an indivisible test property\n"
./src/omnicore-cli --regtest omni_sendissuancefixed $ADDR 1 1 0 "Z_TestCat" "Z_TestSubCat" "Z_IndivisTestProperty" "Z_TestURL" "Z_TestData" 10000000 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Creating a divisible test property\n"
./src/omnicore-cli --regtest omni_sendissuancefixed $ADDR 1 2 0 "Z_TestCat" "Z_TestSubCat" "Z_DivisTestProperty" "Z_TestURL" "Z_TestData" 10000 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "\nTesting a trade against self that uses Omni (1.1 OMNI for 20 #3)\n"
printf "   * Executing the trade\n"
TXID=$(./src/omnicore-cli --regtest omni_sendtrade $ADDR 3 20 1 1.1)
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking the trade was valid..."
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "\nTesting a trade against self that doesn't use Omni to confirm non-Omni pairs are disabled (4.45 #4 for 20 #3)\n"
printf "   * Executing the trade\n"
TXID=$(./src/omnicore-cli --regtest omni_sendtrade $ADDR 3 20 4 4.45)
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking the trade was invalid..."
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "false," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "\nActivating trading on all pairs\n"
printf "   * Sending the activation\n"
BLOCKS=$(./src/omnicore-cli --regtest getblockcount)
TXID=$(./src/omnicore-cli --regtest omni_sendactivation $ADDR 8 $(($BLOCKS + 8)) 999)
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "     # Checking the activation transaction was valid..."
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Mining 10 blocks to forward past the activation block\n"
./src/omnicore-cli --regtest setgenerate true 10 >null
printf "     # Checking the activation went live as expected..."
FEATUREID=$(./src/omnicore-cli --regtest omni_getactivations | grep -A 10 completed | grep featureid | cut -c27)
if [ $FEATUREID == "8" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $FEATUREID
    FAIL=$((FAIL+1))
fi
printf "\nTesting a trade against self that doesn't use Omni to confirm non-Omni pairs are now enabled (4.45 #4 for 20 #3)\n"
printf "   * Executing the trade\n"
TXID=$(./src/omnicore-cli --regtest omni_sendtrade $ADDR 3 20 4 4.45)
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking the trade was valid..."
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi

printf "\n"
printf "####################\n"
printf "#  Summary:        #\n"
printf "#    Passed = %d    #\n" $PASS
printf "#    Failed = %d    #\n" $FAIL
printf "####################\n"
printf "\n"

./src/omnicore-cli --regtest stop


