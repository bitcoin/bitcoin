// Copyright (c) 2016-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "util.h"
#include "rpc/server.h"
#include "alias.h"
#include "cert.h"
#include "base58.h"
#include "chainparams.h"
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
BOOST_GLOBAL_FIXTURE( SyscoinTestingSetup );

BOOST_FIXTURE_TEST_SUITE (syscoin_alias_tests, BasicSyscoinTestingSetup)
const unsigned int MAX_ALIAS_UPDATES_PER_BLOCK = 5;
BOOST_AUTO_TEST_CASE (generate_big_aliasdata)
{
	UniValue r;
	ECC_Start();
		// rate converstion to SYS
	pegRates["USD"] = 2690.1;
	pegRates["EUR"] = 2695.2;
	pegRates["GBP"] = 2697.3;
	pegRates["CAD"] = 2698.0;
	pegRates["BTC"] = 100000.0;
	pegRates["ZEC"] = 10000.0;
	pegRates["SYS"] = 1.0;
	printf("Running generate_big_aliasdata...\n");
	GenerateBlocks(200,"node1");
	GenerateBlocks(200,"node2");
	GenerateBlocks(200,"node3");
	// 512 bytes long
	string gooddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddSfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	string baddata = gooddata + "a";
	AliasNew("node1", "jag1", gooddata);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasnew jag2 " + baddata + " 3 0 TPbBz99JatywT2BUTFzDYFHXBGWz5be3bw '' '' ''"));
	UniValue varray = r.get_array();
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + varray[0].get_str()));
	BOOST_CHECK_NO_THROW(CallRPC("node1", "syscoinsendrawtransaction " + find_value(r.get_obj(), "hex").get_str()));
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew jag2 pub 3 0 TTVgyEvCfgZFiVL32kD7jMRaBKtGCHqwbD '' '' ''"), runtime_error);
	GenerateBlocks(5);	
}
BOOST_AUTO_TEST_CASE (generate_aliaswitness)
{
	printf("Running generate_aliaswitness...\n");
	GenerateBlocks(5);
	UniValue r;
	AliasNew("node1", "witness1", "pub");
	AliasNew("node2", "witness2", "pub");
	string hex_str = AliasUpdate("node1", "witness1", "newpubdata", "''", "witness2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo witness1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str(), "pub");
	BOOST_CHECK(!hex_str.empty());
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "signrawtransaction " + hex_str));
	BOOST_CHECK_NO_THROW(CallRPC("node2", "syscoinsendrawtransaction " + find_value(r.get_obj(), "hex").get_str()));
	GenerateBlocks(5, "node2");
	GenerateBlocks(5);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo witness1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str(), "newpubdata");
}
BOOST_AUTO_TEST_CASE (generate_big_aliasname)
{
	printf("Running generate_big_aliasname...\n");
	GenerateBlocks(5);
	// 64 bytes long
	string goodname = "sfsdfdfsdsfsfsdfdfsdsfdsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdsfsdfdd";
	// 256 bytes long
	string gooddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";	
	// 65 bytes long
	string badname =  "sfsdfdfsdsfsfsdfdfsdsfdsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdsfsddfda";
	AliasNew("node1", goodname, gooddata);
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew " + badname + " 3d 3 0 TWnXcTHMiKtZME84Y8YA5DwXtdYBAZ5SVc '' '' ''"), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_aliasupdate)
{
	printf("Running generate_aliasupdate...\n");
	GenerateBlocks(1);
	AliasNew("node1", "jagupdate", "data");
	AliasNew("node1", "jagupdate1", "data");
	// update an alias that isn't yours
	string hex_str = AliasUpdate("node2", "jagupdate");
	BOOST_CHECK(!hex_str.empty());
	// only update alias, no data
	hex_str = AliasUpdate("node1", "jagupdate");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node1", "jagupdate1");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node1", "jagupdate", "newpub");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node1", "jagupdate", "newpub1");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node1", "jagupdate1", "newpub2");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node1", "jagupdate1", "newpub3");
	BOOST_CHECK(hex_str.empty());

}

BOOST_AUTO_TEST_SUITE_END ()