// Copyright (c) 2016-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/test_syscoin_services.h>
#include <test/data/ethspv_valid.json.h>
#include <util/time.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <services/asset.h>
#include <base58.h>
#include <chainparams.h>
#include <boost/test/unit_test.hpp>
#include <iterator>
#include <core_io.h>
#include <key.h>
#include <math.h>
#include <key_io.h>
#include <univalue.h>
#include <util/strencodings.h>
#include <services/rpc/assetrpc.h>
using namespace std;
extern UniValue read_json(const std::string& jsondata);

BOOST_FIXTURE_TEST_SUITE(syscoin_asset_tests, BasicSyscoinTestingSetup)

BOOST_AUTO_TEST_CASE(generate_big_assetdata)
{
	tfm::format(std::cout,"Running generate_big_assetdata...\n");
	GenerateBlocks(5);
	string newaddress = GetNewFundedAddress("node1");
	// 256 bytes long
	string gooddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	// 257 bytes long
	UniValue r;
	string baddata = gooddata + "a";
	string guid = AssetNew("node1", newaddress, gooddata);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "listassets"));
	UniValue rArray = r.get_array();
	BOOST_CHECK(rArray.size() > 0);
	BOOST_CHECK_EQUAL(itostr(find_value(rArray[0].get_obj(), "asset_guid").get_uint()), guid);
	string guid1 = AssetNew("node1", newaddress, gooddata);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid));
	BOOST_CHECK(itostr(find_value(r.get_obj(), "asset_guid").get_uint()) == guid);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid1));
    BOOST_CHECK(itostr(find_value(r.get_obj(), "asset_guid").get_uint()) == guid1);
}
BOOST_AUTO_TEST_CASE(generate_asset_spt_sysx)
{
    UniValue r;
    tfm::format(std::cout,"Running generate_asset_spt_sysx...\n");
    GenerateBlocks(5);
    GenerateBlocks(5, "node2");
    GenerateBlocks(5, "node3");
    string updateFlags = itostr(ASSET_UPDATE_CONTRACT | ASSET_UPDATE_DATA);
    // cannot edit supply
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetupdate " + strSYSXAsset + " " + strSYSXAddress + " '' 1 " + updateFlags + " ''"));

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string hexStr = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
    BOOST_CHECK(r.write().size() < 32);

    // can update contract + pub data
    AssetUpdate("node1", strSYSXAsset, "pub", "''", updateFlags, "0x931d387731bbbc988b312206c74f77d004d6b84b");   
}
BOOST_AUTO_TEST_CASE(generate_asset_address_spend)
{
    UniValue r;
    tfm::format(std::cout,"Running generate_asset_address_spend...\n");
    GenerateBlocks(5);
    GenerateBlocks(5, "node2");
    GenerateBlocks(5, "node3");
    // fund asset+allocation on addresses and try to spend from those addresses it should be disallowed
    string creatoraddress = GetNewFundedAddress("node3");
    string useraddress = GetNewFundedAddress("node3");

    BOOST_CHECK_NO_THROW(r = CallExtRPC("node3", "getbalance"));
    std::string resBalance = r.write();
    CAmount nAmount = AmountFromValue(resBalance) - 3*COIN; // 3 coins for fees
    std::string strAmount = ValueFromAmount(nAmount).write();
    CAmount nAmountHalf = nAmount/2;
    std::string strAmountHalf = ValueFromAmount(nAmountHalf).write();
    BOOST_CHECK_NO_THROW(r = CallExtRPC("node3", "getnewaddress"));
	std::string newaddress = r.get_str();
    CallRPC("node3", "sendtoaddress " + creatoraddress + " " + strAmountHalf, true, false);
    CallRPC("node3", "sendtoaddress " + useraddress + " " + strAmountHalf, true, false);
    GenerateBlocks(5, "node3");
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "addressbalance " + creatoraddress));
    CAmount nAmountBalance = AmountFromValue(find_value(r.get_obj(), "amount"));
    BOOST_CHECK_EQUAL(nAmountBalance, nAmountHalf);

    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "addressbalance " + useraddress));
    nAmountBalance = AmountFromValue(find_value(r.get_obj(), "amount"));
    BOOST_CHECK_EQUAL(nAmountBalance, nAmountHalf);

    string assetguid = AssetNew("node3", creatoraddress, "pubdata", "0x931d387731bbbc988b312206c74f77d004d6b84b");

    AssetSend("node3", assetguid, "\"[{\\\"address\\\":\\\"" + useraddress + "\\\",\\\"amount\\\":0.5}]\"");
 
    // try to send coins and see it doesn't go through because balances are the same
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "addressbalance " + creatoraddress));
    nAmountBalance = AmountFromValue(find_value(r.get_obj(), "amount"));
    
    BOOST_CHECK_NO_THROW(r = CallExtRPC("node3", "getnewaddress"));
	newaddress = r.get_str();


    CallRPC("node3", "sendtoaddress " + useraddress + " " + strAmountHalf, true, false);
    GenerateBlocks(5, "node3");

    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "addressbalance " + creatoraddress));
    CAmount nAmountBalanceAfter = AmountFromValue(find_value(r.get_obj(), "amount"));
    BOOST_CHECK_EQUAL(nAmountBalance, nAmountBalanceAfter);

    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "addressbalance " + useraddress));
    nAmountBalance = AmountFromValue(find_value(r.get_obj(), "amount"));
    
    BOOST_CHECK_NO_THROW(r = CallExtRPC("node3", "getnewaddress"));
	newaddress = r.get_str();

    CallRPC("node3", "sendtoaddress " + creatoraddress + " " + strAmountHalf, true, false);
    GenerateBlocks(5, "node3");

    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "addressbalance " + useraddress));
    nAmountBalanceAfter = AmountFromValue(find_value(r.get_obj(), "amount"));
    BOOST_CHECK_EQUAL(nAmountBalance, nAmountBalanceAfter);

    // clear the allocation so we can send the sys again from useraddress
    AssetAllocationTransfer(false, "node3", assetguid, useraddress, "\"[{\\\"address\\\":\\\"" + creatoraddress + "\\\",\\\"amount\\\":0.5}]\"");

    // try again and this time should pass
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "addressbalance " + useraddress));
    nAmountBalance = AmountFromValue(find_value(r.get_obj(), "amount"));
    
    BOOST_CHECK_NO_THROW(r = CallExtRPC("node3", "getnewaddress"));
	newaddress = r.get_str();

    CallRPC("node3", "sendtoaddress " + creatoraddress + " " + strAmountHalf, true, false);
    GenerateBlocks(5, "node3");

    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "addressbalance " + useraddress));
    nAmountBalanceAfter = AmountFromValue(find_value(r.get_obj(), "amount"));
    // would have completely spent funds from this address to pay the creatoraddress
    BOOST_CHECK_EQUAL(0, nAmountBalanceAfter);

    // sendfrom should work on creatoraddress because we use create a manual raw tx sourced from creatoraddress which doesn't hit the auto-selection wallet algorithm

    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "addressbalance " + creatoraddress));
    nAmountBalance = AmountFromValue(find_value(r.get_obj(), "amount"));
    
    BOOST_CHECK_NO_THROW(r = CallExtRPC("node3", "getnewaddress"));
	newaddress = r.get_str();


    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "sendfrom " + creatoraddress + " " + useraddress + " " + strAmountHalf));
    string hex_str = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "signrawtransactionwithwallet " + hex_str));
    hex_str = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "sendrawtransaction " + hex_str, true, false));
   
    GenerateBlocks(5, "node3");

    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "addressbalance " + creatoraddress));
    nAmountBalanceAfter = AmountFromValue(find_value(r.get_obj(), "amount"));
    // now creator address should be down about half the coins (half coins + fees in sendfrom) since it got all of the them in the previous sendtoaddress that worked
    BOOST_CHECK((nAmountBalance - nAmountHalf) > nAmountBalanceAfter);
    BOOST_CHECK((nAmountBalance - nAmountHalf - nAmountBalanceAfter) < 0.001*COIN);
}

