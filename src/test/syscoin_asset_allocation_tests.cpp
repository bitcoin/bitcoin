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
BOOST_AUTO_TEST_CASE(generate_assetallocationpruning)
{
	UniValue r;
	// makes sure services expire in 100 blocks instead of 1 year of blocks for testing purposes
	printf("Running generate_assetallocationpruning...\n");
	AliasNew("node1", "jagprunealias2", "changeddata1");
	MilliSleep(5000);
	AliasNew("node2", "jagprunealias2node2", "data");
	// stop node2 create a service,  mine some blocks to expire the service, when we restart the node the service data won't be synced with node2
	StopNode("node2");
	string guid = AssetNew("node1", "bcf", "jagprunealias2", "pubdata", "8", "false", "10000", "-1", "0.05");
	AssetSend("node1", guid, "\"[{\\\"aliasto\\\":\\\"jagprunealias2\\\",\\\"amount\\\":1}]\"", "assetallocationsend");
	AssetAllocationTransfer(false, "node1", guid, "jagprunealias2", "\"[{\\\"aliasto\\\":\\\"jagprunealias2node2\\\",\\\"amount\\\":0.11}]\"", "allocationsendmemo");
	// we can find it as normal first
	BOOST_CHECK_NO_THROW(CallRPC("node1", "assetinfo " + guid + " false"));
	// make sure our alias doesn't expire
	string hex_str = AliasUpdate("node1", "jagprunealias2");
	BOOST_CHECK(hex_str.empty());
	GenerateBlocks(5, "node1");
	ExpireAlias("jagprunealias2");
	StartNode("node2");
	GenerateBlocks(5, "node2");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid + " false"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), true);

	// asset allocation is not expired but cannot claim interest because asset is expired
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagprunealias2node2 false"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), false);

	BOOST_CHECK_THROW(r = CallRPC("node2", "assetallocationcollectinterest " + guid + " jagprunealias2node2 ''"), runtime_error);

	// should be pruned
	BOOST_CHECK_THROW(CallRPC("node2", "assetinfo " + guid + " false"), runtime_error);

	// expire asset allocation alias and now asset allocation is also expired
	ExpireAlias("jagprunealias2node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagprunealias2node2 false"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), true);

	// stop node3
	StopNode("node3");
	// should fail: already expired alias
	BOOST_CHECK_THROW(CallRPC("node1", "assetupdate " + guid + " jagprunealias2 assets 0 0 ''"), runtime_error);
	GenerateBlocks(5, "node1");

	// stop and start node1
	StopNode("node1");
	StartNode("node1");
	GenerateBlocks(5, "node1");

	BOOST_CHECK_THROW(CallRPC("node1", "assetinfo " + guid + " false"), runtime_error);

	// create a new service
	AliasNew("node1", "jagprunealias2", "temp");
	AliasNew("node2", "jagprunealias2node2", "temp");
	string guid1 = AssetNew("node1", "bcf", "jagprunealias2", "pubdata", "8", "false", "10000", "-1", "0.05");
	// ensure you can still update before expiry
	AssetUpdate("node1", guid1);
	AssetSend("node1", guid1, "\"[{\\\"aliasto\\\":\\\"jagprunealias2\\\",\\\"amount\\\":1}]\"", "assetallocationsend");
	AssetAllocationTransfer(false, "node1", guid1, "jagprunealias2", "\"[{\\\"aliasto\\\":\\\"jagprunealias2node2\\\",\\\"amount\\\":0.11}]\"", "allocationsendmemo");
	// you can search it still on node1/node2
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid1 + " false"));
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetinfo " + guid1 + " false"));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid1 + " jagprunealias2node2 false"));
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationinfo " + guid1 + " jagprunealias2node2 false"));
	// make sure our alias doesn't expire
	hex_str = AliasUpdate("node1", "jagprunealias2");
	BOOST_CHECK(hex_str.empty());
	GenerateBlocks(5, "node1");
	ExpireAlias("jagprunealias2node2");
	// now it should be expired
	BOOST_CHECK_THROW(r = CallRPC("node2", "assetallocationcollectinterest " + guid1 + " jagprunealias2node2 ''"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "assetupdate " + guid1 + " jagprunealias2 assets 0 0 ''"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node2", "assetallocationcollectinterest " + guid1 + " jagprunealias2node2 ''"), runtime_error);
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetinfo " + guid1 + " false"));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid1 + " jagprunealias2node2 false"));
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationinfo " + guid1 + " jagprunealias2node2 false"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), true);
	// and it should say its expired
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetinfo " + guid1 + " false"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), true);
	GenerateBlocks(5, "node1");
	StartNode("node3");
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "assetinfo " + guid1 + " false"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node3", "assetallocationinfo " + guid1 + " jagprunealias2node2 false"), runtime_error);
}
BOOST_AUTO_TEST_SUITE_END ()
