// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/test_syscoin_services.h>
#include <test/data/utxo.json.h>
#include <test/data/assetbalances.json.h>
#include <util/time.h>
#include <rpc/server.h>
#include <boost/test/unit_test.hpp>
#include <univalue.h>
#include <util/strencodings.h>
#include <core_io.h>
#include <rpc/util.h>
#include <services/rpc/assetrpc.h>
using namespace std;
int currentTx = 0;
extern UniValue read_json(const std::string& jsondata);
BOOST_FIXTURE_TEST_SUITE (syscoin_snapshot_tests, SyscoinMainNetSetup)
struct PaymentAmount
{
	std::string address;
	std::string amount;
};
struct AssetAllocationKey
{
	std::string guid;
	std::string sender;
};
void SendSnapShotPayment(const std::string &strSend)
{
	currentTx++;
	std::string strSendMany = "sendmany \"\" \"{" + strSend + "}\"";
	CallRPC("mainnet1", strSendMany, false, false);
}
void GenerateSnapShot(const std::vector<PaymentAmount> &paymentAmounts)
{
	// generate snapshot payments and let it mature
	printf("Generating 101 blocks to start the mainnet\n");
	GenerateMainNetBlocks(101, "mainnet1");
	int numberOfTxPerBlock = 250;
	int totalTx = 0;
	double nTotal  =0;
	std::string sendManyString = "";
	for(unsigned int i =0;i<paymentAmounts.size();i++)
	{
		if(sendManyString != "") 
			sendManyString += ",";
		sendManyString += "\\\"" + paymentAmounts[i].address + "\\\":" + paymentAmounts[i].amount;
		double total;
		BOOST_CHECK(ParseDouble(paymentAmounts[i].amount, &total));
		
		nTotal += total;
		totalTx++;
		if(i != 0 && (i%numberOfTxPerBlock) == 0)
		{
			printf("strSendMany #%d, total %f, num txs %d\n", currentTx, nTotal, totalTx);
			SendSnapShotPayment(sendManyString);
			GenerateMainNetBlocks(1, "mainnet1");
			sendManyString = "";
			nTotal = 0;
		}
	}
	if(sendManyString != "") 
	{
		printf("FINAL strSendMany #%d, total %f, num txs %d\n", currentTx, nTotal, totalTx);
		SendSnapShotPayment(sendManyString);
		GenerateMainNetBlocks(1, "mainnet1");
	}

	GenerateMainNetBlocks(1, "mainnet1");
	printf("done!\n");
}
void GetUTXOs(std::vector<PaymentAmount> &paymentAmounts)
{
	int countTx = 0;
	int rejectTx = 0;
    UniValue tests = read_json(std::string(json_tests::utxo, json_tests::utxo + sizeof(json_tests::utxo)));
    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        UniValue test = tests[idx];
        std::string strTest = test.write();
        if (test.size() < 2) // Allow for extra stuff (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
		PaymentAmount payment;
        payment.address  = test[0].get_str();
		CAmount amountInSys1 = test[1].get_int64();
		// don't transfer less than 1 coin utxo's
		if(amountInSys1 <= 0.1*COIN)
		{
			rejectTx++;
			continue;
		}
		countTx++;
        payment.amount = ValueFromAmount(amountInSys1).write();
		paymentAmounts.push_back(payment);
    }
	printf("Read %d total utxo sets, rejected %d, valid %d\n", rejectTx+countTx, rejectTx, countTx);
}
void SendSnapShotAssetAllocation(const std::string &guid, const std::string &strSend)
{
	currentTx++;
	// string AssetSend(const string& node, const string& name, const string& inputs, const string& witness = "''", bool completetx=true, bool bRegtest = true);
	AssetSend("mainnet1", guid, "\"[" + strSend + "]\"", "''", true, false);
}
void GenerateAssetAllocationSnapshots(const std::string &guid, const UniValue &assetAllocations, int precision)
{
	UniValue r;
	int numberOfTxPerBlock = 249;
	int totalTx = 0;
	CAmount nTotal  =0;
	currentTx = 0;
	std::string sendManyString = "";
	printf("Sending allocations for asset %s\n", guid.c_str());
	for(unsigned int i =0;i<assetAllocations.size();i++)
	{
		const UniValue& assetAllocationObj = assetAllocations[i].get_obj();
		const std::string &address = find_value(assetAllocationObj, "address").get_str();
		UniValue amountValue = find_value(assetAllocationObj, "balance");
		const CAmount &nBalance = AssetAmountFromValue(amountValue, precision);
		const std::string &balance = ValueFromAssetAmount(nBalance, precision).write();
		BOOST_CHECK_NO_THROW(r = CallRPC("mainnet1", "convertaddress " + address, false));
		const std::string &witnessaddress = find_value(r.get_obj(), "v4address").get_str();
		
		if(sendManyString != "") 
			sendManyString += ",";
		sendManyString += "{\\\"address\\\":\\\"" + witnessaddress + "\\\",\\\"amount\\\":" + balance + "}";
		
		nTotal += nBalance;
		totalTx++;
		if(i != 0 && (i%numberOfTxPerBlock) == 0)
		{
			printf("strSendMany #%d, total %s, num txs %d\n", currentTx, ValueFromAssetAmount(nTotal, precision).write().c_str(), totalTx);
			SendSnapShotAssetAllocation(guid, sendManyString);
			sendManyString = "";
			nTotal = 0;
		}
	}
	if(sendManyString != "") 
	{
		printf("FINAL strSendMany #%d, total %s, num txs %d\n", currentTx, ValueFromAssetAmount(nTotal, precision).write().c_str(), totalTx);
		SendSnapShotAssetAllocation(guid, sendManyString);
	}

	GenerateMainNetBlocks(1, "mainnet1");
	printf("done!\n");
}
void GetAssetBalancesAndPrepareAsset()
{
	
    UniValue assetBalanceArray = read_json(std::string(json_tests::assetbalances, json_tests::assetbalances + sizeof(json_tests::assetbalances)));
    printf("Creating and distributing %d assets\n", assetBalanceArray.size());
	for (unsigned int idx = 0; idx < assetBalanceArray.size(); idx++) {
		UniValue r;
		const std::string &newaddress = GetNewFundedAddress("mainnet1",false);
		printf("Got funded address %s\n", newaddress.c_str());
        const UniValue &assetBalances = assetBalanceArray[idx].get_obj();
		const std::string &address = find_value(assetBalances, "address").get_str();
		const std::string &symbol = find_value(assetBalances, "symbol").get_str();
		std::string publicvalue = find_value(assetBalances, "publicvalue").get_str();
		if(publicvalue.empty())
			publicvalue = "''";
		const std::string &balance = find_value(assetBalances, "balance").get_str();
		const std::string &total_supply = find_value(assetBalances, "total_supply").get_str();
		const std::string &max_supply = find_value(assetBalances, "max_supply").get_str();
		const int &nPrecision = find_value(assetBalances, "precision").get_int();
		const UniValue& paymentAmounts = find_value(assetBalances, "allocations").get_array();

		// string AssetNew(const string& node, const string& address, const string& pubdata = "''", const string& contract="''", const string& precision="8", const string& supply = "1", const string& maxsupply = "10", const string& updateflags = "31", const string& witness = "''", const string& symbol = "SYM",  bool bRegtest = true);
		std::string guid = AssetNew("mainnet1", newaddress, publicvalue, "''", itostr(nPrecision), total_supply, max_supply, "31", "''", symbol, false);
		printf("Created asset %s - %s with %d allocations\n", guid.c_str(), symbol.c_str(), paymentAmounts.size());
		if(paymentAmounts.size() > 0){
			GenerateAssetAllocationSnapshots(guid, paymentAmounts, nPrecision);
			printf("Created %d allocations\n", paymentAmounts.size());
		}
		BOOST_CHECK_NO_THROW(r = CallRPC("mainnet1", "convertaddress " + address, false));
		const std::string &witnessaddress = find_value(r.get_obj(), "v4address").get_str();
		// void AssetTransfer(const string& node, const string &tonode, const string& guid, const string& toaddress, const string& witness = "''", bool bRegtest = true);
		AssetTransfer("mainnet1", "mainnet2", guid, witnessaddress, "''", false);
    }
}
bool IsMainNetAlreadyCreated()
{
	int height;
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC("mainnet1", "getblockchaininfo", false));
	height = find_value(r.get_obj(), "blocks").get_int();
	return height > 1;
}
void generate_snapshot_asset_consistency_check()
{
	UniValue assetInvalidatedResults, assetNowResults, assetValidatedResults;
	UniValue assetAllocationsInvalidatedResults, assetAllocationsNowResults, assetAllocationsValidatedResults;
	UniValue r;
	printf("Running generate_snapshot_asset_consistency_check...\n");
	GenerateBlocks(5);

	BOOST_CHECK_NO_THROW(r = CallRPC("mainnet1", "getblockcount", false, false));
	string strBeforeBlockCount = r.write();
    // remove new line and string terminator
    strBeforeBlockCount = strBeforeBlockCount.substr(1, strBeforeBlockCount.size() - 4);
    
	// first check around disconnect/connect by invalidating and revalidating an early block
	BOOST_CHECK_NO_THROW(assetNowResults = CallRPC("mainnet1", "listassets " + itostr(INT_MAX) + " 0", false));
	BOOST_CHECK_NO_THROW(assetAllocationsNowResults = CallRPC("mainnet1", "listassetallocations " + itostr(INT_MAX) + " 0", false));

	// disconnect back to block 10 where no assets would have existed
	BOOST_CHECK_NO_THROW(r = CallRPC("mainnet1", "getblockhash 10", false, false));
	string blockHashInvalidated = r.get_str();
	BOOST_CHECK_NO_THROW(CallRPC("mainnet1", "invalidateblock " + blockHashInvalidated, false, false));
	BOOST_CHECK_NO_THROW(r = CallRPC("mainnet1", "getblockcount", false, false));
    string strAfterBlockCount = r.write();
    strAfterBlockCount = strAfterBlockCount.substr(1, strAfterBlockCount.size() - 4);
	BOOST_CHECK_EQUAL(strAfterBlockCount, "9");

	UniValue emptyResult(UniValue::VARR);
	BOOST_CHECK_NO_THROW(assetInvalidatedResults = CallRPC("mainnet1", "listassets " + itostr(INT_MAX) + " 0", false));
	BOOST_CHECK_EQUAL(assetInvalidatedResults.write(), emptyResult.write());
	BOOST_CHECK_NO_THROW(assetAllocationsInvalidatedResults = CallRPC("mainnet1", "listassetallocations " + itostr(INT_MAX) + " 0", false));
	BOOST_CHECK_EQUAL(assetAllocationsInvalidatedResults.write(), emptyResult.write());

	// reconnect to tip and ensure block count matches
	BOOST_CHECK_NO_THROW(CallRPC("mainnet1", "reconsiderblock " + blockHashInvalidated, false, false));
	BOOST_CHECK_NO_THROW(r = CallRPC("mainnet1", "getblockcount", false, false));
    string strRevalidatedBlockCount = r.write();
    strRevalidatedBlockCount = strRevalidatedBlockCount.substr(1, strRevalidatedBlockCount.size() - 4);
	BOOST_CHECK_EQUAL(strRevalidatedBlockCount, strBeforeBlockCount);

	BOOST_CHECK_NO_THROW(assetValidatedResults = CallRPC("mainnet1", "listassets " + itostr(INT_MAX) + " 0", false));
	BOOST_CHECK_EQUAL(assetValidatedResults.write(), assetNowResults.write());
	BOOST_CHECK_NO_THROW(assetAllocationsValidatedResults = CallRPC("mainnet1", "listassetallocations " + itostr(INT_MAX) + " 0", false));
	BOOST_CHECK_EQUAL(assetAllocationsValidatedResults.write(), assetAllocationsNowResults.write());	
	// try to check after reindex
	StopNode("mainnet1", false);
	StartNode("mainnet1", false, "", true);

	BOOST_CHECK_NO_THROW(assetValidatedResults = CallRPC("mainnet1", "listassets " + itostr(INT_MAX) + " 0", false));
	BOOST_CHECK_EQUAL(assetValidatedResults.write(), assetNowResults.write());
	BOOST_CHECK_NO_THROW(assetAllocationsValidatedResults = CallRPC("mainnet1", "listassetallocations " + itostr(INT_MAX) + " 0", false));
	BOOST_CHECK_EQUAL(assetAllocationsValidatedResults.write(), assetAllocationsNowResults.write());	
}
BOOST_AUTO_TEST_CASE (generate_and_verify_snapshot)
{
	printf("Running generate_and_verify_snapshot...\n");
	std::vector<PaymentAmount> paymentAmounts;
	if(!IsMainNetAlreadyCreated())
	{
		GetUTXOs(paymentAmounts);
		GenerateSnapShot(paymentAmounts);
		GetAssetBalancesAndPrepareAsset();
		generate_snapshot_asset_consistency_check();
	}
}
BOOST_AUTO_TEST_SUITE_END ()
