// Copyright (c) 2016-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "rpc/server.h"
#include "alias.h"
#include "cert.h"
#include "feedback.h"
#include <boost/test/unit_test.hpp>
BOOST_GLOBAL_FIXTURE( SyscoinTestingSetup );

BOOST_FIXTURE_TEST_SUITE (syscoin_offer_tests, BasicSyscoinTestingSetup)


BOOST_AUTO_TEST_CASE (generate_offernew)
{
	printf("Running generate_offernew...\n");
	UniValue r;

	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "selleralias1", "changeddata1");

	// generate a good offer
	string offerguid = OfferNew("node1", "selleralias1", "category", "title", "100", "0.05", "description", "USD");
	// by default offers are not private and should be searchable
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguid), true);

	// should fail: generate an offer with unknown alias
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew fooalias category title 100 0.05 description USD '' SYS false 1 BUYNOW 0 0 false 0 ''"), runtime_error);

	// should fail: generate an offer with negative quantity
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew selleralias1 category title -2 0.05 description USD '' SYS false 1 BUYNOW 0 0 false 0 ''"), runtime_error);

	// should fail: generate an offer too-large category
	string s257bytes = "SdfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddz";
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew selleralias1 " + s257bytes + " title 100 0.05 description USD '' SYS false 1 BUYNOW 0 0 false 0 ''"), runtime_error);	

	// should fail: generate an offer too-large title
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew selleralias1 category " + s257bytes + " 100 0.05 description USD '' SYS false 1 BUYNOW 0 0 false 0 ''"), runtime_error);	

	// should fail: generate an offer too-large description
	string s1025bytes =   "sasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsdz";
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew selleralias1 category title 100 0.05 " + s1025bytes + " USD '' SYS false 1 BUYNOW 0 0 false 0 ''"), runtime_error);

	// should pass: generate an offer with random currency
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offernew selleralias1 category title 100 0.05 description ZZZ '' SYS false 1 BUYNOW 0 0 false 0 ''"));
	// TODO test payment options
}

