#!/bin/bash

################################################################################################
## PLEASE DISABLE DEV OMNI FOR THESE TESTS!!!                                                 ##
##                                                                                            ##
## The fee distribution tests require an exact amount of Omni in each address.  Dev Omni will ##
## skew these tests by increasing the amount of Omni in the Exodus Address.                   ##
## To use this script in regtest mode, temporarily disable Dev Omni and recompile by adding:  ##
##    return 0;                                                                               ##
## as the first line of the function calculate_and_update_devmsc in omnicore.cpp.  This line  ##
## must be removed and Omni Core recompiled to use on mainnet.                                ##
################################################################################################


PASS=0
FAIL=0
clear
printf "Preparing a test environment...\n"
printf "   * Starting a fresh regtest daemon\n"
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
printf "   * Creating an indivisible test property in the test ecosystem\n"
./src/omnicore-cli --regtest omni_sendissuancefixed $ADDR 2 1 0 "Z_TestCat" "Z_TestSubCat" "Z_IndivisTestProperty" "Z_TestURL" "Z_TestData" 10000000 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Generating addresses to use as fee recipients (OMNI holders)\n"
ADDRESS=()
for i in {1..6}
do
   ADDRESS=("${ADDRESS[@]}" $(./src/omnicore-cli --regtest getnewaddress))
