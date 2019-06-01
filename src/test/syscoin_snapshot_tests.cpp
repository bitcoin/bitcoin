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
		/*  "\nCreate a new asset\n",
            {
                {"address", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "An address that you own."},
                {"symbol", RPCArg::Type::STR, RPCArg::Optional::NO, "Asset symbol (1-8 characters)"},
                {"public_value", RPCArg::Type::STR, RPCArg::Optional::NO, "public data, 256 characters max."},
                {"contract", RPCArg::Type::STR, RPCArg::Optional::NO, "Ethereum token contract for SyscoinX bridge. Must be in hex and not include the '0x' format tag. For example contract '0xb060ddb93707d2bc2f8bcc39451a5a28852f8d1d' should be set as 'b060ddb93707d2bc2f8bcc39451a5a28852f8d1d'. Leave empty for no smart contract bridge."},
                {"precision", RPCArg::Type::NUM, RPCArg::Optional::NO, "Precision of balances. Must be between 0 and 8. The lower it is the higher possible max_supply is available since the supply is represented as a 64 bit integer. With a precision of 8 the max supply is 10 billion."},
                {"total_supply", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Initial supply of asset. Can mint more supply up to total_supply amount or if total_supply is -1 then minting is uncapped."},
                {"max_supply", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Maximum supply of this asset. Set to -1 for uncapped. Depends on the precision value that is set, the lower the precision the higher max_supply can be."},
                {"update_flags", RPCArg::Type::NUM, RPCArg::Optional::NO, "Ability to update certain fields. Must be decimal value which is a bitmask for certain rights to update. The bitmask represents 0x01(1) to give admin status (needed to update flags), 0x10(2) for updating public data field, 0x100(4) for updating the smart contract field, 0x1000(8) for updating supply, 0x10000(16) for being able to update flags (need admin access to update flags as well). 0x11111(31) for all."},
                {"witness", RPCArg::Type::STR, RPCArg::Optional::NO, "Witness address that will sign for web-of-trust notarization of this transaction."}
            }*/
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
BOOST_AUTO_TEST_CASE (generate_and_verify_snapshot)
{
	
	std::vector<PaymentAmount> paymentAmounts;
	/*if(!IsMainNetAlreadyCreated())
	{
		GetUTXOs(paymentAmounts);
		GenerateSnapShot(paymentAmounts);
		GetAssetBalancesAndPrepareAsset();
		
	}*/
	GetAssetBalancesAndPrepareAsset();
}
BOOST_AUTO_TEST_SUITE_END ()