BOOST_AUTO_TEST_CASE (generate_certoffer)
{
	printf("Running generate_certoffer...\n");
	UniValue r;

	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "node1alias", "node1aliasdata");
	AliasNew("node1", "node1aliasa", "node1aliasdata");
	AliasNew("node2", "node2alias", "node2aliasdata");

	string certguid1  = CertNew("node1", "node1alias", "title", "pubdata");
	string certguid1a  = CertNew("node1", "node1aliasa", "title", "pubdata");
	string certguid2  = CertNew("node2", "node2alias", "title", "pubdata");

	// generate a good cert offer
	string offerguidnoncert = OfferNew("node1", "node1alias", "category", "title", "10", "0.05", "description", "USD");
	string offerguid = OfferNew("node1", "node1alias", "certificates", "title", "1", "0.05", "description", "USD", certguid1);
	string offerguid1 = OfferNew("node1", "node1alias", "certificates-music", "title", "1", "0.05", "description", "USD", certguid1);

	// must use certificates category for certoffer
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew node1alias category title 1 0.05 description USD " + certguid1 + " SYS false 1 BUYNOW 0 0 false 0 ''"), runtime_error);

	// should fail: generate a cert offer using a quantity greater than 1
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew node1alias certificates title 2 0.05 description USD " + certguid1 + " SYS false 1 BUYNOW 0 0 false 0 ''"), runtime_error);

	// should fail: generate a cert offer using a zero quantity
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew  node1alias certificates title 0 0.05 description USD " + certguid1 + " SYS false 1 BUYNOW 0 0 false 0 ''"), runtime_error);

	// should fail: generate a cert offer using an unlimited quantity
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew node1alias certificates title -1 0.05 description USD " + certguid1 + " SYS false 1 BUYNOW 0 0 false 0 ''"), runtime_error);

	// should fail: generate a cert offer using a cert guid you don't own
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew node1alias certificates title 1 0.05 description USD " + certguid2 + " SYS false 1 BUYNOW 0 0 false 0 ''"), runtime_error);

	// should fail: update non cert offer to cert category
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate node1alias " + offerguidnoncert + " certificates title 1 0.15 description USD false '' 0 SYS BUYNOW 0 0 false 0 ''"), runtime_error);

	// should fail: update non cert offer to cert sub category
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate node1alias " + offerguidnoncert + " certificates>music title 1 0.15 description USD false '' 0 SYS BUYNOW 0 0 false 0 ''"), runtime_error);

	// update cert category to sub category of certificates
	OfferUpdate("node1", "node1alias", offerguid, "certificates-music", "titlenew", "1", "0.15", "descriptionnew", "USD", "''", certguid1);

	// should fail: try to change non cert offer to cert offer without cert category
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate node1alias " + offerguidnoncert + " category title 1 0.15 description USD false " + certguid1 + " 0 SYS BUYNOW 0 0 false 0 ''"), runtime_error);

	// change non cert offer to cert offer
	OfferUpdate("node1", "node1alias", offerguidnoncert, "certificates", "titlenew", "1", "0.15", "descriptionnew", "USD", "''", certguid1);


	// generate a cert offer if accepting only BTC
	OfferNew("node1", "node1alias", "certificates", "title", "1", "0.05", "description", "USD", certguid1, "BTC");
	// generate a cert offer if accepting BTC OR SYS
	OfferNew("node1", "node1alias", "certificates", "title", "1", "0.05", "description", "USD", certguid1, "SYS+BTC");

	// should fail: generate a cert offer using different alias for cert and offer
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew node1alias certificates title 1 0.05 description USD " + certguid1a + " SYS false 1 BUYNOW 0 0 false 0 ''"), runtime_error);

	// should fail: generate a cert offer with invalid payment option
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew node1alias certificates title 1 0.05 description USD " + certguid1 + " BTC+SSS false 1 BUYNOW 0 0 false 0 ''"), runtime_error);
}
BOOST_AUTO_TEST_CASE(generate_offerwhitelists)
{
	printf("Running generate_offerwhitelists...\n");
	UniValue r;

	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "sellerwhitelistalias", "changeddata1");
	AliasNew("node2", "selleraddwhitelistalias", "changeddata1");
	AliasNew("node2", "selleraddwhitelistalias1", "changeddata1");
	AliasNew("node3", "sellerwhitelistaliasarbiter", "changeddata1");
	// generate a good offer
	string offerguid = OfferNew("node1", "sellerwhitelistalias", "category", "title", "100", "9.00", "description", "SYS");
	// add to whitelist
	AliasAddWhitelist("node1", "sellerwhitelistalias", "selleraddwhitelistalias", "5");
	// add to whitelist
	AliasAddWhitelist("node1", "sellerwhitelistalias", "selleraddwhitelistalias1", "6");
	// remove from whitelist
	AliasRemoveWhitelist("node1", "sellerwhitelistalias", "selleraddwhitelistalias", "5");
	string whiteListArray = "\"[{\\\"alias\\\":\\\"selleraddwhitelistalias\\\",\\\"discount_percentage\\\":100}]\"";
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdatewhitelist sellerwhitelistalias " + whiteListArray + " ''"), runtime_error);
	whiteListArray = "\"[{\\\"alias\\\":\\\"selleraddwhitelistalias\\\",\\\"discount_percentage\\\":-1}]\"";
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdatewhitelist sellerwhitelistalias " + whiteListArray + " ''"), runtime_error);

	string hex_str = AliasUpdate("node1", "sellerwhitelistalias", "changeddata2");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node2", "selleraddwhitelistalias", "changeddata2");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node2", "selleraddwhitelistalias1", "changeddata2");
	BOOST_CHECK(hex_str.empty());

	// add to whitelist
	AliasAddWhitelist("node1", "sellerwhitelistalias", "selleraddwhitelistalias", "4");
	AliasClearWhitelist("node1", "sellerwhitelistalias");
	BOOST_CHECK_THROW(CallRPC("node1", "aliasclearwhitelist sellerwhitelistalias ''"), runtime_error);

	AliasAddWhitelist("node1", "sellerwhitelistalias", "selleraddwhitelistalias", "6");

	OfferAccept("node1", "node2", "selleraddwhitelistalias1", "sellerwhitelistaliasarbiter", offerguid, "1");

	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress selleraddwhitelistalias1 100"), runtime_error);
	GenerateBlocks(10);

	hex_str = AliasUpdate("node1", "sellerwhitelistalias", "changeddata2");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node2", "selleraddwhitelistalias", "changeddata2");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node2", "selleraddwhitelistalias1", "changeddata2");
	BOOST_CHECK(hex_str.empty());

	AliasRemoveWhitelist("node1", "sellerwhitelistalias", "selleraddwhitelistalias", "6");

	AliasAddWhitelist("node1", "sellerwhitelistalias", "selleraddwhitelistalias", "1");
	OfferAccept("node1", "node2", "selleraddwhitelistalias1", "sellerwhitelistaliasarbiter", offerguid, "1");
	AliasAddWhitelist("node1", "sellerwhitelistalias", "selleraddwhitelistalias1", "2");


	OfferAccept("node1", "node2", "selleraddwhitelistalias1", "sellerwhitelistaliasarbiter", offerguid, "1", "2");
	AliasClearWhitelist("node1", "sellerwhitelistalias");


}
BOOST_AUTO_TEST_CASE (generate_offernew_linkedoffer)
{
	printf("Running generate_offernew_linkedoffer...\n");
	UniValue r;

	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "selleralias5", "changeddata1");
	AliasNew("node2", "selleralias6", "changeddata1");

	// generate a good offer
	string offerguid = OfferNew("node1", "selleralias5", "category", "title", "100", "10.00", "description", "USD");
	AliasAddWhitelist("node1", "selleralias5", "selleralias6", "10");
	string lofferguid = OfferLink("node2", "selleralias6", offerguid, "20", "newdescription");

	// it was already added to whitelist, update it to 5% discount
	AliasAddWhitelist("node1", "selleralias5", "selleralias6", "5");

	// generate a cert offer using a negative percentage bigger or equal to than discount which was set to 5% (uses -5 in calculcation)
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerlink selleralias6 " + offerguid + " -5 newdescription ''"), runtime_error);	


	// should fail: generate a cert offer using too-large pergentage
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerlink selleralias6 " + offerguid + " 101 newdescription ''"), runtime_error);	

	// should fail: generate an offerlink with too-large description
	string s1025bytes =   "sasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsdz";
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerlink selleralias6 " + offerguid + " 5 " + s1025bytes + " ''"), runtime_error);

	// let the offer expire
	ExpireAlias("selleralias6");
	// should fail: try to link against an expired offer
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerlink selleralias6 " + offerguid + " 5 newdescription ''"), runtime_error);
	
}