done
printf "   * Using a total of 1000 OMNI\n"
printf "   * Seeding %s with 50.00 OMNI\n" ${ADDRESS[1]}
./src/omnicore-cli --regtest omni_send $ADDR ${ADDRESS[1]} 1 50.0 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Seeding %s with 100.00 OMNI\n" ${ADDRESS[2]}
./src/omnicore-cli --regtest omni_send $ADDR ${ADDRESS[2]} 1 100.0 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Seeding %s with 150.00 OMNI\n" ${ADDRESS[3]}
./src/omnicore-cli --regtest omni_send $ADDR ${ADDRESS[3]} 1 150.0 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Seeding %s with 200.00 OMNI\n" ${ADDRESS[4]}
./src/omnicore-cli --regtest omni_send $ADDR ${ADDRESS[4]} 1 200.0 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "\nActivating the fee system...\n"
printf "   * Sending the activation\n"
BLOCKS=$(./src/omnicore-cli --regtest getblockcount)
TXID=$(./src/omnicore-cli --regtest omni_sendactivation $ADDR 9 $(($BLOCKS + 8)) 999)
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
if [ $FEATUREID == "9" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $FEATUREID
    FAIL=$((FAIL+1))
fi
printf "\nChecking share of fees for recipients...\n"
printf "   * Checking %s has a 5 percent share of fees... " ${ADDRESS[1]}
FEESHARE=$(./src/omnicore-cli --regtest omni_getfeeshare ${ADDRESS[1]} | grep feeshare | cut -d '"' -f4)
if [ $FEESHARE == "5.0000%" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $FEESHARE
    FAIL=$((FAIL+1))
fi
printf "   * Checking %s has a 10 percent share of fees... " ${ADDRESS[2]}
FEESHARE=$(./src/omnicore-cli --regtest omni_getfeeshare ${ADDRESS[2]} | grep feeshare | cut -d '"' -f4)
if [ $FEESHARE == "10.0000%" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $FEESHARE
    FAIL=$((FAIL+1))
fi
printf "   * Checking %s has a 15 percent share of fees... " ${ADDRESS[3]}
FEESHARE=$(./src/omnicore-cli --regtest omni_getfeeshare ${ADDRESS[3]} | grep feeshare | cut -d '"' -f4)
if [ $FEESHARE == "15.0000%" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $FEESHARE
    FAIL=$((FAIL+1))
fi
printf "   * Checking %s has a 20 percent share of fees... " ${ADDRESS[4]}
FEESHARE=$(./src/omnicore-cli --regtest omni_getfeeshare ${ADDRESS[4]} | grep feeshare | cut -d '"' -f4)
if [ $FEESHARE == "20.0000%" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $FEESHARE
    FAIL=$((FAIL+1))
fi
printf "   * Checking %s has a 50 percent share of fees... " $ADDR
FEESHARE=$(./src/omnicore-cli --regtest omni_getfeeshare $ADDR | grep feeshare | cut -d '"' -f4)
if [ $FEESHARE == "50.0000%" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $FEESHARE
    FAIL=$((FAIL+1))
fi
printf "   * Checking %s has a 100 percent share of fees in the test ecosystem... " $ADDR
FEESHARE=$(./src/omnicore-cli --regtest omni_getfeeshare $ADDR 2 | grep feeshare | cut -d '"' -f4)
if [ $FEESHARE == "100.0000%" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $FEESHARE
    FAIL=$((FAIL+1))
fi
printf "\nTesting a trade against self that results in a 1 willet fee for property 3 (1.0 OMNI for 2000 #3)\n"
printf "   * Executing the trade\n"
./src/omnicore-cli --regtest omni_sendtrade $ADDR 3 2000 1 1.0 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
./src/omnicore-cli --regtest omni_sendtrade $ADDR 1 1.0 3 2000 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking the fee cache now has 1 fee cached for property 3..."
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 3 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "1" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "      # Checking the trading address now owns 9999999 of property 3..."
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance $ADDR 3 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "9999999" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "\nTesting another trade against self that results in a 5 willet fee for property 3 (1.0 OMNI for 10000 #3)\n"
printf "   * Executing the trade\n"
./src/omnicore-cli --regtest omni_sendtrade $ADDR 3 10000 1 1.0 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
./src/omnicore-cli --regtest omni_sendtrade $ADDR 1 1.0 3 10000 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking the fee cache now has 6 fee cached for property 3... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 3 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "6" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "      # Checking the trading address now owns 9999994 instead of property 3... "
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance $ADDR 3 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "9999994" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "\nTesting a trade against self that results in a 1 willet fee for property 4 (1.0 OMNI for 0.00002 #4)\n"
printf "   * Executing the trade\n"
./src/omnicore-cli --regtest omni_sendtrade $ADDR 4 0.00002000 1 1.0 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
./src/omnicore-cli --regtest omni_sendtrade $ADDR 1 1.0 4 0.00002000 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking the fee cache now has 0.00000001 fee cached for property 4... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 4 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "0.00000001" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "      # Checking the trading address now owns 9999.99999999 of property 4... "
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance $ADDR 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "9999.99999999" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "\nTesting a trade against self that results in a 5000 willet fee for property 4 (1.0 OMNI for 0.1 #4)\n"
printf "   * Executing the trade\n"
./src/omnicore-cli --regtest omni_sendtrade $ADDR 4 0.1 1 1.0 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
./src/omnicore-cli --regtest omni_sendtrade $ADDR 1 1.0 4 0.1 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking the fee cache now has 0.00005001 fee cached for property 4... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 4 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "0.00005001" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "      # Checking the trading address now owns 9999.99994999 of property 4... "
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance $ADDR 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "9999.99994999" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "\nIncreasing volume to get close to 10000000 fee trigger point for property 4\n"
printf "   * Executing the trades\n"
for i in {1..5}
do
    ./src/omnicore-cli --regtest omni_sendtrade $ADDR 4 39.96 1 1.0 >null
    ./src/omnicore-cli --regtest setgenerate true 1 >null
    ./src/omnicore-cli --regtest omni_sendtrade $ADDR 1 1.0 4 39.96 >null
    ./src/omnicore-cli --regtest setgenerate true 1 >null
done
printf "   * Verifiying the results\n"
printf "      # Checking the fee cache now has 0.09995001 fee cached for property 4... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 4 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "0.09995001" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "      # Checking the trading address now owns 9999.90004999 of property 4... "
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance $ADDR 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "9999.90004999" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "\nPerforming a small trade to take fee cache to 0.1 and trigger distribution for property 4\n"
printf "   * Executing the trade\n"
./src/omnicore-cli --regtest omni_sendtrade $ADDR 4 0.09999999 1 0.8 >null
./src/omnicore-cli --regtest omni_sendtrade $ADDR 1 0.8 4 0.09999999 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking distribution was triggered and the fee cache is now empty for property 4... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 4 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "0.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "      # Checking %s received 0.00500000 fee share... " ${ADDRESS[1]}
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance ${ADDRESS[1]} 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "0.00500000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "      # Checking %s received 0.01000000 fee share... " ${ADDRESS[2]}
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance ${ADDRESS[2]} 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "0.01000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "      # Checking %s received 0.01500000 fee share... " ${ADDRESS[3]}
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance ${ADDRESS[3]} 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "0.01500000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "      # Checking %s received 0.02000000 fee share... " ${ADDRESS[4]}
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance ${ADDRESS[4]} 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "0.02000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "      # Checking %s received 0.05000000 fee share... " $ADDR
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance $ADDR 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "9999.95000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
./src/omnicore-cli --regtest omni_getfeetrigger 2147483651
printf "\nRolling back the chain to orphan a block and the last trade to test reorg protection (disconnecting 1 block from tip and mining a replacement)\n"
printf "   * Executing the rollback\n"
BLOCK=$(./src/omnicore-cli --regtest getblockcount)
BLOCKHASH=$(./src/omnicore-cli --regtest getblockhash $(($BLOCK)))
./src/omnicore-cli --regtest invalidateblock $BLOCKHASH >null
PREVBLOCK=$(./src/omnicore-cli --regtest getblockcount)
printf "   * Clearing the mempool\n"
./src/omnicore-cli --regtest clearmempool >null
printf "   * Verifiying the results\n"
printf "      # Checking the block count has been reduced by 1... "
EXPBLOCK=$((BLOCK-1))
if [ $EXPBLOCK == $PREVBLOCK ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $PREVBLOCK
    FAIL=$((FAIL+1))
fi
printf "   * Mining a replacement block\n"
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
NEWBLOCK=$(./src/omnicore-cli --regtest getblockcount)
NEWBLOCKHASH=$(./src/omnicore-cli --regtest getblockhash $(($BLOCK)))
printf "      # Checking the block count is the same as before the rollback... "
if [ $BLOCK == $NEWBLOCK ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $NEWBLOCK
    FAIL=$((FAIL+1))
fi
printf "      # Checking the block hash is different from before the rollback... "
if [ $BLOCKHASH == $NEWBLOCKHASH ]
  then
    printf "FAIL (result:%s)\n" $NEWBLOCKHASH
    FAIL=$((FAIL+1))
  else
    printf "PASS\n"
    PASS=$((PASS+1))
fi
printf "      # Checking the fee cache now again has 0.09995001 fee cached for property 4... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 4 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "0.09995001" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "      # Checking %s balance has been rolled back to 0... " ${ADDRESS[1]}
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance ${ADDRESS[1]} 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "0.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "      # Checking %s balance has been rolled back to 0... " ${ADDRESS[2]}
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance ${ADDRESS[2]} 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "0.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "      # Checking %s balance has been rolled back to 0... " ${ADDRESS[3]}
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance ${ADDRESS[3]} 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "0.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "      # Checking %s balance has been rolled back to 0... " ${ADDRESS[4]}
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance ${ADDRESS[4]} 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "0.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "      # Checking %s balance has been rolled back to 9999.90004999... " $ADDR
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance $ADDR 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "9999.90004999" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "\nPerforming a small trade to take fee cache to 0.1 and retrigger distribution for property 4\n"
printf "   * Executing the trade\n"
./src/omnicore-cli --regtest omni_sendtrade $ADDR 4 0.09999999 1 0.8 >null
./src/omnicore-cli --regtest omni_sendtrade $ADDR 1 0.8 4 0.09999999 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking distribution was triggered and the fee cache is now empty for property 4... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 4 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "0.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "\nTesting a trade against self that results in a 1 willet fee for property 4 (1.0 OMNI for 0.00002 #4)\n"
printf "   * Executing the trade\n"
./src/omnicore-cli --regtest omni_sendtrade $ADDR 4 0.00002000 1 1.0 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
./src/omnicore-cli --regtest omni_sendtrade $ADDR 1 1.0 4 0.00002000 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking the fee cache now has 0.00000001 fee cached for property 4... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 4 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "0.00000001" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "\nTesting another trade against self that results in a 1 willet fee for property 4 (1.0 OMNI for 0.00002 #4)\n"
printf "   * Executing the trade\n"
./src/omnicore-cli --regtest omni_sendtrade $ADDR 4 0.00002000 1 1.0 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
./src/omnicore-cli --regtest omni_sendtrade $ADDR 1 1.0 4 0.00002000 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking the fee cache now has 0.00000002 fee cached for property 4... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 4 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "0.00000002" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "\nRolling back the chain to orphan a block (disconnecting 1 block from tip and mining a replacement)\n"
printf "   * Executing the rollback\n"
BLOCK=$(./src/omnicore-cli --regtest getblockcount)
BLOCKHASH=$(./src/omnicore-cli --regtest getblockhash $(($BLOCK)))
./src/omnicore-cli --regtest invalidateblock $BLOCKHASH >null
PREVBLOCK=$(./src/omnicore-cli --regtest getblockcount)
printf "   * Clearing the mempool\n"
./src/omnicore-cli --regtest clearmempool >null
printf "   * Verifiying the results\n"
printf "      # Checking the block count has been reduced by 1... "
EXPBLOCK=$((BLOCK-1))
if [ $EXPBLOCK == $PREVBLOCK ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $PREVBLOCK
    FAIL=$((FAIL+1))
fi
printf "   * Mining a replacement block\n"
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
NEWBLOCK=$(./src/omnicore-cli --regtest getblockcount)
NEWBLOCKHASH=$(./src/omnicore-cli --regtest getblockhash $(($BLOCK)))
printf "      # Checking the block count is the same as before the rollback... "
if [ $BLOCK == $NEWBLOCK ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $NEWBLOCK
    FAIL=$((FAIL+1))
fi
printf "      # Checking the block hash is different from before the rollback... "
if [ $BLOCKHASH == $NEWBLOCKHASH ]
  then
    printf "FAIL (result:%s)\n" $NEWBLOCKHASH
    FAIL=$((FAIL+1))
  else
    printf "PASS\n"
    PASS=$((PASS+1))
fi
printf "      # Checking the fee cache has been rolled back to 0.00000001 for property 4... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 4 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "0.00000001" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "\nMining 51 blocks to test that fee cache is not affected by fee pruning\n"
printf "   * Verifiying the results\n"
printf "      # Checking the fee cache is 0.00000001 for property 4... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 4 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "0.00000001" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "   * Mining the blocks...\n"
./src/omnicore-cli --regtest setgenerate true 51 >null
printf "      # Checking the fee cache is still 0.00000001 for property 4... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 4 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "0.00000001" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "   * Executing a trade to generate 1 willet fee\n"
./src/omnicore-cli --regtest omni_sendtrade $ADDR 4 0.00002000 1 1.0 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
./src/omnicore-cli --regtest omni_sendtrade $ADDR 1 1.0 4 0.00002000 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "      # Checking the fee cache now has 0.00000002 fee cached for property 4... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 4 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "0.00000002" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "   * Executing a trade to generate 1 willet fee\n"
./src/omnicore-cli --regtest omni_sendtrade $ADDR 4 0.00002000 1 1.0 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
./src/omnicore-cli --regtest omni_sendtrade $ADDR 1 1.0 4 0.00002000 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "      # Checking the fee cache now has 0.00000003 fee cached for property 4... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 4 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "0.00000003" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "\nAdding some test ecosystem volume to trigger distribution\n"
printf "   * Executing the trades\n"
for i in {1..9}
do
    ./src/omnicore-cli --regtest omni_sendtrade $ADDR 2147483651 20000 2 10.0 >null
    ./src/omnicore-cli --regtest setgenerate true 1 >null
    ./src/omnicore-cli --regtest omni_sendtrade $ADDR 2 10.0 2147483651 20000 >null
    ./src/omnicore-cli --regtest setgenerate true 1 >null
done
printf "   * Verifiying the results\n"
printf "      # Checking the fee cache now has 90 fee cached for property 2147483651... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 2147483651 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "90" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "      # Checking the trading address now owns 9999910 of property 2147483651... "
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance $ADDR 2147483651 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "9999910" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "\nTriggering distribution in the test ecosystem for property 2147483651\n"
printf "   * Executing the trade\n"
./src/omnicore-cli --regtest omni_sendtrade $ADDR 2147483651 20000 2 10.0 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
./src/omnicore-cli --regtest omni_sendtrade $ADDR 2 10.0 2147483651 20000 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Verifiying the results\n"
printf "      # Checking distribution was triggered and the fee cache is now empty for property 2147483651... "
CACHEDFEE=$(./src/omnicore-cli --regtest omni_getfeecache 2147483651 | grep cachedfee | cut -d '"' -f4)
if [ $CACHEDFEE == "0" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $CACHEDFEE
    FAIL=$((FAIL+1))
fi
printf "      # Checking %s received 100 fee share... " $ADDR
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance $ADDR 2147483651 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "10000000" ]
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

./src/omnicore-cli --regtest stop




