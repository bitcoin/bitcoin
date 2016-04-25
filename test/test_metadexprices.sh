#!/bin/bash

PASS=0
FAIL=0
clear
printf "Preparing a test environment...\n"
printf "   * Starting a fresh regtest daemon\n"
rm -r ~/.bitcoin/regtest
./src/omnicored --regtest --server --daemon >nul
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
printf "   * Creating another indivisible test property\n"
./src/omnicore-cli --regtest omni_sendissuancefixed $ADDR 1 1 0 "Z_TestCat" "Z_TestSubCat" "Z_IndivisTestProperty" "Z_TestURL" "Z_TestData" 10000000 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "\nTesting a trade against self that uses divisible / divisible (10.0 OMNI for 100.0 #4)\n"
printf "   * Executing the trade\n"
TXID=$(./src/omnicore-cli --regtest omni_sendtrade $ADDR 1 10.0 4 100.0)
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking the unit price was 10.0..."
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep unitprice | cut -d '"' -f4)
if [ $RESULT == "10.00000000000000000000000000000000000000000000000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "\nTesting a trade against self that uses divisible / indivisible (10.0 OMNI for 100 #3)\n"
printf "   * Executing the trade\n"
TXID=$(./src/omnicore-cli --regtest omni_sendtrade $ADDR 1 10.0 3 100)
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking the unit price was 10.0..."
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep unitprice | cut -d '"' -f4)
if [ $RESULT == "10.00000000000000000000000000000000000000000000000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "\nTesting a trade against self that uses indivisible / divisible (10 #3 for 100.0 OMNI)\n"
printf "   * Executing the trade\n"
TXID=$(./src/omnicore-cli --regtest omni_sendtrade $ADDR 3 10 1 100.0)
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking the unit price was 10.0..."
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep unitprice | cut -d '"' -f4)
if [ $RESULT == "10.00000000000000000000000000000000000000000000000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "\nTesting a trade against self that uses indivisible / indivisible (10 #5 for 100 #3)\n"
printf "   * Executing the trade\n"
TXID=$(./src/omnicore-cli --regtest omni_sendtrade $ADDR 5 10 3 100)
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking the unit price was 10.0..."
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep unitprice | cut -d '"' -f4)
if [ $RESULT == "10.00000000000000000000000000000000000000000000000000" ]
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