BOOST_AUTO_TEST_CASE (generate_offernew_linkedofferexmode)
{
	printf("Running generate_offernew_linkedofferexmode...\n");
	UniValue r;

	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "selleralias8", "changeddata1");
	AliasNew("node2", "selleralias9", "changeddata1");

	// generate a good offer 
	string offerguid = OfferNew("node1", "selleralias8", "category", "title", "100", "0.05", "description", "USD");

	// should fail: attempt to create a linked offer for a product without being on the whitelist
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerlink selleralias9 " + offerguid + " 5 newdescription ''"), runtime_error);

	AliasAddWhitelist("node1", "selleralias8", "selleralias9", "5");

	// should succeed: attempt to create a linked offer for a product while being on the whitelist
	OfferLink("node2", "selleralias9", offerguid, "5", "newdescription");
}

BOOST_AUTO_TEST_CASE (generate_offernew_linkedlinkedoffer)
{
	printf("Running generate_offernew_linkedlinkedoffer...\n");
	UniValue r;

	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "selleralias12", "changeddata1");
	AliasNew("node2", "selleralias13", "changeddata1");
	AliasNew("node3", "selleralias14", "changeddata1");

	// generate a good offer
	string offerguid = OfferNew("node1", "selleralias12", "category", "title", "100", "0.05", "description", "USD");
	AliasAddWhitelist("node1", "selleralias12", "selleralias13", "5");
	string lofferguid = OfferLink("node2", "selleralias13", offerguid, "5", "newdescription");

	// should fail: try to generate a linked offer with a linked offer
	BOOST_CHECK_THROW(r = CallRPC("node3", "offerlink selleralias14 " + lofferguid + " 5 newdescription ''"), runtime_error);

}