BOOST_AUTO_TEST_CASE(generate_asset_audittxroot)
{
    tfm::format(std::cout,"Running generate_asset_audittxroot...\n");
    UniValue r;
    BOOST_CHECK_NO_THROW(CallRPC("node1", "syscoinsetethheaders \"[[709780,\\\"709780\\\",\\\"709779\\\",\\\"a\\\",\\\"a\\\",0],[707780,\\\"707780\\\",\\\"707779\\\",\\\"a\\\",\\\"a\\\",0],[707772,\\\"707772\\\",\\\"707771\\\",\\\"a\\\",\\\"a\\\",0],[707776,\\\"707776\\\",\\\"707775\\\",\\\"a\\\",\\\"a\\\",0],[707770,\\\"707770\\\",\\\"707769\\\",\\\"a\\\",\\\"a\\\",0],[707778,\\\"707778\\\",\\\"707777\\\",\\\"a\\\",\\\"a\\\",0],[707774,\\\"707774\\\",\\\"707773\\\",\\\"a\\\",\\\"a\\\",0]]\""));
    int64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsetethstatus synced 709780"));
    int64_t end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    tfm::format(std::cout,"syscoinsetethstatus elasped time %lld\n", end-start);
    UniValue blocksArray = find_value(r.get_obj(), "missing_blocks").get_array();
    // the - MAX_ETHEREUM_TX_ROOTS check to ensure you have at least that many roots stored from the tip
    BOOST_CHECK_EQUAL(find_value(blocksArray[0].get_obj(), "from").get_uint() , 659780);
    BOOST_CHECK(find_value(blocksArray[0].get_obj(), "to").get_uint() == 707769);

    BOOST_CHECK(find_value(blocksArray[1].get_obj(), "from").get_uint() == 707771);
    BOOST_CHECK(find_value(blocksArray[1].get_obj(), "to").get_uint() == 707771);
    BOOST_CHECK(find_value(blocksArray[2].get_obj(), "from").get_uint() == 707773);
    BOOST_CHECK(find_value(blocksArray[2].get_obj(), "to").get_uint() == 707773);
    BOOST_CHECK(find_value(blocksArray[3].get_obj(), "from").get_uint() == 707775);
    BOOST_CHECK(find_value(blocksArray[3].get_obj(), "to").get_uint() == 707775);
    BOOST_CHECK(find_value(blocksArray[4].get_obj(), "from").get_uint() == 707777);
    BOOST_CHECK(find_value(blocksArray[4].get_obj(), "to").get_uint() == 707777);
    BOOST_CHECK(find_value(blocksArray[5].get_obj(), "from").get_uint() == 707779);
    BOOST_CHECK(find_value(blocksArray[5].get_obj(), "to").get_uint() == 707779);
    BOOST_CHECK(find_value(blocksArray[6].get_obj(), "from").get_uint() == 707781);
    BOOST_CHECK(find_value(blocksArray[6].get_obj(), "to").get_uint() == 709779);
    BOOST_CHECK_NO_THROW(CallRPC("node1", "syscoinsetethheaders \"[[707773,\\\"707773\\\",\\\"707772\\\",\\\"a\\\",\\\"a\\\",0],[707775,\\\"707775\\\",\\\"707774\\\",\\\"a\\\",\\\"a\\\",0],[707771,\\\"707771\\\",\\\"707770\\\",\\\"a\\\",\\\"a\\\",0],[707777,\\\"707777\\\",\\\"707776\\\",\\\"a\\\",\\\"a\\\",0],[707779,\\\"707779\\\",\\\"707778\\\",\\\"a\\\",\\\"a\\\",0],[707781,\\\"707781\\\",\\\"707780\\\",\\\"a\\\",\\\"a\\\",0]]\""));
    start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsetethstatus synced 709780"));
    end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    tfm::format(std::cout,"syscoinsetethstatus1 elasped time %lld\n", end-start);
    blocksArray = find_value(r.get_obj(), "missing_blocks").get_array();
    BOOST_CHECK(find_value(blocksArray[0].get_obj(), "from").get_uint() == 659780);
    BOOST_CHECK(find_value(blocksArray[0].get_obj(), "to").get_uint() == 707769);

    BOOST_CHECK(find_value(blocksArray[1].get_obj(), "from").get_uint() == 707782);
    BOOST_CHECK(find_value(blocksArray[1].get_obj(), "to").get_uint() == 709779);

    // now fork and check it revalidates chain
    // 707771 (should be 707772) -> 707773 and 707773 (should be 707774) -> 707775
    BOOST_CHECK_NO_THROW(CallRPC("node1", "syscoinsetethheaders \"[[707773,\\\"707773\\\",\\\"707771\\\",\\\"a\\\",\\\"a\\\",0],[707775,\\\"707775\\\",\\\"707773\\\",\\\"a\\\",\\\"a\\\",0]]\""));
    start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsetethstatus synced 709780"));
    end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    tfm::format(std::cout,"syscoinsetethstatus2 elasped time %lld\n", end-start);
    blocksArray = find_value(r.get_obj(), "missing_blocks").get_array();

    BOOST_CHECK_EQUAL(blocksArray.size(), 4);
    // we should still have the missing ranges prior to the forks
    BOOST_CHECK_EQUAL(find_value(blocksArray[0].get_obj(), "from").get_uint(),659780);
    BOOST_CHECK_EQUAL(find_value(blocksArray[0].get_obj(), "to").get_uint() ,707769);
    
    // 707773 is affected, -50 is 707723 and +50 is 707823
    BOOST_CHECK_EQUAL(find_value(blocksArray[1].get_obj(), "from").get_uint() , 707723);
    BOOST_CHECK_EQUAL(find_value(blocksArray[1].get_obj(), "to").get_uint() , 707823);
    
    BOOST_CHECK_EQUAL(find_value(blocksArray[2].get_obj(), "from").get_uint() , 707725);
    BOOST_CHECK_EQUAL(find_value(blocksArray[2].get_obj(), "to").get_uint() , 707825);
    
    // last missing range stays in the missing range list
    BOOST_CHECK_EQUAL(find_value(blocksArray[3].get_obj(), "from").get_uint() , 707782);
    BOOST_CHECK_EQUAL(find_value(blocksArray[3].get_obj(), "to").get_uint() , 709779);
}
BOOST_AUTO_TEST_CASE(generate_asset_audittxroot1)
{
    tfm::format(std::cout,"Running generate_asset_audittxroot1...\n");
    UniValue r;
    std::string roots = "";
    unsigned int nStartHeight = 700000;
    unsigned int nMissingRange1 = 700059;
    unsigned int nMissingRange2 = 800022;
    unsigned int nMissingRange3 = 814011;
    unsigned int nCount = 0;
    for(unsigned int i = nStartHeight;i<(nStartHeight+120000);i++){
        if(i == nMissingRange1 || i == nMissingRange2 || i == nMissingRange3)
            continue;
        if(nCount > 0)
            roots += ",";
        roots += strprintf("[%d,\\\"%d\\\",\\\"%d\\\",\\\"a\\\",\\\"a\\\",0]", i, i, i-1);
        if(nCount > 0 && (nCount % 400) == 0){
            BOOST_CHECK_NO_THROW(CallRPC("node1", "syscoinsetethheaders \"[" + roots + "]\""));
            roots = "";
            nCount = 0;
            continue;
        }
        nCount++;
    }
    if(!roots.empty())
        BOOST_CHECK_NO_THROW(CallRPC("node1", "syscoinsetethheaders \"[" + roots + "]\""));
    int64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsetethstatus synced 820000"));
    int64_t end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    tfm::format(std::cout,"syscoinsetethstatus elasped time %lld\n", end-start);
    UniValue blocksArray = find_value(r.get_obj(), "missing_blocks").get_array();
    BOOST_CHECK(find_value(blocksArray[0].get_obj(), "from").get_uint() == 700059);
    BOOST_CHECK(find_value(blocksArray[0].get_obj(), "to").get_uint() == 700059);
    BOOST_CHECK(find_value(blocksArray[1].get_obj(), "from").get_uint() == 800022);
    BOOST_CHECK(find_value(blocksArray[1].get_obj(), "to").get_uint() == 800022);
    BOOST_CHECK(find_value(blocksArray[2].get_obj(), "from").get_uint() == 814011);
    BOOST_CHECK(find_value(blocksArray[2].get_obj(), "to").get_uint() == 814011);
    BOOST_CHECK_NO_THROW(CallRPC("node1", "syscoinsetethheaders \"[[814011,\\\"814011\\\",\\\"814010\\\",\\\"a\\\",\\\"a\\\",0],[700059,\\\"700059\\\",\\\"700058\\\",\\\"a\\\",\\\"a\\\",0],[800022,\\\"800022\\\",\\\"800021\\\",\\\"a\\\",\\\"a\\\",0]]\""));
    start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsetethstatus synced 820000"));
    end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    tfm::format(std::cout,"syscoinsetethstatus1 elasped time %lld\n", end-start);
    blocksArray = find_value(r.get_obj(), "missing_blocks").get_array();
    BOOST_CHECK(blocksArray.empty());
}
/*BOOST_AUTO_TEST_CASE(generate_asset_throughput)
{

    int64_t start = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    UniValue r;
    tfm::format(std::cout,"Running generate_asset_throughput...\n");
    GenerateBlocks(5, "node1");
    GenerateBlocks(5, "node3");
    vector<string> vecAssets;
    // setup senders and receiver node addresses
    vector<string> senders;
    vector<string> receivers;
    senders.push_back("node1");
    senders.push_back("node2");
    receivers.push_back("node3");
    BOOST_CHECK(receivers.size() == 1);
    // user modifiable variables

    // for every asset you add numberOfAssetSendsPerBlock tx's effectively
    int numAssets = 10;
    BOOST_CHECK(numAssets >= 1);

    int numberOfAssetSendsPerBlock = 250;
    BOOST_CHECK(numberOfAssetSendsPerBlock >= 1 && numberOfAssetSendsPerBlock <= 250);

      // NOT MEANT TO BE MODIFIED! CALCULATE INSTEAD!
    const int numberOfTransactionToSend = numAssets*numberOfAssetSendsPerBlock;

    // make sure numberOfAssetSendsPerBlock isn't a fraction of numberOfTransactionToSend
    BOOST_CHECK((numberOfTransactionToSend % numberOfAssetSendsPerBlock) == 0);

    vector<string> unfundedAccounts;
    vector<string> rawSignedAssetAllocationSends;
    vector<string> vecFundedAddresses;
    GenerateBlocks((numAssets+1)/ numberOfAssetSendsPerBlock);
    // PHASE 1:  GENERATE UNFUNDED ADDRESSES FOR RECIPIENTS TO ASSETALLOCATIONSEND
    tfm::format(std::cout,"Throughput test: Total transaction count: %d, Receivers Per Asset Allocation Transfer %d, Total Number of Assets needed %d\n\n", numberOfTransactionToSend, numberOfAssetSendsPerBlock, numAssets);
    tfm::format(std::cout,"creating %d unfunded addresses...\n", numberOfAssetSendsPerBlock);
    for(int i =0;i<numberOfAssetSendsPerBlock;i++){
        BOOST_CHECK_NO_THROW(r = CallExtRPC("node1", "getnewaddress"));
        unfundedAccounts.emplace_back(r.get_str());
    }

   // PHASE 2:  GENERATE FUNDED ADDRESSES FOR CREATING AND SENDING ASSETS
    // create address for funding
    BOOST_CHECK_NO_THROW(r = CallExtRPC("node1", "getnewaddress"));
    string fundedAccount = r.get_str();
    tfm::format(std::cout,"creating %d funded accounts for using with assetsend/assetallocationsend in subsequent steps...\n", numAssets*numberOfAssetSendsPerBlock);
    string sendManyString = "";
    for(int i =0;i<numAssets;i++){
        BOOST_CHECK_NO_THROW(r = CallExtRPC("node1", "getnewaddress"));
        string fundedAccount = r.get_str();
        if(sendManyString != "")
            sendManyString += ",";
        sendManyString += "\"" + fundedAccount + "\":1";
        if(((i+1)% numberOfAssetSendsPerBlock)==0){
            tfm::format(std::cout,"Sending funds to batch of %d funded accounts, approx. %d batches remaining\n", numberOfAssetSendsPerBlock, (numAssets-i)/ numberOfAssetSendsPerBlock);
            std::string strSendMany = "sendmany \"\" {" + sendManyString + "}";
            CallExtRPC("node1", "sendmany", "\"\",{" + sendManyString + "}");
            sendManyString = "";

        }
        vecFundedAddresses.push_back(fundedAccount);
    }
    if(!sendManyString.empty()){
        std::string strSendMany = "sendmany \"\" {" + sendManyString + "}";
        CallExtRPC("node1", "sendmany", "\"\",{" + sendManyString + "}");
    }
    GenerateBlocks(5);

    // PHASE 3:  SET tpstestsetenabled ON ALL SENDER/RECEIVER NODES
    for (auto &sender : senders)
        BOOST_CHECK_NO_THROW(CallExtRPC(sender, "tpstestsetenabled", "true"));
    for (auto &receiver : receivers)
        BOOST_CHECK_NO_THROW(CallExtRPC(receiver, "tpstestsetenabled", "true"));

     // PHASE 4:  CREATE ASSETS
    // create assets needed
    tfm::format(std::cout,"creating %d sender assets...\n", numAssets);
    for(int i =0;i<numAssets;i++){
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetnew " + vecFundedAddresses[i] + " tps '' '' 8 250 250 31 ''"));
        string guid = itostr(find_value(r.get_obj(), "asset_guid").get_uint());
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
        string hex_str = find_value(r.get_obj(), "hex").get_str();
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hex_str, true, false));
        vecAssets.push_back(guid);
    }
    GenerateBlocks(5);
    tfm::format(std::cout,"sending assets with assetsend...\n");
    // PHASE 5:  SEND ASSETS TO NEW ALLOCATIONS
    for(int i =0;i<numAssets;i++){
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "listassetindexassets " + vecFundedAddresses[i]));
        UniValue indexArray = r.get_array();
        BOOST_CHECK_EQUAL(indexArray.size(), 1);
        uint32_t nAsset = find_value(indexArray[0].get_obj(), "asset_guid").get_uint();
        uint32_t nAssetStored;
        ParseUInt32(vecAssets[i], &nAssetStored);
        BOOST_CHECK_EQUAL(nAsset, nAssetStored);
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetsendmany " + vecAssets[i] + " \"[{\\\"address\\\":\\\"" + vecFundedAddresses[i] + "\\\",\\\"amount\\\":250}]\" ''"));

        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
        string hex_str = find_value(r.get_obj(), "hex").get_str();
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hex_str, true, false));       
    }

    GenerateBlocks(5);

    // PHASE 6:  SEND ALLOCATIONS TO NEW ALLOCATIONS (UNFUNDED ADDRESSES) USING ZDAG
    tfm::format(std::cout,"Creating assetallocationsend transactions...\n");
    int count = 0;
    int unfoundedAccountIndex = 0;
    int assetAllocationSendIndex = 0;
    // create vector of signed transactions
    string assetAllocationSendMany = "";
    int txCount = 0;
    for(int i =0;i<numAssets;i++){
        // send asset to numberOfAssetSendsPerBlock addresses
        string assetAllocationSendMany = "";
        // +1 to account for change output
        for (int j = 0; j < numberOfAssetSendsPerBlock; j++) {
            if(assetAllocationSendMany != "")
                assetAllocationSendMany += ",";
            assetAllocationSendMany += "{\\\"address\\\":\\\"" + unfundedAccounts[unfoundedAccountIndex++] + "\\\",\\\"amount\\\":1}";
            if(unfoundedAccountIndex >= unfundedAccounts.size())
                unfoundedAccountIndex = 0;
        }

        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsendmany " + vecAssets[i] + " " + vecFundedAddresses[i] + " \"[" + assetAllocationSendMany + "]\" ''"));
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
        string hex_str = find_value(r.get_obj(), "hex").get_str();
        rawSignedAssetAllocationSends.push_back(hex_str);
    }

    BOOST_CHECK(assetAllocationSendMany.empty());

    // PHASE 7:  DISTRIBUTE LOAD AMONG SENDERS
    // push vector of signed transactions to tpstestadd on every sender node distributed evenly
    int txPerSender = rawSignedAssetAllocationSends.size() / senders.size();
    tfm::format(std::cout,"Dividing work (%d transactions) between %d senders (%d per sender)...\n", rawSignedAssetAllocationSends.size(), senders.size(), txPerSender);

    unsigned int j=0;
    unsigned int i=0;
    unsigned int senderIndex=0;
    while(j < rawSignedAssetAllocationSends.size()){
        string vecTX = "[";
        unsigned int currentTxIndex = i * txPerSender;
        unsigned int nextTxIndex = (i+1) * txPerSender;
        if((nextTxIndex+txPerSender) > rawSignedAssetAllocationSends.size())
            nextTxIndex += txPerSender;
        for(j=currentTxIndex;j< nextTxIndex;j++){
            if(j >= rawSignedAssetAllocationSends.size())
                break;
            if(vecTX != "[")
                vecTX += ",";
            vecTX += "{\"tx\":\"" + rawSignedAssetAllocationSends[j] + "\"}";
        }
        if(vecTX != "["){
            vecTX += "]";
            BOOST_CHECK_NO_THROW(CallExtRPC(senders[senderIndex++], "tpstestadd", "0," + vecTX));
        }
        if(senderIndex >= senders.size())
            senderIndex = 0;
        i++;
    }

    // PHASE 8:  CALL tpstestadd ON ALL SENDER/RECEIVER NODES WITH A FUTURE TIME
    // set the start time to 1 second from now (this needs to be profiled, if the tpstestadd setting time to every node exceeds say 500ms then this time should be extended to account for the latency).
    // rule of thumb if sender count is high (> 25) then profile how long it takes and multiple by 10 and get ceiling of next second needed to send this rpc to every node to have them sync up

    // this will set a start time to every node which will send the vector of signed txs to the network
    int64_t tpstarttime = GetTimeMicros();
    int microsInSecond =  1000 * 1000;
    tpstarttime = tpstarttime + microsInSecond;
    tfm::format(std::cout,"Adding assetsend transactions to queue on sender nodes...\n");
    // ensure mnsync isn't doing its thing before the test starts
    for (auto &sender : senders){
        BOOST_CHECK_NO_THROW(CallRPC(sender, "mnsync next", true, false));
        BOOST_CHECK_NO_THROW(CallRPC(sender, "mnsync next", true, false));
        BOOST_CHECK_NO_THROW(CallRPC(sender, "mnsync next", true, false));
        BOOST_CHECK_NO_THROW(CallRPC(sender, "mnsync next", true, false));
        BOOST_CHECK_NO_THROW(CallRPC(sender, "mnsync next", true, false));
    }
    for (auto &receiver : receivers){
        BOOST_CHECK_NO_THROW(CallRPC(receiver, "mnsync next", true, false));
        BOOST_CHECK_NO_THROW(CallRPC(receiver, "mnsync next", true, false));
        BOOST_CHECK_NO_THROW(CallRPC(receiver, "mnsync next", true, false));
        BOOST_CHECK_NO_THROW(CallRPC(receiver, "mnsync next", true, false));
        BOOST_CHECK_NO_THROW(CallRPC(receiver, "mnsync next", true, false));
    }
    for (auto &sender : senders){
        BOOST_CHECK_NO_THROW(CallExtRPC(sender, "tpstestadd",  i64tostr(tpstarttime)));
        BOOST_CHECK_NO_THROW(r = CallExtRPC(sender, "tpstestinfo"));
        BOOST_CHECK_EQUAL(find_value(r.get_obj(), "testinitiatetime").get_int64(), tpstarttime);
    }
    for (auto &receiver : receivers){
        BOOST_CHECK_NO_THROW(CallExtRPC(receiver, "tpstestadd", i64tostr(tpstarttime)));
        BOOST_CHECK_EQUAL(find_value(r.get_obj(), "testinitiatetime").get_int64(), tpstarttime);
    }

    // PHASE 9:  WAIT 10 SECONDS + DELAY SET ABOVE (1 SECOND)
    tfm::format(std::cout,"Waiting 11 seconds as per protocol...\n");
    // start 11 second wait
    MilliSleep(11000);

    // PHASE 10:  CALL tpstestinfo ON SENDERS AND GET AVERAGE START TIME (TIME SENDERS PUSHED TRANSACTIONS TO THE SOCKETS)
    // get the elapsed time of each node on how long it took to push the vector of signed txs to the network
    int64_t avgteststarttime = 0;
    for (auto &sender : senders) {
        BOOST_CHECK_NO_THROW(r = CallExtRPC(sender, "tpstestinfo"));
        avgteststarttime += find_value(r.get_obj(), "testinitiatetime").get_int64();
    }
    avgteststarttime /= senders.size();

    // PHASE 11:  CALL tpstestinfo ON RECEIVERS AND GET AVERAGE RECEIVE TIME, CALCULATE AVERAGE
    // gather received transfers on the receiver, you can query any receiver node here, in general they all should see the same state after the elapsed time.
    BOOST_CHECK_NO_THROW(r = CallExtRPC(receivers[0], "tpstestinfo"));
    UniValue tpsresponse = r.get_obj();
    UniValue tpsresponsereceivers = find_value(tpsresponse, "receivers").get_array();

    float totalTime = 0;
    for (size_t i = 0; i < tpsresponsereceivers.size(); i++) {
        const UniValue &responseObj = tpsresponsereceivers[i].get_obj();
        totalTime += find_value(responseObj, "time").get_int64() - avgteststarttime;
    }
    // average the start time - received time by the number of responses received (usually number of responses should match number of transactions sent beginning of test)
    totalTime /= tpsresponsereceivers.size();


    // PHASE 12:  DISPLAY RESULTS

    tfm::format(std::cout,"tpstarttime %lld avgteststarttime %lld totaltime %.2f, num responses %zu\n", tpstarttime, avgteststarttime, totalTime, tpsresponsereceivers.size());
    for (auto &sender : senders)
        BOOST_CHECK_NO_THROW(CallExtRPC(sender, "tpstestsetenabled", "false"));
    for (auto &receiver : receivers)
        BOOST_CHECK_NO_THROW(CallExtRPC(receiver, "tpstestsetenabled", "false"));
    int64_t end = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const int64_t &startblock = GetTimeMicros();
    tfm::format(std::cout,"creating %d blocks\n", (numAssets/(93*4)) + 2);
    GenerateBlocks((numAssets/(93*4)) + 2, receivers[0]);

    const int64_t &endblock = GetTimeMicros();
    tfm::format(std::cout,"elapsed time in block creation: %lld\n", endblock-startblock);
    tfm::format(std::cout,"elapsed time in seconds: %lld\n", end-start);
    tfm::format(std::cout,"checking indexes...\n");
    unfoundedAccountIndex = 0;
    for(int i =0;i<numAssets;i++){
        uint32_t nAssetStored;
        ParseUInt32(vecAssets[i], &nAssetStored);
        // send asset to numberOfAssetSendsPerBlock addresses
        string assetAllocationSendMany = "";
        // +1 to account for change output
        for (int j = 0; j < numberOfAssetSendsPerBlock; j++) {
            if(assetAllocationSendMany != "")
                assetAllocationSendMany += ",";
            assetAllocationSendMany += "{\\\"address\\\":\\\"" + unfundedAccounts[unfoundedAccountIndex++] + "\\\",\\\"amount\\\":1}";
            BOOST_CHECK_NO_THROW(r = CallRPC("node1", "listassetindexallocations " + unfundedAccounts[unfoundedAccountIndex-1]));
            UniValue indexArray = r.get_array();
            BOOST_CHECK_EQUAL(indexArray.size(), numAssets);       
            if(unfoundedAccountIndex >= unfundedAccounts.size())
                unfoundedAccountIndex = 0;

        }

        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "listassetindexassets " + vecFundedAddresses[i]));
        UniValue indexArray = r.get_array();
        BOOST_CHECK_EQUAL(indexArray.size(), 1);
        uint32_t nAsset = find_value(indexArray[0].get_obj(), "asset_guid").get_uint();
        BOOST_CHECK_EQUAL(nAsset, nAssetStored);
    }
}*/
BOOST_AUTO_TEST_CASE(generate_assetallocationmint)
{
    UniValue r;
    tfm::format(std::cout,"Running generate_assetallocationmint...\n");  
    // txid 0x80ed016c876a100efea6332ddb9268e1f283e301ba8418be822a7bb93d989e82 on rinkeby
    // remove a0 RLP encoding header because when geth gets the tx root from relayer and pushes to syscoin, it does not include the RLP header
	std::string spv_tx_root = "3a41f9f71e7507776ecdea4d2f1a4819cb316cbc6f3d59db2419dad5bbf39c6d";
	std::string spv_tx_parent_nodes = "f9021bf851a094afbb5157e5d610e4cae7a5075c57dbf2c690f5c4d708dea4d20bb22c15d31380808080808080a067ce91a7124c12e50858eef169ccb55469fa6fb9e5cdc77146db32c0f6d6239f8080808080808080f87180a0d685cfa5def41a511cabed701a6aa2243b12269d1c8aa1c55034f72dba1cf811a05ad5fe4ff120b0a86ef0924aedf369bfdf6f94564c486eb3ad37b309e9ddf7d9a05c8b2438eedcd4e70e17a257979ab655ea8c7680393fd51da45f4e4cb99f9fa780808080808080808080808080f9015220b9014ef9014b820276843b9aca008307a120949afFaffFF00b3285E1a3337122Bd2dF2c4c9D62d80b8e45f959b690000000000000000000000000000000000000000000000015af1d78b58c40000000000000000000000000000000000000000000000000000000000003acaeec0000000000000000000000000fe234d3994f95bf7cebd9837c4444f5af63f0a97000000000000000000000000000000000000000000000000000000000000001200000000000000000000000000000000000000000000000000000000000000a0000000000000000000000000000000000000000000000000000000000000001500e37ddd289ccd1fb130a91210644810b2415aec4000000000000000000000002ba09c6a7a1f762f120bba618c6184a3bdc4f09f741ddec33b901bee1b8e40bb6a9da05a30b17b585bfb8ba159673d7df3732e6e1810dd1ba01295e8a9c9cc6380b25e";
	std::string spv_tx_value = "f9014b820276843b9aca008307a120949afFaffFF00b3285E1a3337122Bd2dF2c4c9D62d80b8e45f959b690000000000000000000000000000000000000000000000015af1d78b58c40000000000000000000000000000000000000000000000000000000000003acaeec0000000000000000000000000fe234d3994f95bf7cebd9837c4444f5af63f0a97000000000000000000000000000000000000000000000000000000000000001200000000000000000000000000000000000000000000000000000000000000a0000000000000000000000000000000000000000000000000000000000000001500e37ddd289ccd1fb130a91210644810b2415aec4000000000000000000000002ba09c6a7a1f762f120bba618c6184a3bdc4f09f741ddec33b901bee1b8e40bb6a9da05a30b17b585bfb8ba159673d7df3732e6e1810dd1ba01295e8a9c9cc6380b25e";
	std::string spv_tx_path = "02";
    
    // remove a0 RLP header from spv_receipt_root as well
    std::string spv_receipt_root = "f64d97b74a72506e825505ecfacd678c70194a7ba54729ead2834d6fa993d43f";
    std::string spv_receipt_parent_nodes = "f90390f851a0454b5b5501e441e1e816f1cf79759d0b4e3a662ac21dd85543ecfb9edcaba46380808080808080a032f8a6872a1490a037d170f019bf25df0fa86ea2602ad8c56a2ec22485cd84818080808080808080f87180a0c24a67f9eaa382328aeb6b543060050e7912b7ed75e4e301e2bc1985c83a02caa037a0e4bf073268ce91e1b12d9b9dc19847369512c7b2ee7a313923f560b051f0a057748f4d06331e027d83a83cc2e671bec1f586f8c53e1418d7d81cd54a9d1a6b80808080808080808080808080f902c720b902c3f902c001830ad9d7b9010000000000000000000000000020000000000000000000000000000000000100000000000000000000000000800000000000000000000000000000000000200000000000000000000000000008000000000000000000000000300000000000000000000000000000000000200000000080000000200000000200000010000000000000000000000000000000000000000000000000000010000000001000000000020000000000000000000000000000000000000000008100000000000000000000000002000000000000000000000000000000008000000000000000000000000010000000000000000000000000000000000000000000002000000000000000f901b5f89b94fe234d3994f95bf7cebd9837c4444f5af63f0a97f863a0ddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3efa0000000000000000000000000b0ea8c9ee8aa87efd28a12de8c034f947c144053a00000000000000000000000009afFaffFF00b3285E1a3337122Bd2dF2c4c9D62da00000000000000000000000000000000000000000000000015af1d78b58c40000f89b94fe234d3994f95bf7cebd9837c4444f5af63f0a97f863a08c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925a0000000000000000000000000b0ea8c9ee8aa87efd28a12de8c034f947c144053a00000000000000000000000009afFaffFF00b3285E1a3337122Bd2dF2c4c9D62da00000000000000000000000000000000000000000000000000000000000000000f879949afFaffFF00b3285E1a3337122Bd2dF2c4c9D62de1a07def9f73ac6a7ac8fe4dc55f5257d48aed7e3f9d5247b0020598b87a5c369d82b840000000000000000000000000b0ea8c9ee8aa87efd28a12de8c034f947c1440530000000000000000000000000000000000000000000000015af1d78b58c40000";
    std::string spv_receipt_value = "f902c001830ad9d7b9010000000000000000000000000020000000000000000000000000000000000100000000000000000000000000800000000000000000000000000000000000200000000000000000000000000008000000000000000000000000300000000000000000000000000000000000200000000080000000200000000200000010000000000000000000000000000000000000000000000000000010000000001000000000020000000000000000000000000000000000000000008100000000000000000000000002000000000000000000000000000000008000000000000000000000000010000000000000000000000000000000000000000000002000000000000000f901b5f89b94fe234d3994f95bf7cebd9837c4444f5af63f0a97f863a0ddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3efa0000000000000000000000000b0ea8c9ee8aa87efd28a12de8c034f947c144053a00000000000000000000000009afFaffFF00b3285E1a3337122Bd2dF2c4c9D62da00000000000000000000000000000000000000000000000015af1d78b58c40000f89b94fe234d3994f95bf7cebd9837c4444f5af63f0a97f863a08c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925a0000000000000000000000000b0ea8c9ee8aa87efd28a12de8c034f947c144053a00000000000000000000000009afFaffFF00b3285E1a3337122Bd2dF2c4c9D62da00000000000000000000000000000000000000000000000000000000000000000f879949afFaffFF00b3285E1a3337122Bd2dF2c4c9D62de1a07def9f73ac6a7ac8fe4dc55f5257d48aed7e3f9d5247b0020598b87a5c369d82b840000000000000000000000000b0ea8c9ee8aa87efd28a12de8c034f947c1440530000000000000000000000000000000000000000000000015af1d78b58c40000";
    
    int height = 4953001;
    string newaddress = GetNewFundedAddress("node1");
    string amount = "25";
    string assetguid = AssetNew("node1", newaddress, "pubdata", "0xfe234d3994f95bf7cebd9837c4444f5af63f0a97");
    AssetAllocationMint("node1", assetguid, newaddress, amount, height, spv_tx_value, spv_tx_root, spv_tx_parent_nodes, spv_tx_path, spv_receipt_value, spv_receipt_root, spv_receipt_parent_nodes);
    
    // try to mint again
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationmint " + assetguid + " " + newaddress + " " + amount + " " + itostr(height) + " " + spv_tx_value + " a0" + spv_tx_root + " " + spv_tx_parent_nodes + " " + spv_tx_path + " " + spv_receipt_value + " a0" + spv_receipt_root + " " + spv_receipt_parent_nodes +  " ''"));
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string hex_str = find_value(r.get_obj(), "hex").get_str();
    // should fail: already minted with that block+txindex tuple
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hex_str, true, false)); 
    BOOST_CHECK(r.write().size() < 32);

    // increase time by 1 week and assetallocationmint should throw
	SleepFor(604800 * 1000);
	BOOST_CHECK_THROW(r = CallRPC("node1", "assetallocationmint " + assetguid + " " + newaddress + " " + amount + " " + itostr(height) + " " + spv_tx_value + " a0" + spv_tx_root + " " + spv_tx_parent_nodes + " " + spv_tx_path + " " + spv_receipt_value + " a0" + spv_receipt_root + " " + spv_receipt_parent_nodes +  " ''"), runtime_error);
}
BOOST_AUTO_TEST_CASE(generate_burn_syscoin_asset)
{
    UniValue r;
    tfm::format(std::cout,"Running generate_burn_syscoin_asset...\n");
    GenerateBlocks(5);
    GenerateBlocks(5, "node2");
    GenerateBlocks(5, "node3");

    string creatoraddress = GetNewFundedAddress("node1");
    string useraddress = GetNewFundedAddress("node1");

    string assetguid = AssetNew("node1", creatoraddress, "pubdata", "0x931d387731bbbc988b312206c74f77d004d6b84b");

    AssetSend("node1", assetguid, "\"[{\\\"address\\\":\\\"" + useraddress + "\\\",\\\"amount\\\":0.5}]\"");
    // try to burn more than we own
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationburn " + assetguid + " " + useraddress + " 0.6 0x931d387731bbbc988b312206c74f77d004d6b84b"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string hexStr = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
    BOOST_CHECK(r.write().size() < 32);
    // this one is ok
    BurnAssetAllocation("node1", assetguid, useraddress, "0.5");
    
    // because allocation is empty it should have been erased
    BOOST_CHECK_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress), runtime_error);
    // make sure you can't move coins from burn recipient
    BOOST_CHECK_THROW(r = CallRPC("node1", "assetallocationsendmany " + assetguid + " burn " + "\"[{\\\"address\\\":\\\"" + useraddress + "\\\",\\\"amount\\\":0.5}]\"" + " ''"), runtime_error);


}
BOOST_AUTO_TEST_CASE(generate_burn_syscoin_asset_multiple)
{
    UniValue r;
    tfm::format(std::cout,"Running generate_burn_syscoin_asset_multiple...\n");
    GenerateBlocks(5);
    GenerateBlocks(5, "node2");
    GenerateBlocks(5, "node3");

    string creatoraddress = GetNewFundedAddress("node1");
    string useraddress = GetNewFundedAddress("node1");
    CallExtRPC("node2", "sendtoaddress" , "\"" + useraddress + "\",\"1\"", false);
    CallExtRPC("node2", "sendtoaddress" , "\"" + useraddress + "\",\"1\"", false);
    GenerateBlocks(5, "node1");
    GenerateBlocks(5, "node2");

    string assetguid = AssetNew("node1", creatoraddress, "pubdata", "0xc47bD54a3Df2273426829a7928C3526BF8F7Acaa");

    AssetSend("node1", assetguid, "\"[{\\\"address\\\":\\\"" + useraddress + "\\\",\\\"amount\\\":1.0}]\"");

    // 2 options for burns, all good, 1 good 1 bad
    // all good, burn 0.4 + 0.5 + 0.05

    BurnAssetAllocation("node1", assetguid, useraddress, "0.4", false);
    BurnAssetAllocation("node1", assetguid, useraddress, "0.5", false);
    BurnAssetAllocation("node1", assetguid, useraddress, "0.05", false);
    GenerateBlocks(5, "node1");

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress ));
    UniValue balance2 = find_value(r.get_obj(), "balance");
    BOOST_CHECK_EQUAL(balance2.getValStr(), "0.05000000");


    assetguid = AssetNew("node1", creatoraddress, "pubdata", "0xc47bD54a3Df2273426829a7928C3526BF8F7Acaa");

    AssetSend("node1", assetguid, "\"[{\\\"address\\\":\\\"" + useraddress + "\\\",\\\"amount\\\":1.0}]\"");
    // 1 bad 1 good, burn 0.6+0.6 only 1 should go through

    BurnAssetAllocation("node1", assetguid, useraddress, "0.6", false);
    // try burn more than we own
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationburn " + assetguid + " " + useraddress + " 0.6 0x931d387731bbbc988b312206c74f77d004d6b84b"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string hexStr = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
     // this is a double spend attempt between the two burns so it will relay it and first one that gets to miner wins
    BOOST_CHECK(r.write().size() >= 32);

    // this will stop the chain if both burns were allowed in the chain, the miner must throw away one of the burns to avoid his block from being flagged as invalid
    GenerateBlocks(5, "node1");
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress ));
    balance2 = find_value(r.get_obj(), "balance");
    BOOST_CHECK_EQUAL(balance2.getValStr(), "0.40000000");


}
BOOST_AUTO_TEST_CASE(generate_burn_syscoin_asset_zdag_within_block)
{
    UniValue r;
    tfm::format(std::cout,"Running generate_burn_syscoin_asset_zdag_within_block...\n");
    GenerateBlocks(5);
    GenerateBlocks(5, "node2");
    GenerateBlocks(5, "node3");

    string creatoraddress = GetNewFundedAddress("node1");

    string useraddress2 = GetNewFundedAddress("node2");
    string useraddress1 = GetNewFundedAddress("node1");


    GenerateBlocks(5, "node1");
    GenerateBlocks(5, "node2");

    string assetguid = AssetNew("node1", creatoraddress, "pubdata", "0xc47bD54a3Df2273426829a7928C3526BF8F7Acaa", "8", "10", "100");
    AssetSend("node1", assetguid, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":2.0}]\"");
    // From W2 send 1 SPT to W1 - Wait for Confirm
    AssetAllocationTransfer(false, "node2", assetguid, useraddress2, "\"[{\\\"address\\\":\\\"" + useraddress1 + "\\\",\\\"amount\\\":1.0}]\"");

    // From W2 send 1 SPT to W1 - DO NOT WAIT FOR CONFIRM
    AssetAllocationTransfer(true, "node2", assetguid, useraddress2, "\"[{\\\"address\\\":\\\"" + useraddress1 + "\\\",\\\"amount\\\":1.0}]\"");
    MilliSleep(500);
    // From W1 send 2 SPT to W2 within same Block
    AssetAllocationTransfer(true, "node1", assetguid, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":2.0}]\"");

    GenerateBlocks(5, "node1");
    // useraddress1 should be empty
    BOOST_CHECK_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress1), runtime_error);

    // useraddress2 should have 2
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress2));
    UniValue balance = find_value(r.get_obj(), "balance");
    BOOST_CHECK_EQUAL(balance.getValStr(), "2.00000000");

    // useraddress1 should be empty
    BOOST_CHECK_THROW(r = CallRPC("node2", "assetallocationinfo " + assetguid + " " + useraddress1), runtime_error);

    // useraddress2 should have 2
    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationinfo " + assetguid + " " + useraddress2));
    balance = find_value(r.get_obj(), "balance");
    BOOST_CHECK_EQUAL(balance.getValStr(), "2.00000000");

    // useraddress1 should be empty
    BOOST_CHECK_THROW(r = CallRPC("node3", "assetallocationinfo " + assetguid + " " + useraddress1), runtime_error);

    // useraddress2 should have 2
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationinfo " + assetguid + " " + useraddress2));
    balance = find_value(r.get_obj(), "balance");
    BOOST_CHECK_EQUAL(balance.getValStr(), "2.00000000");    
}
// a = 1, a->b(0.4), a->c(0.2), burn a(0.4) (a=0, b=0.4, c=0.2 and burn=0.4)
BOOST_AUTO_TEST_CASE(generate_burn_syscoin_asset_zdag)
{
    UniValue r;
    tfm::format(std::cout,"Running generate_burn_syscoin_asset_zdag...\n");
    GenerateBlocks(5);
    GenerateBlocks(5, "node2");
    GenerateBlocks(5, "node3");

    string creatoraddress = GetNewFundedAddress("node1");

    string useraddress2 = GetNewFundedAddress("node1");
    string useraddress3 = GetNewFundedAddress("node1");
    string useraddress1 = GetNewFundedAddress("node1");
    CallExtRPC("node2", "sendtoaddress" , "\"" + useraddress1 + "\",\"1\"", false);
    CallExtRPC("node2", "sendtoaddress" , "\"" + useraddress1 + "\",\"1\"", false);


    GenerateBlocks(5, "node1");
    GenerateBlocks(5, "node2");

    string assetguid = AssetNew("node1", creatoraddress, "pubdata", "0xc47bD54a3Df2273426829a7928C3526BF8F7Acaa");

    AssetSend("node1", assetguid, "\"[{\\\"address\\\":\\\"" + useraddress1 + "\\\",\\\"amount\\\":1.0}]\"");

    AssetAllocationTransfer(true, "node1", assetguid, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.4}]\"");
    AssetAllocationTransfer(true, "node1", assetguid, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress3 + "\\\",\\\"amount\\\":0.2}]\"");
    BurnAssetAllocation("node1", assetguid, useraddress1, "0.4", false);

    GenerateBlocks(5, "node1");

    BOOST_CHECK_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress1), runtime_error);

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress2));
    UniValue balance = find_value(r.get_obj(), "balance");
    BOOST_CHECK_EQUAL(balance.getValStr(), "0.40000000");

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress3));
    balance = find_value(r.get_obj(), "balance");
    BOOST_CHECK_EQUAL(balance.getValStr(), "0.20000000");

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " burn"));
    balance = find_value(r.get_obj(), "balance");
    BOOST_CHECK_EQUAL(balance.getValStr(), "0.40000000");
}
// a = 1, burn a(0.8) a->b (0.4), a->c(0.2) (a=0.4, b=0.4, c=0.2 and burn=0)
BOOST_AUTO_TEST_CASE(generate_burn_syscoin_asset_zdag1)
{
    UniValue r;
    tfm::format(std::cout,"Running generate_burn_syscoin_asset_zdag1...\n");
    GenerateBlocks(5);
    GenerateBlocks(5, "node2");
    GenerateBlocks(5, "node3");

    string creatoraddress = GetNewFundedAddress("node1");

    string useraddress2 = GetNewFundedAddress("node1");
    string useraddress3 = GetNewFundedAddress("node1");
    string useraddress1 = GetNewFundedAddress("node1");
    CallExtRPC("node2", "sendtoaddress" , "\"" + useraddress1 + "\",\"1\"", false);
    CallExtRPC("node2", "sendtoaddress" , "\"" + useraddress1 + "\",\"1\"", false);


    GenerateBlocks(5, "node1");
    GenerateBlocks(5, "node2");

    string assetguid = AssetNew("node1", creatoraddress, "pubdata", "0xc47bD54a3Df2273426829a7928C3526BF8F7Acaa", "8", "5");

    AssetSend("node1", assetguid, "\"[{\\\"address\\\":\\\"" + useraddress1 + "\\\",\\\"amount\\\":1.0},{\\\"address\\\":\\\"" + useraddress3 + "\\\",\\\"amount\\\":0.2}]\"");

    string burntxid = BurnAssetAllocation("node1", assetguid, useraddress1, "0.8", false);
    // try xfer more than we own, prev output link in mempool ensures we classify this as sender conflict and potential dbl spend
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsendmany " + assetguid + " " + useraddress1 + " " + "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.4}]\"" + " ''"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string hexStr = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "decoderawtransaction " + hexStr));
    string txid1 = find_value(r.get_obj(), "txid").get_str();
    BOOST_CHECK(r.write().size() >= 32);
    

    string txid2 = AssetAllocationTransfer(true, "node1", assetguid, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress3 + "\\\",\\\"amount\\\":0.2}]\"");
    string txid3 = AssetAllocationTransfer(true, "node1", assetguid, useraddress3, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.2}]\"");
    GenerateBlocks(5, "node1");
    // should be balance 0 and thus deleted
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress1));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress2));
    UniValue balance = find_value(r.get_obj(), "balance_zdag");
    BOOST_CHECK_EQUAL(balance.getValStr(), "0.20000000");

    BOOST_CHECK_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress3), runtime_error);

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " burn"));
    balance = find_value(r.get_obj(), "balance_zdag");
    BOOST_CHECK_EQUAL(balance.getValStr(), "0.80000000");


    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + burntxid));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);
    // txid1 and txid2 should still be in mempool but will be conflicting
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + txid1));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + txid2));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + txid3));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);
}
// a = 1, b = 0.2, c = 0.1, a->b (0.2), b->a(0.2),  a->c(0.2), c->a(0.2), burn a(0.5), burn a(0.5), b->c(0.2), burn c(0.3) (a=0, b=0, c=0 and burn=1.3)
BOOST_AUTO_TEST_CASE(generate_burn_syscoin_asset_zdag2)
{
    UniValue r;
    tfm::format(std::cout,"Running generate_burn_syscoin_asset_zdag2...\n");
    GenerateBlocks(5);
    GenerateBlocks(5, "node2");
    GenerateBlocks(5, "node3");

    string creatoraddress = GetNewFundedAddress("node1");
    string useraddress1 = GetNewFundedAddress("node1");
    string useraddress2 = GetNewFundedAddress("node1");
    string useraddress3 = GetNewFundedAddress("node1");
    CallExtRPC("node2", "sendtoaddress" , "\"" + useraddress2 + "\",\"1\"", false);
    CallExtRPC("node2", "sendtoaddress" , "\"" + useraddress1 + "\",\"1\"", false);
    CallExtRPC("node2", "sendtoaddress" , "\"" + useraddress1 + "\",\"1\"", false);
    CallExtRPC("node2", "sendtoaddress" , "\"" + useraddress1 + "\",\"1\"", false);
    CallExtRPC("node2", "sendtoaddress" , "\"" + useraddress1 + "\",\"1\"", false);
    CallExtRPC("node2", "sendtoaddress" , "\"" + useraddress1 + "\",\"1\"", false);
    CallExtRPC("node2", "sendtoaddress" , "\"" + useraddress3 + "\",\"1\"", false);
    GenerateBlocks(5, "node1");
    GenerateBlocks(5, "node2");
    GenerateBlocks(5, "node3");

    string assetguid = AssetNew("node1", creatoraddress, "pubdata", "0xc47bD54a3Df2273426829a7928C3526BF8F7Acaa", "8", "3");

    AssetSend("node1", assetguid, "\"[{\\\"address\\\":\\\"" + useraddress1 + "\\\",\\\"amount\\\":1.0},{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.2},{\\\"address\\\":\\\"" + useraddress3 + "\\\",\\\"amount\\\":0.1}]\"");
   
    AssetAllocationTransfer(true, "node1", assetguid, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.2}]\"");


    AssetAllocationTransfer(true, "node1", assetguid, useraddress2, "\"[{\\\"address\\\":\\\"" + useraddress1 + "\\\",\\\"amount\\\":0.2}]\"");
  
    AssetAllocationTransfer(true, "node1", assetguid, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress3 + "\\\",\\\"amount\\\":0.2}]\"");

    AssetAllocationTransfer(true, "node1", assetguid, useraddress3, "\"[{\\\"address\\\":\\\"" + useraddress1 + "\\\",\\\"amount\\\":0.2}]\"");
   
    BurnAssetAllocation("node1", assetguid, useraddress1, "0.5", false);
    BurnAssetAllocation("node1", assetguid, useraddress1, "0.5", false);

    // try to burn more than you own
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationburn " + assetguid + " " + useraddress1 + " 0.2 0x931d387731bbbc988b312206c74f77d004d6b84b"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string hexStr = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
    // this is a double spend attempt between the two burns so it will relay it and first one that gets to miner wins
    BOOST_CHECK(r.write().size() >= 32);
    
    AssetAllocationTransfer(true, "node1", assetguid, useraddress2, "\"[{\\\"address\\\":\\\"" + useraddress3 + "\\\",\\\"amount\\\":0.2}]\"");
    BurnAssetAllocation("node1", assetguid, useraddress3, "0.3", false);

    GenerateBlocks(5, "node1");

    BOOST_CHECK_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress1), runtime_error);

    BOOST_CHECK_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress2), runtime_error);

    BOOST_CHECK_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress3), runtime_error);

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " burn"));
    UniValue balance = find_value(r.get_obj(), "balance");
    BOOST_CHECK_EQUAL(balance.getValStr(), "1.30000000");
}

