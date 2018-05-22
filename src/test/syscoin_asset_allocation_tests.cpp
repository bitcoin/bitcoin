// Copyright (c) 2016-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "util.h"
#include "rpc/server.h"
#include "alias.h"
#include "cert.h"
#include "asset.h"
#include "base58.h"
#include "chainparams.h"
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <iterator>
#include <chrono>
#include "ranges.h"
using namespace boost::chrono;
using namespace std;
BOOST_GLOBAL_FIXTURE( SyscoinTestingSetup );

BOOST_FIXTURE_TEST_SUITE(syscoin_asset_allocation_tests, BasicSyscoinTestingSetup)
BOOST_AUTO_TEST_CASE(generate_asset_allocation_send)
{
	UniValue r;
	printf("Running generate_asset_allocation_send...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagassetallocationsend1", "data");
	AliasNew("node2", "jagassetallocationsend2", "data");
	string guid = AssetNew("node1", "usd", "jagassetallocationsend1", "data", "8", "false", "1", "-1");

	AssetSend("node1", guid, "\"[{\\\"aliasto\\\":\\\"jagassetallocationsend1\\\",\\\"amount\\\":1}]\"", "assetallocationsend");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetallocationsend1 false"));
	UniValue balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 1 * COIN);

	string txid0 = AssetAllocationTransfer(false, "node1", guid, "jagassetallocationsend1", "\"[{\\\"aliasto\\\":\\\"jagassetallocationsend2\\\",\\\"amount\\\":0.11}]\"", "allocationsendmemo");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetallocationsend2 false"));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false),0.11 * COIN);

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 " + txid0));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);

	// send using zdag
	string txid1 = AssetAllocationTransfer(true, "node1", guid, "jagassetallocationsend1", "\"[{\\\"aliasto\\\":\\\"jagassetallocationsend2\\\",\\\"amount\\\":0.12}]\"", "allocationsendmemo");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetallocationsend2 false"));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 0.23 * COIN);

	// non zdag cannot be found since it was already mined, but ends up briefly in conflict state because sender is conflicted
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 " + txid0));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MINOR_CONFLICT_OK);

	// first tx should have to wait 1 sec for good status
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid +  " jagassetallocationsend1 " + txid1));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MINOR_CONFLICT_OK);

	// check just sender
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 ''"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MINOR_CONFLICT_OK);

	// wait for 1 second as required by unit test
	MilliSleep(1000);

	// second send
	string txid2 = AssetAllocationTransfer(true, "node1", guid, "jagassetallocationsend1", "\"[{\\\"aliasto\\\":\\\"jagassetallocationsend2\\\",\\\"amount\\\":0.13}]\"", "allocationsendmemo");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetallocationsend2 false"));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 0.36 * COIN);
	
	// sender is conflicted so txid0 is conflicted by extension even if its not found
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 " + txid0));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MINOR_CONFLICT_OK);

	// first ones now OK because it was found explicitely
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 " + txid1));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);

	// second one hasn't waited enough time yet
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 " + txid2));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MINOR_CONFLICT_OK);

	// check just sender
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 ''"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MINOR_CONFLICT_OK);

	// wait for 1.5 second to clear minor warning status
	MilliSleep(1500);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 " + txid0));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 " + txid1));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);


	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 " + txid2));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);


	// check just sender as well
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 ''"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);


	// after pow, should get not found on all of them
	GenerateBlocks(1);

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 " + txid0));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 " + txid1));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);


	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 " + txid2));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);

	// check just sender as well
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 ''"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);

}
BOOST_AUTO_TEST_CASE(generate_asset_allocation_througput)
{
	UniValue r;
	printf("Running generate_asset_allocation_througput...\n");
	GenerateBlocks(5);
	map<string, string> assetMap;
	map<string, string> assetAliasMap;
	// create 1000 aliases and assets for each asset
	printf("creating 1000 aliases/asset...\n");
	for (int i = 0; i < 1000; i++) {
		string aliasname = "jagthroughput-" + boost::lexical_cast<string>(i);
		string aliasnameto = "jagthroughput3-" + boost::lexical_cast<string>(i);
	
		// registration
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasnew " + aliasname + " '' no 0 '' '' '' ''"));
		UniValue varray = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscointxfund " + varray[0].get_str()));
		varray = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + varray[0].get_str()));
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsendrawtransaction " + find_value(r.get_obj(), "hex").get_str()));
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "generate 1"));
		// activation
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasnew " + aliasname + " '' no 0 '' '' '' ''"));
		UniValue varray1 = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscointxfund " + varray1[0].get_str()));
		varray1 = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + varray1[0].get_str()));
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsendrawtransaction " + find_value(r.get_obj(), "hex").get_str()));

		// registration
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasnew " + aliasnameto + " '' no 0 '' '' '' ''"));
		varray = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "syscointxfund " + varray[0].get_str()));
		varray = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "signrawtransaction " + varray[0].get_str()));
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "syscoinsendrawtransaction " + find_value(r.get_obj(), "hex").get_str()));
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "generate 1"));
		// activation
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasnew " + aliasnameto + " '' no 0 '' '' '' ''"));
		varray1 = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "syscointxfund " + varray1[0].get_str()));
		varray1 = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "signrawtransaction " + varray1[0].get_str()));
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "syscoinsendrawtransaction " + find_value(r.get_obj(), "hex").get_str()));


		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetnew tpstest " + aliasname + " '' " + " assets " + " 8 false 1 10 0 false ''"));
		UniValue arr = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + arr[0].get_str()));
		string hex_str = find_value(r.get_obj(), "hex").get_str();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsendrawtransaction " + hex_str));
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "decoderawtransaction " + hex_str));
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "generate 1"));
		string guid = arr[1].get_str();
		assetMap[guid] = aliasnameto;
		assetAliasMap[guid] = aliasname;
		if (i % 100 == 0)
			printf("%.2f percentage done\n", 100.0f / (1000.0f / (i+1)));
	}
	printf("Creating assetsend transactions to node3 alias...\n");
	vector<string> assetSendTxVec;
	int count = 0;
	for (auto& assetTuple : assetMap) {
		count++;
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetsend " + assetTuple.first + " " + assetAliasMap[assetTuple.first] + " " + "\"[{\\\"aliasto\\\":\\\"" + assetTuple.second + "\\\",\\\"amount\\\":1}]\"" + " '' ''"));
		UniValue arr = r.get_array();
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + arr[0].get_str()));
		string hex_str = find_value(r.get_obj(), "hex").get_str();

		assetSendTxVec.push_back(hex_str);
		if (count % 100 == 0)
			printf("%.2f percentage done\n", 100.0f / (1000.0f / count));
	}
	printf("Sending assetsend transactions to network...\n");
	map<string, int64_t> sendTimes;
	count = 0;
	for (auto& hex_str : assetSendTxVec) {
		count++;
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsendrawtransaction " + hex_str));
		string txid = find_value(r.get_obj(), "txid").get_str();
		sendTimes[txid] = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		if (count % 100 == 0)
			printf("%.2f percentage done\n", 100.0f / (1000.0f / count));
	}
	printf("Gathering results...\n");
	float totalTime = 0;
	// wait 10 seconds
	MilliSleep(10000);
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "tpstestinfo"));
	UniValue tpsresponse = r.get_array();
	for (int i = 0; i < tpsresponse.size();i++) {
		UniValue responesObj = tpsresponse[i].get_obj();
		string txid = find_value(responesObj, "txid").get_str();
		int64_t timeRecv = find_value(responesObj, "time").get_int64();
		if (sendTimes.find(txid) == sendTimes.end())
			continue;
		totalTime += timeRecv - sendTimes[txid];
	}
	totalTime /= tpsresponse.size();
	printf("totalTime %.2f, num responses %d\n", totalTime, tpsresponse.size());
}
BOOST_AUTO_TEST_SUITE_END ()