BOOST_AUTO_TEST_CASE (generate_offerupdate)
{
	printf("Running generate_offerupdate...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "selleralias2", "changeddata1");

	// generate a good offer
	string offerguid = OfferNew("node1", "selleralias2", "category", "title", "100", "0.05", "description", "USD");

	// perform a valid update
	OfferUpdate("node1", "selleralias2", offerguid, "category", "titlenew", "90", "0.15", "descriptionnew");

	// should fail: offer cannot be updated by someone other than owner
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "offerupdate selleralias2 " + offerguid + " category title 90 0.15 description USD false '' 0 SYS BUYNOW 0 0 false 0 ''"));
	UniValue arr = r.get_array();
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "signrawtransaction " + arr[0].get_str()));
	BOOST_CHECK(!find_value(r.get_obj(), "complete").get_bool());

	// should fail: generate an offer with unknown alias
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate fooalias " + offerguid + " category title 90 0.15 description USD false '' 0 SYS BUYNOW 0 0 false 0 ''"), runtime_error);

	// should fail: generate an offer too-large category
	string s257bytes =   "dSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddz";
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate selleralias2 " + offerguid + " " + s257bytes + " title 90 0.15 description USD false '' 0 SYS BUYNOW 0 0 false 0 ''"), runtime_error);	

	// should fail: generate an offer too-large title
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate selleralias2 " + offerguid + " category " + s257bytes + " 90 0.15 description USD false '' 0 SYS BUYNOW 0 0 false 0 ''"), runtime_error);	

	// should fail: generate an offer too-large description
	string s1025ytes =   "dasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsdz";
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate selleralias2 " + offerguid + " category title 90 0.15 " + s1025ytes + " USD false '' 0 SYS BUYNOW 0 0 false 0 ''"), runtime_error);

}