BOOST_AUTO_TEST_CASE(generate_burn_syscoin_asset_zdag3)
{
    UniValue r;
    tfm::format(std::cout,"Running generate_burn_syscoin_asset_zdag3...\n");
    GenerateBlocks(5);
    GenerateBlocks(5, "node2");
    GenerateBlocks(5, "node3");

    string creatoraddress = GetNewFundedAddress("node1");

    string useraddress1 = GetNewFundedAddress("node1");
    string useraddress2 = GetNewFundedAddress("node1");
    string useraddress3 = GetNewFundedAddress("node1");
    CallExtRPC("node1", "sendtoaddress" , "\"" + useraddress1 + "\",\"1\"", false);
    CallExtRPC("node1", "sendtoaddress" , "\"" + useraddress1 + "\",\"1\"", false);
    CallExtRPC("node1", "sendtoaddress" , "\"" + creatoraddress + "\",\"1\"", false);
    GenerateBlocks(5, "node1");
       
    string assetguid = AssetNew("node1", creatoraddress, "pubdata", "0xc47bD54a3Df2273426829a7928C3526BF8F7Acaa");
    
    AssetSend("node1", assetguid, "\"[{\\\"address\\\":\\\"" + useraddress1 + "\\\",\\\"amount\\\":1.0}]\"");
    AssetAllocationTransfer(false, "node1", assetguid, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.1}]\"");
    
    // try to burn more than you own
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationburn " + assetguid + " " + useraddress2 + " 0.4 0x931d387731bbbc988b312206c74f77d004d6b84b"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string hexStr = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
    BOOST_CHECK(r.write().size() < 32);

    string txid1 = AssetAllocationTransfer(true, "node1", assetguid, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.1}]\"");
    string txid2 = AssetAllocationTransfer(true, "node1", assetguid, useraddress2, "\"[{\\\"address\\\":\\\"" + useraddress3 + "\\\",\\\"amount\\\":0.1}]\"");
    

    GenerateBlocks(5, "node1");
    
    // no zdag tx found after block
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + txid1));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + txid2));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);
    
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress1));
    UniValue balance = find_value(r.get_obj(), "balance");
    BOOST_CHECK_EQUAL(balance.getValStr(), "0.80000000"); 

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress2));
    balance = find_value(r.get_obj(), "balance");
    BOOST_CHECK_EQUAL(balance.getValStr(), "0.10000000");

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress3));
    balance = find_value(r.get_obj(), "balance");
    BOOST_CHECK_EQUAL(balance.getValStr(), "0.10000000");
    
    AssetAllocationTransfer(true, "node1", assetguid, useraddress2, "\"[{\\\"address\\\":\\\"" + useraddress1 + "\\\",\\\"amount\\\":0.1}]\"");
    
    BOOST_CHECK_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " burn"), runtime_error);
   

    // now do more zdag and check status are ok this time
    txid1 = AssetAllocationTransfer(true, "node1", assetguid, useraddress3, "\"[{\\\"address\\\":\\\"" + useraddress1 + "\\\",\\\"amount\\\":0.1}]\"");
    txid2 = AssetAllocationTransfer(true, "node1", assetguid, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.05}]\"");

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + txid1));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + txid2));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);
    
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress1));
    balance = find_value(r.get_obj(), "balance_zdag");
    BOOST_CHECK_EQUAL(balance.getValStr(), "0.95000000");

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress2));
    balance = find_value(r.get_obj(), "balance_zdag");
    BOOST_CHECK_EQUAL(balance.getValStr(), "0.05000000");

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress3));
    balance = find_value(r.get_obj(), "balance_zdag");
    BOOST_CHECK_EQUAL(balance.getValStr(), "0.00000000");
    
    GenerateBlocks(5, "node1");
    
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress1));
    balance = find_value(r.get_obj(), "balance");
    BOOST_CHECK_EQUAL(balance.getValStr(), "0.95000000"); 

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress2));
    balance = find_value(r.get_obj(), "balance");
    BOOST_CHECK_EQUAL(balance.getValStr(), "0.05000000");

    BOOST_CHECK_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress3), runtime_error);
}
// try a dbl spend with zdag
BOOST_AUTO_TEST_CASE(generate_burn_syscoin_asset_zdag4)
{
    UniValue r;
    tfm::format(std::cout,"Running generate_burn_syscoin_asset_zdag4...\n");
    GenerateBlocks(5);
    GenerateBlocks(5, "node2");
    GenerateBlocks(5, "node3");

    string creatoraddress = GetNewFundedAddress("node1");

    string useraddress1 = GetNewFundedAddress("node1");
    string useraddress2 = GetNewFundedAddress("node1");
    string useraddress3 = GetNewFundedAddress("node1");
    CallExtRPC("node1", "sendtoaddress" , "\"" + useraddress1 + "\",\"1\"", false);
    CallExtRPC("node1", "sendtoaddress" , "\"" + useraddress1 + "\",\"1\"", false);
    CallExtRPC("node1", "sendtoaddress" , "\"" + creatoraddress + "\",\"1\"", false);
    GenerateBlocks(5, "node1");
       
    string assetguid = AssetNew("node1", creatoraddress, "pubdata", "0xc47bD54a3Df2273426829a7928C3526BF8F7Acaa");
    
    AssetSend("node1", assetguid, "\"[{\\\"address\\\":\\\"" + useraddress1 + "\\\",\\\"amount\\\":1.0}]\"");
    AssetAllocationTransfer(false, "node1", assetguid, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.1}]\"");
    // burn and transfer at same time for dbl spend attempt
    // burn
 
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationburn " + assetguid + " " + useraddress1 + " 0.8 0x931d387731bbbc988b312206c74f77d004d6b84b"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string burnHex = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "decoderawtransaction " + burnHex));
    string burntxid = find_value(r.get_obj(), "txid").get_str();
    // asset xfer
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsendmany " + assetguid + " " + useraddress1 + " \"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.8}]\"" + " ''"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string assetHex = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "decoderawtransaction " + assetHex));
    string assettxid = find_value(r.get_obj(), "txid").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "sendrawtransaction " + burnHex, true, false));
    MilliSleep(500);
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "sendrawtransaction " + assetHex, true, false));
    MilliSleep(500);
    string txid1 = AssetAllocationTransfer(true, "node1", assetguid, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.1}]\"");
    string txid2 = AssetAllocationTransfer(true, "node1", assetguid, useraddress2, "\"[{\\\"address\\\":\\\"" + useraddress1 + "\\\",\\\"amount\\\":0.05}]\"");
    MilliSleep(500);
    // txid1 depends on double spend so it is conflicted
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + txid1));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);
    // because txid2 depends on txid1 which depends on a double spend, it is also conflicted
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + txid2));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);
    
   
    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationverifyzdag " + txid1));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);

    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationverifyzdag " + txid2));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);

    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationverifyzdag " + txid1));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);

    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationverifyzdag " + txid2));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);
    // the burn txid should be received on all nodes and it should be in conflict
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + burntxid));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);
    // likewise with the asset txid
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + assettxid));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);


    // the burn txid should be received on all nodes and it should be in conflict
    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationverifyzdag " + burntxid));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);
    // likewise with the asset txid
    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationverifyzdag " + assettxid));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);


    // the burn txid should be received on all nodes and it should be in conflict
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationverifyzdag " + burntxid));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);
    // likewise with the asset txid
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationverifyzdag " + assettxid));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);
    GenerateBlocks(5);
    // asset xfer on node3 ensure it doesn't link against previous tx
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationsendmany " + assetguid + " " + useraddress1 + " \"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.01}]\"" + " ''"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    assetHex = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "decoderawtransaction " + assetHex));
    string assettxid2 = find_value(r.get_obj(), "txid").get_str();
    // send on node2 where it would have had a conflicting output for this sender since it double spent
    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "sendrawtransaction " + assetHex, true, false));
    MilliSleep(100);
    // status should be conflicting because sender conflict status hasn't cleared even after a block should be invalid-sender during the graph check
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + assettxid2));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);
    
    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationverifyzdag " + assettxid2));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);

    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationverifyzdag " + assettxid2));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);
  
    tfm::format(std::cout,"Setting time ahead 300 seconds...\n");
    SleepFor(300 * 1000, 0);
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress1));
    UniValue balance = find_value(r.get_obj(), "balance");
    BOOST_CHECK_EQUAL(balance.getValStr(), "0.05000000");
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " " + useraddress2));
    balance = find_value(r.get_obj(), "balance");
    // depending on which one arrived first, the above 0.05 balance is good because that means that at least 1 zdag transaction went through
    BOOST_CHECK(balance.getValStr() == "0.95000000" || balance.getValStr() == "0.15000000");
    // if the send went through then burn should be invalid
    if(balance.getValStr() == "0.95000000"){
        BOOST_CHECK_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " burn"), runtime_error);
    }
    // if the send didn't go through the burn should be valid
    else if (balance.getValStr() == "0.15000000") {
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + assetguid + " burn"));
        balance = find_value(r.get_obj(), "balance");
        BOOST_CHECK_EQUAL(balance.getValStr(), "0.80000000");
    }
    // no zdag tx found after block
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + txid1));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + txid2));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);

    // asset xfer on node3 ensure it doesn't link against previous tx but this time should be ok
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationsendmany " + assetguid + " " + useraddress1 + " \"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.01}]\"" + " ''"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    assetHex = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "decoderawtransaction " + assetHex));
    string assettxid3 = find_value(r.get_obj(), "txid").get_str();
    // send on node2 where it would have had a conflicting output for this sender since it double spent
    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "sendrawtransaction " + assetHex, true, false));
    MilliSleep(500);
    // status should be ok since timeout should clear output linking
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + assettxid3));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);
    
    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationverifyzdag " + assettxid3));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);

    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationverifyzdag " + assettxid3));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);


}
BOOST_AUTO_TEST_CASE(generate_burn_syscoin_asset_zdag_dbl_spend_same_input)
{
    UniValue r;
    tfm::format(std::cout,"Running generate_burn_syscoin_asset_zdag_dbl_spend_same_input...\n");
    GenerateBlocks(5);
    GenerateBlocks(5, "node2");
    GenerateBlocks(5, "node3");

    string creatoraddress = GetNewFundedAddress("node1");

    string useraddress1 = GetNewFundedAddress("node1");
    string useraddress2 = GetNewFundedAddress("node1");
    string useraddress3 = GetNewFundedAddress("node1");
    CallExtRPC("node1", "sendtoaddress" , "\"" + useraddress1 + "\",\"1\"", false);
    CallExtRPC("node1", "sendtoaddress" , "\"" + useraddress1 + "\",\"1\"", false);
    CallExtRPC("node1", "sendtoaddress" , "\"" + creatoraddress + "\",\"1\"", false);
    GenerateBlocks(5, "node1");
       
    string assetguid = AssetNew("node1", creatoraddress, "pubdata", "0xc47bD54a3Df2273426829a7928C3526BF8F7Acaa");
    
    AssetSend("node1", assetguid, "\"[{\\\"address\\\":\\\"" + useraddress1 + "\\\",\\\"amount\\\":1.0}]\"");
    AssetAllocationTransfer(false, "node1", assetguid, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.1}]\"");
    // burn and transfer at same time for dbl spend attempt
    // burn
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationburn " + assetguid + " " + useraddress1 + " 0.8 0x931d387731bbbc988b312206c74f77d004d6b84b"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string burnHex = find_value(r.get_obj(), "hex").get_str();

    // asset xfer
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsendmany " + assetguid + " " + useraddress1 + " \"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.8}]\"" + " ''"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string assetHex = find_value(r.get_obj(), "hex").get_str();

    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "sendrawtransaction " + assetHex, true, false));
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "decoderawtransaction " + assetHex + " true"));
	string txid1 = find_value(r.get_obj(), "txid").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "sendrawtransaction " + burnHex, true, false));

	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "decoderawtransaction " + burnHex + " true"));
	string txid2 = find_value(r.get_obj(), "txid").get_str();
    
    MilliSleep(500);
    // check just sender, burn marks as major issue on zdag
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + txid1));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);
    
    // check just sender, burn marks as major issue on zdag
    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationverifyzdag " + txid2));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);

    // check just sender, burn marks as major issue on zdag
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationverifyzdag " + txid1));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);
    GenerateBlocks(5);
} 
BOOST_AUTO_TEST_CASE(generate_burn_syscoin_asset_zdag_dbl_spend_long_chain)
{
    UniValue r;
    tfm::format(std::cout,"Running generate_burn_syscoin_asset_zdag_dbl_spend_long_chain...\n");
    GenerateBlocks(5);
    GenerateBlocks(5, "node2");
    GenerateBlocks(5, "node3");

    string creatoraddress = GetNewFundedAddress("node1");

    string useraddress1 = GetNewFundedAddress("node1");
    string useraddress2 = GetNewFundedAddress("node1");
    string useraddress3 = GetNewFundedAddress("node1");
    GenerateBlocks(5, "node1");
    string assetguid = AssetNew("node1", useraddress3, "pubdata", "0x931d387731bbbc988b312206c74f77d004d6b84b");

    SyscoinBurn("node1", useraddress1, strSYSXAsset, "1");
    SyscoinBurn("node1", useraddress2, strSYSXAsset, "1");
    string tx1 = AssetAllocationTransfer(true, "node1", strSYSXAsset, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.1}]\"");
    string tx2 = AssetAllocationTransfer(true, "node1", strSYSXAsset, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.01}]\"");
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx1, tx2));
    string tx3 = AssetAllocationTransfer(true, "node1", strSYSXAsset, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.001}]\"");
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx2, tx3));
    string tx4 = AssetAllocationTransfer(true, "node1", strSYSXAsset, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.0001}]\"");
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx3, tx4));
    string tx5 = AssetAllocationTransfer(true, "node1", strSYSXAsset, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.00001}]\"");
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx4, tx5));
    string tx6 = AssetAllocationTransfer(true, "node1", strSYSXAsset, useraddress1, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.000001}]\"");
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx5, tx6));
    string tx7 = BurnAssetAllocation("node1", strSYSXAsset, useraddress1, "0.01", false, "''");
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx6, tx7));
    string tx8 = BurnAssetAllocation("node1", strSYSXAsset, useraddress2, "0.01", false, "''");
    BOOST_CHECK(!AreTwoTransactionsLinked("node1", tx6, tx8));
    string tx9 = BurnAssetAllocation("node1", strSYSXAsset, useraddress1, "0.001", false, "''");
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx7, tx9));
    string tx10 = BurnAssetAllocation("node1", strSYSXAsset, useraddress2, "0.001", false, "''");
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx8, tx10));
    string tx11 = BurnAssetAllocation("node1", strSYSXAsset, useraddress2, "0.0001", false, "''");
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx10, tx11));
    string tx12 = BurnAssetAllocation("node1", strSYSXAsset, useraddress1, "0.0001", false, "''");
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx9, tx12));
    string tx13 = SyscoinBurn("node1", useraddress1, strSYSXAsset, "1", false);
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx12, tx13));
    string tx14 = AssetUpdate("node1", strSYSXAsset , "''", "''", "''", "''", "''", false);
    string tx15 = SyscoinBurn("node1", useraddress2, strSYSXAsset, "1", false);
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx11, tx15)); // last one in sequence for useraddress2
    string tx16 = AssetUpdate("node1", strSYSXAsset, "''", "''", "''", "''", "''", false);
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx14, tx16));
    string tx17 = SyscoinBurn("node1", useraddress1, strSYSXAsset, "1", false);
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx13, tx17)); // last one in sequence for useraddress1
    string tx18 = AssetUpdate("node1", strSYSXAsset, "''", "''", "''", "''", "''", false);
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx16, tx18));
    string tx19 = AssetSend("node1", assetguid, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.1}]\"", "''", true, true, false);
    string tx20 = AssetUpdate("node1", strSYSXAsset, "''", "''", "''", "''", "''", false);
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx18, tx20));
    string tx21 = AssetSend("node1", assetguid, "\"[{\\\"address\\\":\\\"" + useraddress1 + "\\\",\\\"amount\\\":0.1}]\"", "''", true, true, false);
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx19, tx21));
    string tx22 = AssetUpdate("node1", strSYSXAsset, "''", "''", "''", "''", "''", false);
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx20, tx22));
    string tx23 = AssetSend("node1", assetguid, "\"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":0.1}]\"", "''", true, true, false);
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx21, tx23));
    string tx24 = AssetUpdate("node1", strSYSXAsset, "''", "''", "''", "", "''", false);
    BOOST_CHECK(AreTwoTransactionsLinked("node1", tx22, tx24));
    MilliSleep(500);
    
    // not a zdag transaction but still gives status ok
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + tx24));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + tx15));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);
  
    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationverifyzdag " + tx17));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);

    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationverifyzdag " + tx15));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);

    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationverifyzdag " + tx17));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);

    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationverifyzdag " + tx15));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);

    // dbl spend attempt
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsendmany " + strSYSXAsset + " " + useraddress1 + " \"[{\\\"address\\\":\\\"" + useraddress2 + "\\\",\\\"amount\\\":50.8}]\"" + " ''"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string assetHex = find_value(r.get_obj(), "hex").get_str();

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + assetHex, true, false));
    BOOST_CHECK(r.write().size() >= 32);
    
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "decoderawtransaction " + assetHex + " true"));
    string dblspendtx = find_value(r.get_obj(), "txid").get_str();
    MilliSleep(500);

    // original transactions are now marked as conflicted because the actor's are flagged
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + tx17));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + tx15));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);



    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + dblspendtx));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + dblspendtx));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);  
  
    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationverifyzdag " + dblspendtx));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);

    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationverifyzdag "+ tx5));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);  


    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationverifyzdag " + dblspendtx));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);

    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationverifyzdag " + tx6));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MAJOR_CONFLICT);
    tfm::format(std::cout, "Setting time ahead 300 seconds...\n");
    SleepFor(300 * 1000, 0);
    // after confirm we shouldn't have any txs to check
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + dblspendtx));
    // could be not found or status ok , status ok if node1 wallet accepting to mempool again because it detected unconfirmed transactions
    BOOST_CHECK(find_value(r.get_obj(), "status").get_int() == ZDAG_NOT_FOUND || find_value(r.get_obj(), "status").get_int() == ZDAG_STATUS_OK);

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + dblspendtx));
    BOOST_CHECK(find_value(r.get_obj(), "status").get_int() == ZDAG_NOT_FOUND || find_value(r.get_obj(), "status").get_int() == ZDAG_STATUS_OK);
    

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + tx17));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationverifyzdag " + tx15));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);


    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationverifyzdag " + tx17));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);

    BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationverifyzdag " + tx15));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);


    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationverifyzdag " + tx17));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);
  
    BOOST_CHECK_NO_THROW(r = CallRPC("node3", "assetallocationverifyzdag " + tx15));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);
}
BOOST_AUTO_TEST_CASE(generate_bad_assetmaxsupply_address)
{
    UniValue r;
	GenerateBlocks(5);
	tfm::format(std::cout,"Running generate_bad_assetmaxsupply_address...\n");
	GenerateBlocks(5);
	string newaddress = GetNewFundedAddress("node1");
	// 256 bytes long
	string gooddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	// 0 max supply good
	BOOST_CHECK_NO_THROW(CallRPC("node1", "assetnew " + newaddress + " tmp " + gooddata + " '' 8 1 0 31 ''"));
	// 1 max supply good
	BOOST_CHECK_NO_THROW(CallRPC("node1", "assetnew " + newaddress + " tmp " + gooddata + " '' 8 1 1 31 ''"));
	// balance > max supply
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetnew " + newaddress + " tmp " + gooddata + " '' 3 2000 1000 31 ''"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string hexStr = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
    BOOST_CHECK(r.write().size() < 32);
    
}

