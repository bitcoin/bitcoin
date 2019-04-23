// Copyright (c) 2016-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_syscoin_services.h"
#include <util/time.h>
#include "util.h"
#include "rpc/server.h"
#include "services/asset.h"
#include "base58.h"
#include "chainparams.h"
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <iterator>
#include <key.h>
using namespace std;
BOOST_GLOBAL_FIXTURE( SyscoinTestingSetup );
BOOST_FIXTURE_TEST_SUITE(syscoin_asset_allocation_tests, BasicSyscoinTestingSetup)
BOOST_AUTO_TEST_CASE(generate_asset_allocation_address_sync)
{
	UniValue r;
	printf("Running generate_asset_allocation_address_sync...\n");
	GenerateBlocks(5);
	string newaddress = GetNewFundedAddress("node1");
    BOOST_CHECK_NO_THROW(r = CallExtRPC("node1", "getnewaddress"));
    string newaddressreceiver = r.get_str();
	string guid = AssetNew("node1", newaddress, "data", "''", "8", "10000", "1000000");

	AssetSend("node1", guid, "\"[{\\\"address\\\":\\\"" + newaddressreceiver + "\\\",\\\"amount\\\":5000}]\"");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " " + newaddressreceiver ));
	UniValue balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationinfo " + guid + " " + newaddressreceiver ));
	balance = find_value(r.get_obj(), "balance");
	StopNode("node2");
	StartNode("node2");
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " " + newaddressreceiver ));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "assetallocationinfo " + guid + " " + newaddressreceiver ));
	balance = find_value(r.get_obj(), "balance");

}
BOOST_AUTO_TEST_CASE(generate_asset_allocation_send_address)
{
	UniValue r;
	printf("Running generate_asset_allocation_send_address...\n");
	GenerateBlocks(5);
    GenerateBlocks(101, "node2");
	string newaddress1 = GetNewFundedAddress("node1");
    CallRPC("node2", "sendtoaddress " + newaddress1 + " 1", true, false);
    CallRPC("node2", "sendtoaddress " + newaddress1 + " 1", true, false);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
    BOOST_CHECK_NO_THROW(r = CallExtRPC("node1", "getnewaddress"));
    string newaddress2 = r.get_str();
        
	string guid = AssetNew("node1", newaddress1, "data","''", "8", "1", "100000");

	AssetSend("node1", guid, "\"[{\\\"address\\\":\\\"" + newaddress1 + "\\\",\\\"amount\\\":1}]\"");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " " + newaddress1 ));
	UniValue balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8), 1 * COIN);

	string txid0 = AssetAllocationTransfer(false, "node1", guid, newaddress1, "\"[{\\\"address\\\":\\\"" + newaddress2 + "\\\",\\\"amount\\\":0.11}]\"");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " " + newaddress2 ));
	balance = find_value(r.get_obj(), "balance");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8), 0.11 * COIN);

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " " + txid0));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);

	// send using zdag
	string txid1 = AssetAllocationTransfer(true, "node1", guid, newaddress1, "\"[{\\\"address\\\":\\\"" + newaddress2 + "\\\",\\\"amount\\\":0.12}]\"");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " " + newaddress2));
	balance = find_value(r.get_obj(), "balance_zdag");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8), 0.23 * COIN);

	// non zdag cannot be found since it was already mined, but ends up briefly in conflict state because sender is conflicted
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " " + txid0));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MINOR_CONFLICT);

	// first tx should have to wait 1 sec for good status
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " " + txid1));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MINOR_CONFLICT);

	// check just sender
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " ''"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MINOR_CONFLICT);

	// wait for 1 second as required by unit test
	MilliSleep(1000);
	// second send
	string txid2 = AssetAllocationTransfer(true, "node1", guid, newaddress1, "\"[{\\\"address\\\":\\\"" + newaddress2 + "\\\",\\\"amount\\\":0.13}]\"");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationinfo " + guid + " " + newaddress2 ));
	balance = find_value(r.get_obj(), "balance_zdag");
	BOOST_CHECK_EQUAL(AssetAmountFromValue(balance, 8), 0.36 * COIN);

	// sender is conflicted so txid0 is conflicted by extension even if its not found
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " " + txid0));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MINOR_CONFLICT);

	// first ones now OK because it was found explicitely
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " " + txid1));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);

	// second one hasn't waited enough time yet
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " " + txid2));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MINOR_CONFLICT);

	// check just sender
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " ''"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_MINOR_CONFLICT);

	// wait for 1.5 second to clear minor warning status
	MilliSleep(1500);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " " + txid0));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " " + txid1));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);


	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " " + txid2));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);


	// check just sender as well
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " ''"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_STATUS_OK);


	// after pow, should get not found on all of them
	GenerateBlocks(1);

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " " + txid0));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " " + txid1));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);


	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " " + txid2));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);

	// check just sender as well
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "assetallocationsenderstatus " + guid + " " + newaddress1 + " ''"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "status").get_int(), ZDAG_NOT_FOUND);
    ECC_Stop();
}
BOOST_AUTO_TEST_SUITE_END ()