BOOST_AUTO_TEST_CASE (generate_offerupdate_editcurrency)
{
	printf("Running generate_offerupdate_editcurrency...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "selleraliascurrency", "changeddata1");
	AliasNew("node2", "buyeraliascurrency", "changeddata2");
	AliasNew("node3", "arbiteraliascurrency", "changeddata2");
	AliasNew("node3", "arbiteraliascurrency1", "changeddata2");

	// generate a good offer
	string offerguid = OfferNew("node1", "selleraliascurrency", "category", "title", "100", "0.05", "description", "USD");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeraliascurrency 2000"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeraliascurrency 2000"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeraliascurrency 2000"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeraliascurrency 2000"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeraliascurrency 2000"), runtime_error);
	GenerateBlocks(10);
	// accept and confirm payment is accurate with usd
	string escrowguid = OfferAccept("node1", "node2", "buyeraliascurrency", "arbiteraliascurrency", offerguid, "2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + escrowguid));
	CAmount nTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	// 2690.1 SYS/USD
	BOOST_CHECK(abs(nTotal - AmountFromValue(2 * 0.05*2690.1)) <= 0.0001*COIN);
	// perform a valid update
	OfferUpdate("node1", "selleraliascurrency", offerguid, "category", "titlenew", "90", "0.15", "descriptionnew0", "CAD");
	// accept and confirm payment is accurate with cad
	escrowguid = OfferAccept("node1", "node2", "buyeraliascurrency", "arbiteraliascurrency", offerguid, "3");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + escrowguid));
	nTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	// 2698.0 SYS/CAD
	BOOST_CHECK(abs(nTotal - AmountFromValue(3 * 0.15*2698.0)) <= 0.0001*COIN);

	string hex_str = AliasUpdate("node1", "selleraliascurrency", "changeddata2");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node2", "buyeraliascurrency", "changeddata2");
	BOOST_CHECK(hex_str.empty());


	OfferUpdate("node1", "selleraliascurrency", offerguid, "category", "titlenew", "90", "1.00", "descriptionnew1", "SYS");
	// accept and confirm payment is accurate with sys
	escrowguid = OfferAccept("node1", "node2", "buyeraliascurrency", "arbiteraliascurrency", offerguid, "3");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + escrowguid));
	nTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	// 1 SYS/SYS
	BOOST_CHECK(abs(nTotal - AmountFromValue(3)) <= 0.0001*COIN);

	OfferUpdate("node1", "selleraliascurrency", offerguid, "category", "titlenew", "90", "0.00001000", "descriptionnew2", "BTC");
	// accept and confirm payment is accurate with btc
	escrowguid = OfferAccept("node1", "node2", "buyeraliascurrency", "arbiteraliascurrency", offerguid, "4");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + escrowguid));
	nTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	// 100000.0 SYS/BTC
	BOOST_CHECK(abs(nTotal - AmountFromValue(4 * 0.00001000*100000.0)) <= 0.0001*COIN);

	// try to update currency and accept in same block, ensure payment uses old currency not new
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerupdate selleraliascurrency " + offerguid + " category title 93 0.2 desc EUR false '' 0 SYS BUYNOW 0 0 false 0 ''"));
	UniValue arr = r.get_array();
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + arr[0].get_str()));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsendrawtransaction " + find_value(r.get_obj(), "hex").get_str()));

	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "escrownew false buyeraliascurrency arbiteraliascurrency1 " + offerguid + " 10 true 1 0 25 0.005 0 '' SYS 0 0 ''"));
	UniValue arr1 = r.get_array();
	escrowguid = arr1[1].get_str();
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "signrawtransaction " + arr1[0].get_str()));
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "syscoinsendrawtransaction " + find_value(r.get_obj(), "hex").get_str()));
	
	GenerateBlocks(5);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + offerguid));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "currency").get_str(), "EUR");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + escrowguid));
	nTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	// still used BTC conversion amount
	BOOST_CHECK(abs(nTotal - AmountFromValue(10 * 0.00001000*100000.0)) <= 0.0001*COIN);
	// 2695.2 SYS/EUR
	escrowguid = OfferAccept("node1", "node2", "buyeraliascurrency", "arbiteraliascurrency", offerguid, "3");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + escrowguid));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "currency").get_str(), "EUR");
	nTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	BOOST_CHECK(abs(nTotal - AmountFromValue(3 * 0.2*2695.2)) <= 0.0001*COIN);

	// linked offer with root and linked offer changing currencies

	// external payments


}
BOOST_AUTO_TEST_CASE (generate_offeraccept)
{
	printf("Running generate_offeraccept...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "selleralias3", "somedata");
	AliasNew("node2", "buyeralias3", "somedata");
	AliasNew("node3", "arbiteralias3", "somedata");

	// generate a good offer
	string offerguid = OfferNew("node1", "selleralias3", "category", "title", "100", "0.01", "description", "USD");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeralias3 2000"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeralias3 2000"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeralias3 2000"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeralias3 2000"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeralias3 2000"), runtime_error);
	GenerateBlocks(10);
	// perform a valid accept
	string acceptguid = OfferAccept("node1", "node2", "buyeralias3", "arbiteralias3", offerguid, "1");

	
	// perform an accept on negative quantity
	BOOST_CHECK_THROW(r = CallRPC("node2", "escrownew false buyeralias3 arbiteralias3 " + offerguid + " -1 true 0 0 25 0.005 0 '' SYS 0 0 ''"), runtime_error);

	// perform an accept on zero quantity
	BOOST_CHECK_THROW(r = CallRPC("node2", "escrownew false buyeralias3 arbiteralias3 " + offerguid + " 0 true 0 0 25 0.005 0 '' SYS 0 0 ''"), runtime_error);

	// perform an accept on more items than available
	BOOST_CHECK_THROW(r = CallRPC("node2", "escrownew false buyeralias3 arbiteralias3 " + offerguid + " 100 true 0 0 25 0.005 0 '' SYS 0 0 ''"), runtime_error);


}
BOOST_AUTO_TEST_CASE (generate_linkedaccept)
{
	printf("Running generate_linkedaccept...\n");
	UniValue r;

	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "node1aliaslinked", "node1aliasdata");
	AliasNew("node1", "node1aliaslinked1", "node1aliasdata");
	AliasNew("node2", "node2aliaslinked", "node2aliasdata");
	AliasNew("node3", "node3aliaslinked", "node2aliasdata");

	string offerguid = OfferNew("node1", "node1aliaslinked", "category", "title", "10", "0.05", "description", "USD");
	AliasAddWhitelist("node1", "node1aliaslinked" , "*", "0");
	string lofferguid = OfferLink("node2", "node2aliaslinked", offerguid, "20", "newdescription");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "offerinfo " + lofferguid));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "currency").get_str(), "USD");
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "commission").get_int(), 20);
	BOOST_CHECK_EQUAL(((int)(find_value(r.get_obj(), "price").get_real() * 100)), 6);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress node3aliaslinked 965"), runtime_error);
	GenerateBlocks(10);
	OfferAccept("node1", "node3", "node3aliaslinked", "node1aliaslinked1", lofferguid, "6");
}
BOOST_AUTO_TEST_CASE (generate_cert_linkedaccept)
{
	printf("Running generate_cert_linkedaccept...\n");
	UniValue r;

	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "node1aliascert", "node1aliasdata");
	AliasNew("node2", "node2aliascert", "node2aliasdata");
	AliasNew("node3", "node3aliascert", "node2aliasdata");

	string certguid  = CertNew("node1", "node1aliascert", "title", "pubdata");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certguid));
	BOOST_CHECK(find_value(r.get_obj(), "alias").get_str() == "node1aliascert");
	// generate a good cert offer
	string offerguid = OfferNew("node1", "node1aliascert", "certificates", "title", "1", "0.05", "description", "USD", certguid);
	AliasAddWhitelist("node1", "node1aliascert", "*", "0");
	string lofferguid = OfferLink("node2", "node2aliascert", offerguid, "20", "newdescription");

	string hex_str = AliasUpdate("node1", "node1aliascert", "changeddata2");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node2", "node2aliascert", "changeddata2");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node3", "node3aliascert", "changeddata3");
	BOOST_CHECK(hex_str.empty());
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress node3aliascert 1350"), runtime_error);
	GenerateBlocks(10);
	OfferAccept("node1", "node3", "node3aliascert", "node2aliascert", lofferguid, "1");
	GenerateBlocks(5, "node1");
	GenerateBlocks(5, "node3");
}
BOOST_AUTO_TEST_CASE (generate_offerexpired)
{
	printf("Running generate_offerexpired...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "selleralias4", "somedata");
	AliasNew("node2", "buyeralias4", "somedata");
	AliasNew("node3", "arbiteralias4", "somedata");

	// generate a good offer
	string offerguid = OfferNew("node1", "selleralias4", "category", "title", "100", "0.01", "description", "USD");
	AliasAddWhitelist("node1", "selleralias4", "*", "5");
	// this will expire the offer
	ExpireAlias("buyeralias4");

	// should fail: perform an accept on expired offer
	BOOST_CHECK_THROW(r = CallRPC("node2", "escrownew false buyeralias4 arbiteralias4 " + offerguid + " 1 true 0 0 25 0.005 0 '' SYS 0 0 ''"), runtime_error);

	// should fail: offer update on an expired offer
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate selleralias4 " + offerguid + " category title 90 0.15 description USD false '' 0 SYS BUYNOW 0 0 false 0 ''"), runtime_error);
	
	// should fail: link to an expired offer
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerlink buyeralias4 " + offerguid + " 5 newdescription ''"), runtime_error);	
	

}

BOOST_AUTO_TEST_CASE (generate_offerexpiredexmode)
{
	printf("Running generate_offerexpiredexmode...\n");
	UniValue r;

	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "selleralias10", "changeddata1");
	AliasNew("node2", "selleralias11", "changeddata1");

	// generate a good offer 
	string offerguid = OfferNew("node1", "selleralias10", "category", "title", "100", "0.05", "description", "USD");
	// should succeed: offer seller adds affiliate to whitelist
	AliasAddWhitelist("node1", "selleralias10", "selleralias11", "10");
	ExpireAlias("selleralias10");

	// should fail: remove whitelist item from expired offer
	string whiteListArray = "\"{\\\"aliases\\\":[{\\\"alias\\\":\\\"selleralias11\\\",\\\"discount_percentage\\\":10}]}\"";
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdatewhitelist selleralias10 " + whiteListArray + " ''"), runtime_error);
	// should fail: clear whitelist from expired offer
	BOOST_CHECK_THROW(r = CallRPC("node1", "aliasclearwhitelist selleralias10 ''"), runtime_error);
}

