// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_syscoin_services.h"
#include "data/utxo.json.h"
#include <util/time.h>
#include "rpc/server.h"
#include <boost/test/unit_test.hpp>
#include <univalue.h>
using namespace std;
int currentTx = 0;
extern UniValue read_json(const std::string& jsondata);
BOOST_FIXTURE_TEST_SUITE (syscoin_snapshot_tests, SyscoinMainNetSetup)
struct PaymentAmount
{
	std::string address;
	std::string alias;
	CAmount amount;
};
void SendSnapShotPayment(const std::string &strSend, const std::string &asset, const std::string &alias, const std::string &memo)
{
	currentTx++;
	std::string strSendMany = "assetsendmany " + asset + " " + strSend + "}]\" " + memo + " ''";
	UniValue r;
	BOOST_CHECK_THROW(r = CallRPC("mainnet1", strSendMany, false), runtime_error);
}
void GenerateAirDrop(const std::vector<PaymentAmount> &paymentAmounts, const CAmount& nTotal)
{
	UniValue r;
	// air drop in percentage / 100
	double airDropPct = 0.05;
	string assetName = "asset";
	string aliasName = "alias";
	string memo = assetName+"-AIRDROP";
	BOOST_CHECK_NO_THROW(r = CallRPC("mainnet1", "assetinfo " + assetName, false));

	if (!find_value(r.get_obj(), "inputs").get_array().empty())
	{
		printf("This script does not support airdropping input range based assets *yet*\n");
		return;
	}

	// set up max supply of asset, if its infinite use total supply as the gauge for the airDropPct percentage airdrop
	CAmount nMaxSupply = AmountFromValue(find_value(r.get_obj(), "max_supply"));
	if(nMaxSupply <= 0)
		nMaxSupply = AmountFromValue(find_value(r.get_obj(), "total_supply"));

	// total supply being airdropped
	nMaxSupply *= airDropPct;

	// generate snapshot payments and let it mature
	// import private key of alias
	// drop shares on aliases
	int numberOfTxPerBlock = 250;
	int totalTx = 0;
	CAmount nTotalSent  =0;
	std::string sendManyString = "";
	for(int i =0;i<paymentAmounts.size();i++)
	{
		double fractionOwnershipInSys = paymentAmounts[i].amount / nTotal;
		CAmount nAmountToAirDrop = nMaxSupply * fractionOwnershipInSys;

		if(sendManyString != "") 
			sendManyString += ",";
		//"\"[{\\\"aliasto\\\":\\\"jagassetallocationsend2\\\",\\\"amount\\\":0.1}]\""
		sendManyString += "\"[{\\\"aliasto\\\":\\\"" + paymentAmounts[i].alias + "\\\",\\\"amount\\\":" + ValueFromAmount(nAmountToAirDrop).write();
		nTotalSent += paymentAmounts[i].amount;
		totalTx++;
		if(i != 0 && (i%numberOfTxPerBlock) == 0)
		{
			printf("strSendAsset #%d, total %s, num txs %d\n", currentTx, ValueFromAmount(nTotalSent).write(), totalTx);
			SendSnapShotPayment(sendManyString);
			sendManyString = "";
			nTotalSent = 0;
		}
	}
	if(sendManyString != "") 
	{
		printf("FINAL strSendAsset #%d, total %s, num txs %d\n", currentTx, ValueFromAmount(nTotalSent).write(), totalTx);
		SendSnapShotPayment(sendManyString);

	}
	
}
void GetUTXOs(std::vector<PaymentAmount> &paymentAmounts, CAmount& nTotal)
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
		if(amountInSys1 <= COIN)
		{
			rejectTx++;
			continue;
		}
		nTotal += amountInSys1;
		BOOST_CHECK_NO_THROW(r = CallRPC("mainnet1", "validateaddress " + test[0].get_str(), false));
		BOOST_CHECK(find_value(r.get_obj(), "alias").get_str() !+ "");
		countTx++;
		payment.alias = find_value(r.get_obj(), "alias").get_str();
        payment.amount = amountInSys1;
		paymentAmounts.push_back(payment);
    }
	printf("Read %d total utxo sets, rejected %d, valid aliases %d, total amount %lld\n", rejectTx+countTx, rejectTx, countTx);
}
bool IsMainNetAlreadyCreated()
{
	int height;
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC("mainnet1", "getinfo", false));
	height = find_value(r.get_obj(), "blocks").get_int();
	return height > 1;
}
BOOST_AUTO_TEST_CASE (generate_and_verify_snapshot)
{
	CAmount nTotal = 0;
	std::vector<PaymentAmount> paymentAmounts;
	GetUTXOs(paymentAmounts, nTotal);
	if(!IsMainNetAlreadyCreated())
	{
		GenerateAirDrop(paymentAmounts, nTotal);
	}
}
BOOST_AUTO_TEST_SUITE_END ()
