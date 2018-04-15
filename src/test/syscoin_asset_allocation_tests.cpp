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

	string txid0 = AssetAllocationTransfer(false, "node1", guid, "jagassetallocationsend1", "\"[{\\\"aliasto\\\":\\\"jagassetallocationsend2\\\",\\\"amount\\\":0.1}]\"", "allocationsendmemo");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetallocationsend2 false"));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false),0.1 * COIN);

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " jagassetallocationsend1 " + txid0));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);

	// send using zdag
	string txid1 = AssetAllocationTransfer(true, "node1", guid, "jagassetallocationsend1", "\"[{\\\"aliasto\\\":\\\"jagassetallocationsend2\\\",\\\"amount\\\":0.1}]\"", "allocationsendmemo");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo newassetsend jagassetallocationsend2 false"));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 0.2 * COIN);

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
	string txid2 = AssetAllocationTransfer(true, "node1", guid, "jagassetallocationsend1", "\"[{\\\"aliasto\\\":\\\"jagassetallocationsend2\\\",\\\"amount\\\":0.1}]\"", "allocationsendmemo");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " jagassetallocationsend2 false"));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8, false), 0.3 * COIN);
	
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
BOOST_AUTO_TEST_SUITE_END ()