BOOST_AUTO_TEST_CASE (generate_certofferexpired)
{
	printf("Running generate_certofferexpired...\n");
	UniValue r;

	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "node1alias2a", "node1aliasdata");
	AliasNew("node1", "node1alias2", "node1aliasdata");
	AliasNew("node2", "node2alias2", "node2aliasdata");
	AliasNew("node3", "node3alias2", "node3aliasdata");

	string certguid  = CertNew("node1", "node1alias2", "title", "pubdata");
	string certguid1  = CertNew("node1", "node1alias2a", "title", "pubdata");

	GenerateBlocks(5);

	// generate a good cert offer
	string offerguid = OfferNew("node1", "node1alias2", "certificates", "title", "1", "0.05", "description", "USD", certguid);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress node2alias2 126"), runtime_error);
	GenerateBlocks(10);
	// updates the alias which updates the offer and cert using this alias
	OfferAccept("node1", "node2", "node2alias2", "node3alias2", offerguid, "1");
	// should pass: generate a cert offer
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offernew node1alias2 certificates title 1 0.05 description USD " + certguid + " SYS false 1 BUYNOW 0 0 false 0 ''"));
	UniValue arr = r.get_array();
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + arr[0].get_str()));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsendrawtransaction " + find_value(r.get_obj(), "hex").get_str()));

	offerguid = OfferNew("node1", "node1alias2a", "certificates", "title", "1", "0.05", "description", "USD", certguid1);
	ExpireAlias("node2alias2");
	// should fail: accept an offer with expired alias
	BOOST_CHECK_THROW(r = CallRPC("node2", "escrownew false node2alias2 node3alias2 " + offerguid + " 1 true 0 0 25 0.005 0 '' SYS 0 0 ''"), runtime_error);
	// should fail: generate a cert offer using an expired cert
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew node1alias2 certificates title 1 0.05 description USD " + certguid1 + " SYS false 1 BUYNOW 0 0 false 0 ''"), runtime_error);
	/// should fail: generate a cert offer using an expired cert
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew node1alias2 certificates title 1 0.05 description USD " + certguid + " SYS false 1 BUYNOW 0 0 false 0 ''"), runtime_error);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
}
BOOST_AUTO_TEST_CASE (generate_offerpruning)
{
	UniValue r;
	// makes sure services expire in 100 blocks instead of 1 year of blocks for testing purposes

	printf("Running generate_offerpruning...\n");
	AliasNew("node1", "pruneoffer", "changeddata1");
	// stop node2 create a service,  mine some blocks to expire the service, when we restart the node the service data won't be synced with node2
	StopNode("node2");
	string guid = OfferNew("node1", "pruneoffer", "category", "title", "1", "0.05", "description", "USD");
	
	// we can find it as normal first
	BOOST_CHECK_EQUAL(OfferFilter("node1", guid), true);
	GenerateBlocks(5, "node1");
	// then we let the service expire
	ExpireAlias("pruneoffer");
	StartNode("node2");
	GenerateBlocks(5, "node2");
	// it can still be found via search because node hasn't restarted and pruned itself
	BOOST_CHECK_EQUAL(OfferFilter("node1", guid), true);

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + guid));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), true);

	// should be pruned
	BOOST_CHECK_THROW(CallRPC("node2", "offerinfo " + guid), runtime_error);

	// stop node3
	StopNode("node3");
	// should fail: already expired alias
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdate pruneoffer newdata TTVgyEvCfgZFiVL32kD7jMRaBKtGCHqwbD 0 '' '' ''"), runtime_error);
	GenerateBlocks(5, "node1");
	
	// stop and start node1
	StopNode("node1");
	StartNode("node1");
	GenerateBlocks(5, "node1");

	// after stopping and starting, indexer should remove guid offer from db
	BOOST_CHECK_EQUAL(OfferFilter("node1", guid), false);

	// create a new service
	AliasNew("node1", "pruneoffer", "data");
	string guid1 = OfferNew("node1", "pruneoffer", "category", "title", "1", "0.05", "description", "USD");

	// ensure you can still update before expiry
	OfferUpdate("node1", "pruneoffer", guid1, "category", "title", "1", "0.05", "description", "USD");
	// you can search it still on node1/node2
	BOOST_CHECK_EQUAL(OfferFilter("node1", guid1), true);
	BOOST_CHECK_EQUAL(OfferFilter("node2", guid1), true);
	
	GenerateBlocks(5, "node1");
	// make sure our offer alias doesn't expire
	string hex_str = AliasUpdate("node1", "pruneoffer");
	BOOST_CHECK(hex_str.empty());
	ExpireAlias("pruneoffer");
	// now it should be expired
	BOOST_CHECK_THROW(CallRPC("node1", "offerupdate pruneoffer " + guid1 + " category title 1 0.05 description USD false '' 0 SYS BUYNOW 0 0 false 0 ''"), runtime_error);
	// can still search
	BOOST_CHECK_EQUAL(OfferFilter("node1", guid1), true);
	BOOST_CHECK_EQUAL(OfferFilter("node2", guid1), true);
	// and it should say its expired
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "offerinfo " + guid1));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(),true);	
	GenerateBlocks(5, "node1");
	StartNode("node3");
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "offerinfo " + guid1), runtime_error);
 	BOOST_CHECK_EQUAL(OfferFilter("node3", guid1), false);

}

BOOST_AUTO_TEST_SUITE_END ()