BOOST_AUTO_TEST_CASE(generate_assetupdate_address)
{
	tfm::format(std::cout,"Running generate_assetupdate_address...\n");
	string newaddress = GetNewFundedAddress("node1");
	string guid = AssetNew("node1", newaddress, "data");
	// update an asset that isn't yours
	UniValue r;
	//"assetupdate [asset] [public] [supply] [witness]\n"
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetupdate " + guid + " " + newaddress + " '' 1 31 ''"));

	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string hexStr = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
    BOOST_CHECK(r.write().size() < 32);
    
	AssetUpdate("node1", guid, "pub1");
	// shouldn't update data, just uses prev data because it hasn't changed
	AssetUpdate("node1", guid);
	// update supply, ensure balance gets updated properly, 5+1, 1 comes from the initial assetnew, 1 above doesn't actually get set because asset wasn't yours so total should be 6
	AssetUpdate("node1", guid, "pub12", "5");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid));
	UniValue balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8), 6 * COIN);
	// update supply
    int updateflags = ASSET_UPDATE_ALL & ~ASSET_UPDATE_SUPPLY;
	string guid1 = AssetNew("node1", newaddress, "data", "''", "8", "1", "10", "31");
    // can't change supply > max supply (current balance already 6, max is 10)
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetupdate " + guid + " " + newaddress + " '' 5 " + itostr(updateflags) + " ''"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    hexStr = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
    BOOST_CHECK(r.write().size() < 32);
        

	AssetUpdate("node1", guid1, "pub12", "1", itostr(updateflags));
	// ensure can't update supply (update flags is set to not allowsupply update)
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetupdate " + guid1 + " " + newaddress + " '' 1 " + itostr(updateflags) + " ''"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    hexStr = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
    BOOST_CHECK(r.write().size() < 32);

}
BOOST_AUTO_TEST_CASE(generate_assetupdate_precision_address)
{
	tfm::format(std::cout,"Running generate_assetupdate_precision_address...\n");
	UniValue r;
    string hexStr;
	for (int i = 0; i <= 8; i++) {
		string istr = itostr(i);
		string addressName = GetNewFundedAddress("node1");
		// test max supply for every possible precision
		
		UniValue negonevalue(UniValue::VSTR);
		negonevalue.setStr("-1");
		CAmount precisionCoin = pow(10, i);
		// get max value - 1 (1 is already the supply, and this value is cumulative)
		CAmount negonesupply = AssetAmountFromValue(negonevalue, i) - precisionCoin;
		string maxstrupdate = ValueFromAssetAmount(negonesupply, i).write();
		// can't create asset with more than max+1 balance or max+1 supply
		string maxstrplusone = ValueFromAssetAmount(negonesupply + (precisionCoin *2), i).write();
		string maxstrnew = ValueFromAssetAmount(negonesupply + precisionCoin, i).write();

		string guid = AssetNew("node1", addressName, "data", "''", istr, "1", maxstrnew);
		AssetUpdate("node1", guid, "pub12", maxstrupdate);
		// "assetupdate [asset] [public] [contract] [supply] [update_flags] [witness]\n"
		// can't go above max balance (10^18) / (10^i) for i decimal places
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetupdate " + guid + " pub '' 1 31 ''"));
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
        hexStr = find_value(r.get_obj(), "hex").get_str();
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
        BOOST_CHECK(r.write().size() < 32);


		// "assetnew [address] [symbol] [public value] [contract] [precision=8] [supply] [max_supply] [update_flags] [witness]\n"
		BOOST_CHECK_NO_THROW(r= CallRPC("node1", "assetnew " + addressName + " cat pub '' " + istr + " " + maxstrnew + " " + maxstrnew + " 31 ''"));
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
        hexStr = find_value(r.get_obj(), "hex").get_str();
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
        BOOST_CHECK(r.write().size() >= 32);

        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetnew " + addressName + " sysx pub '' " + istr + " 1 " + maxstrnew + " 31 ''"));
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
        hexStr = find_value(r.get_obj(), "hex").get_str();
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
        BOOST_CHECK(r.write().size() >= 32);

        // will send transaction but use 0 for total supply + balance
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetnew " + addressName + " syse pub '' " + istr + " " + maxstrplusone + " " + maxstrnew + " 31 ''"));
        string guidZero = itostr(find_value(r.get_obj(), "asset_guid").get_uint());
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
        hexStr = find_value(r.get_obj(), "hex").get_str();
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
        BOOST_CHECK(r.write().size() >= 32);
        GenerateBlocks(1);
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guidZero, true ));
        UniValue balance = find_value(r.get_obj(), "balance");
        UniValue totalsupply = find_value(r.get_obj(), "total_supply");
        BOOST_CHECK(AmountFromValue(balance) == 0);
        BOOST_CHECK(AmountFromValue(totalsupply) == 0);


		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetnew " + addressName + " syst pub '' " + istr + " 1 " + maxstrplusone + " 31 ''"));
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
        hexStr = find_value(r.get_obj(), "hex").get_str();
        BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
        BOOST_CHECK(r.write().size() < 32);

	}
    string newaddress = GetNewFundedAddress("node1");
	// invalid precisions
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetnew " + newaddress + " dow pub '' 9 1 2 31 ''"));
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    hexStr = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
    BOOST_CHECK(r.write().size() < 32);

	BOOST_CHECK_THROW(r = CallRPC("node1", "assetnew " + newaddress + " ep500 pub '' -1 1 2 31 ''"), runtime_error);

}
BOOST_AUTO_TEST_CASE(generate_assetsend_address)
{
	UniValue r;
	tfm::format(std::cout,"Running generate_assetsend_address...\n");
	string newaddress = GetNewFundedAddress("node1");
	string newaddress1 = GetNewFundedAddress("node1");
	string guid = AssetNew("node1", newaddress, "data", "''", "8", "10", "20");
	// [{\"address\":\"address\",\"amount\":amount},...]
	AssetSend("node1", guid, "\"[{\\\"address\\\":\\\"" + newaddress1 + "\\\",\\\"amount\\\":7}]\"");
	// ensure amounts are correct
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid));
	UniValue balance = find_value(r.get_obj(), "balance");
	UniValue totalsupply = find_value(r.get_obj(), "total_supply");
	UniValue maxsupply = find_value(r.get_obj(), "max_supply");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8), 3 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(totalsupply, 8), 10 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(maxsupply, 8), 20 * COIN);

	// ensure receiver gets it
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " " + newaddress1 ));

	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8), 7 * COIN);

	// add balances
	AssetUpdate("node1", guid, "pub12", "1");
	// check balance is added to end
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid));
	balance = find_value(r.get_obj(), "balance");
	totalsupply = find_value(r.get_obj(), "total_supply");
	maxsupply = find_value(r.get_obj(), "max_supply");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8), 4 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(totalsupply, 8), 11 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(maxsupply, 8), 20 * COIN);

	AssetUpdate("node1", guid, "pub12", "9");
	// check balance is added to end
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid ));
	balance = find_value(r.get_obj(), "balance");
	totalsupply = find_value(r.get_obj(), "total_supply");
	maxsupply = find_value(r.get_obj(), "max_supply");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8), 13 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(totalsupply, 8), 20 * COIN);
	BOOST_CHECK_EQUAL(AssetAmountFromValue(maxsupply, 8), 20 * COIN);

	// can't go over 20 supply
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetupdate " + guid + " " + newaddress + " '' 1 31 ''"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string hexStr = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
    BOOST_CHECK(r.write().size() < 32);

}
BOOST_AUTO_TEST_CASE(generate_assettransfer_address)
{
    UniValue r;
	tfm::format(std::cout,"Running generate_assettransfer_address...\n");
	GenerateBlocks(15, "node1");
	GenerateBlocks(15, "node2");
	GenerateBlocks(15, "node3");
	string newaddres1 = GetNewFundedAddress("node1");
    string newaddres2 = GetNewFundedAddress("node2");
    BOOST_CHECK_NO_THROW(r = CallExtRPC("node3", "getnewaddress"));
    string newaddres3 = r.get_str();
	string guid1 = AssetNew("node1", newaddres1, "pubdata");
	string guid2 = AssetNew("node2", newaddres2, "pubdata");
    	
	AssetUpdate("node1", guid1, "pub3");
	AssetTransfer("node1", "node2", guid1, newaddres2);

	AssetTransfer("node2", "node3", guid2, newaddres3);
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid1));
    BOOST_CHECK(find_value(r.get_obj(), "address").get_str() == newaddres2);

	// xfer an asset that isn't yours
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assettransfer " + guid1 + " " + newaddres3 + " ''"));

    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransactionwithwallet " + find_value(r.get_obj(), "hex").get_str()));
    string hexStr = find_value(r.get_obj(), "hex").get_str();
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "sendrawtransaction " + hexStr, true, false));
    BOOST_CHECK(r.write().size() < 32);  
    GenerateBlocks(5, "node1");
    BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid1));
    BOOST_CHECK(find_value(r.get_obj(), "address").get_str() == newaddres2);

	// update xferred asset
	AssetUpdate("node2", guid1, "public");

	// retransfer asset
	AssetTransfer("node2", "node3", guid1, newaddres3);
}
BOOST_AUTO_TEST_SUITE_END ()
