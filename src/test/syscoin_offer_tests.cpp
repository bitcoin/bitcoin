#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "rpcserver.h"
#include <boost/test/unit_test.hpp>
BOOST_GLOBAL_FIXTURE( SyscoinTestingSetup );

BOOST_FIXTURE_TEST_SUITE (syscoin_offer_tests, BasicSyscoinTestingSetup)


BOOST_AUTO_TEST_CASE (generate_offernew)
{
	UniValue r;

	GenerateBlocks(200);
	GenerateBlocks(200, "node2");
	GenerateBlocks(200, "node3");

	AliasNew("node1", "selleralias1", "changeddata1");

	// generate a good offer
	string offerguid = OfferNew("node1", "selleralias1", "category", "title", "100", "0.05", "description", "USD");

	// should fail: generate an offer with unknown alias
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew SYS_RATES fooalias category title 100 0.05 description USD"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew_nocheck fooalias category title 100 0.05 description USD"), runtime_error);

	// should fail: generate an offer with negative quantity
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew SYS_RATES selleralias1 category title -2 0.05 description USD"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew_nocheck selleralias1 category title -2 0.05 description USD"), runtime_error);

	// should fail: generate an offer with zero price
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew SYS_RATES selleralias1 category title 100 0 description USD"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew_nocheck selleralias1 category title 100 0 description USD"), runtime_error);

	// should fail: generate an offer with negative price
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew SYS_RATES selleralias1 category title 100 -0.05 description USD"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew_nocheck selleralias1 category title 100 -0.05 description USD"), runtime_error);

	// should fail: generate an offer too-large category
	string s256bytes = "SfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddz";
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew SYS_RATES selleralias1 " + s256bytes + " title 100 0.05 description USD"), runtime_error);	
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew_nocheck selleralias1 " + s256bytes + " title 100 0.05 description USD"), runtime_error);	

	// should fail: generate an offer too-large title
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew SYS_RATES selleralias1 category " + s256bytes + " 100 0.05 description USD"), runtime_error);	
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew_nocheck selleralias1 category " + s256bytes + " 100 0.05 description USD"), runtime_error);

	// should fail: generate an offer too-large description
	string s1024bytes =   "asdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsdz";
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew SYS_RATES selleralias1 category title 100 0.05 " + s1024bytes + " USD"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew_nocheck selleralias1 category title 100 0.05 " + s1024bytes + " USD"), runtime_error);

	// should fail: generate an offer with invalid currency
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew SYS_RATES selleralias1 category title 100 0.05 description ZZZ"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew_nocheck selleralias1 category title 100 0.05 description ZZZ"), runtime_error);
}

BOOST_AUTO_TEST_CASE (generate_certoffernew)
{
	UniValue r;

	GenerateBlocks(200);
	GenerateBlocks(200, "node2");
	GenerateBlocks(200, "node3");

	AliasNew("node1", "node1alias", "node1aliasdata");
	AliasNew("node2", "node2alias", "node2aliasdata");

	string certguid1  = CertNew("node1", "node1alias", "title", "data");
	string certguid1a = CertNew("node1", "node1alias", "title", "data");
	string certguid2  = CertNew("node2", "node2alias", "title", "data");

	// generate a good cert offer
	string offerguid = OfferNew("node1", "node1alias", "category", "title", "1", "0.05", "description", "USD", certguid1);

	// should fail: generate a cert offer using a quantity greater than 1
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew SYS_RATES node1alias category title 2 0.05 description USD " + certguid1a), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew_nocheck node1alias category title 2 0.05 description USD " + certguid1a), runtime_error);

	// should fail: generate a cert offer using a zero quantity
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew SYS_RATES node1alias category title 0 0.05 description USD " + certguid1a), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew_nocheck node1alias category title 0 0.05 description USD " + certguid1a), runtime_error);

	// should fail: generate a cert offer using an unlimited quantity
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew SYS_RATES node1alias category title -1 0.05 description USD " + certguid1a), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew_nocheck node1alias category title -1 0.05 description USD " + certguid1a), runtime_error);

	// should fail: generate a cert offer using a cert guid you don't own
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew SYS_RATES node1alias category title 1 0.05 description USD " + certguid2), runtime_error);	
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew_nocheck node1alias category title 1 0.05 description USD " + certguid2), runtime_error);	

	// should fail: generate a cert offer if accepting only BTC
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew SYS_RATES node1alias category title 1 0.05 description USD " + certguid1a + " 0 1"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew_nocheck node1alias category title 1 0.05 description USD " + certguid1a + " 0 1"), runtime_error);

	// should fail: generate a cert offer using different public keys for cert and alias
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew_nocheck node1alias category title 1 0.05 description USD " + certguid1a + " 0 0 1"), runtime_error);
}

BOOST_AUTO_TEST_CASE (generate_offernew_linkedoffer)
{
	UniValue r;

	GenerateBlocks(200);
	GenerateBlocks(200, "node2");
	GenerateBlocks(200, "node3");

	AliasNew("node1", "selleralias5", "changeddata1");
	AliasNew("node2", "selleralias6", "changeddata1");

	// generate a good offer
	string offerguid = OfferNew("node1", "selleralias5", "category", "title", "100", "0.05", "description", "USD");
	string lofferguid = OfferLink("node2", "selleralias6", offerguid, "5", "newdescription");

	// should fail: generate a cert offer using a negative percentage
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerlink selleralias6 " + offerguid + " -5 newdescription"), runtime_error);	
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerlink_nocheck selleralias6 " + offerguid + " -5 newdescription"), runtime_error);	

	// should fail: generate a cert offer using too-large pergentage
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerlink selleralias6 " + offerguid + " 256 newdescription"), runtime_error);	
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerlink_nocheck selleralias6 " + offerguid + " 256 newdescription"), runtime_error);	

	// should fail: generate an offerlink with too-large description
	string s1024bytes =   "asdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsdz";
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerlink selleralias6 " + offerguid + " 5 " + s1024bytes), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerlink_nocheck selleralias6 " + offerguid + " 5 " + s1024bytes), runtime_error);
}

BOOST_AUTO_TEST_CASE (generate_offernew_linkedofferexmode)
{
	UniValue r;

	GenerateBlocks(200);
	GenerateBlocks(200, "node2");
	GenerateBlocks(200, "node3");

	AliasNew("node1", "selleralias8", "changeddata1");
	AliasNew("node2", "selleralias9", "changeddata1");

	// generate a good offer in exclusive mode
	string offerguid = OfferNew("node1", "selleralias8", "category", "title", "100", "0.05", "description", "USD", "", true);

	// should fail: attempt to create a linked offer for an exclusive mode product without being on the whitelist
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerlink selleralias9 " + offerguid + " 5 newdescription"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerlink_nocheck selleralias9 " + offerguid + " 5 newdescription"), runtime_error);

	// should succeed: offer seller adds affiliate to whitelist
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offeraddwhitelist " + offerguid + " selleralias9 10"));
	GenerateBlocks(10);

	// should succeed: attempt to create a linked offer for an exclusive mode product while being on the whitelist
	OfferLink("node2", "selleralias9", offerguid, "5", "newdescription");
}

BOOST_AUTO_TEST_CASE (generate_offernew_linkedlinkedoffer)
{
	UniValue r;

	GenerateBlocks(200);
	GenerateBlocks(200, "node2");
	GenerateBlocks(200, "node3");

	AliasNew("node1", "selleralias12", "changeddata1");
	AliasNew("node2", "selleralias13", "changeddata1");
	AliasNew("node3", "selleralias14", "changeddata1");

	// generate a good offer
	string offerguid = OfferNew("node1", "selleralias12", "category", "title", "100", "0.05", "description", "USD");
	string lofferguid = OfferLink("node2", "selleralias13", offerguid, "5", "newdescription");

	// should fail: try to generate a linked offer with a linked offer
	BOOST_CHECK_THROW(r = CallRPC("node3", "offerlink selleralias14 " + lofferguid + " 5 newdescription"), runtime_error);	
	BOOST_CHECK_THROW(r = CallRPC("node3", "offerlink_nocheck selleralias14 " + lofferguid + " 5 newdescription"), runtime_error);	
}

BOOST_AUTO_TEST_CASE (generate_offerupdate)
{
	UniValue r;
	
	GenerateBlocks(200);
	GenerateBlocks(200, "node2");
	GenerateBlocks(200, "node3");

	AliasNew("node1", "selleralias2", "changeddata1");

	// generate a good offer
	string offerguid = OfferNew("node1", "selleralias2", "category", "title", "100", "0.05", "description", "USD");

	// perform a valid update
	OfferUpdate("node1", "selleralias2", offerguid, "category", "titlenew", "90", "0.15", "descriptionnew");

	// should fail: offer cannot be updated by someone other than owner
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerupdate SYS_RATES selleralias2 " + offerguid + " category title 90 0.15 description"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerupdate_nocheck selleralias2 " + offerguid + " category title 90 0.15 description"), runtime_error);

	// should fail: generate an offer with unknown alias
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate SYS_RATES fooalias " + offerguid + " category title 90 0.15 description"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate_nocheck fooalias " + offerguid + " category title 90 0.15 description"), runtime_error);

	// should fail: generate an offer with zero price
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate SYS_RATES selleralias2 " + offerguid + " category title 90 0 description"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate_nocheck selleralias2 " + offerguid + " category title 90 0 description"), runtime_error);

	// should fail: generate an offer with negative price
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate SYS_RATES selleralias2 " + offerguid + " category title 90 -0.05 description"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate_nocheck selleralias2 " + offerguid + " category title 90 -0.05 description"), runtime_error);

	// should fail: generate an offer too-large category
	string s256bytes =   "SfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddz";
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate SYS_RATES selleralias2 " + offerguid + " " + s256bytes + " title 90 0.15 description"), runtime_error);	
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate_nocheck selleralias2 " + offerguid + " " + s256bytes + " title 90 0.15 description"), runtime_error);	

	// should fail: generate an offer too-large title
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate SYS_RATES selleralias2 " + offerguid + " category " + s256bytes + " 90 0.15 description"), runtime_error);	
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate_nocheck selleralias2 " + offerguid + " category " + s256bytes + " 90 0.15 description"), runtime_error);	

	// should fail: generate an offer too-large description
	string s1024bytes =   "asdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsdz";
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate SYS_RATES selleralias2 " + offerguid + " category title 90 0.15 " + s1024bytes), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate_nocheck selleralias2 " + offerguid + " category title 90 0.15 " + s1024bytes), runtime_error);
}

BOOST_AUTO_TEST_CASE (generate_offeraccept)
{
	UniValue r;
	
	GenerateBlocks(200);
	GenerateBlocks(200, "node2");
	GenerateBlocks(200, "node3");

	AliasNew("node1", "selleralias3", "somedata");
	AliasNew("node2", "buyeralias3", "somedata");

	// generate a good offer
	string offerguid = OfferNew("node1", "selleralias3", "category", "title", "100", "0.01", "description", "USD");

	// perform a valid accept
	string acceptguid = OfferAccept("node1", "node2", "buyeralias3", offerguid, "1", "message");

	// should fail: generate an offer accept with too-large message
	string s1024bytes =   "asdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsdz";
	BOOST_CHECK_THROW(r = CallRPC("node2", "offeraccept buyeralias3 " + offerguid + " 1 " + s1024bytes), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node2", "offeraccept_nocheck buyeralias3 " + offerguid + " 1 " + s1024bytes), runtime_error);

	// perform an accept on more items than available
	BOOST_CHECK_THROW(r = CallRPC("node2", "offeraccept buyeralias3 " + offerguid + " 100 message"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node2", "offeraccept_nocheck buyeralias3 " + offerguid + " 100 message"), runtime_error);

	// perform an accept on negative quantity
	BOOST_CHECK_THROW(r = CallRPC("node2", "offeraccept buyeralias3 " + offerguid + " -1 message"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node2", "offeraccept_nocheck buyeralias3 " + offerguid + " -1 message"), runtime_error);

	// perform an accept on zero quantity
	BOOST_CHECK_THROW(r = CallRPC("node2", "offeraccept buyeralias3 " + offerguid + " 0 message"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node2", "offeraccept_nocheck buyeralias3 " + offerguid + " 0 message"), runtime_error);
}

BOOST_AUTO_TEST_CASE (generate_offerexpired)
{
	UniValue r;
	
	GenerateBlocks(200);
	GenerateBlocks(200, "node2");
	GenerateBlocks(200, "node3");

	AliasNew("node1", "selleralias4", "somedata");
	AliasNew("node2", "buyeralias4", "somedata");

	// generate a good offer
	string offerguid = OfferNew("node1", "selleralias4", "category", "title", "100", "0.01", "description", "USD");

	// this will expire the offer
	GenerateBlocks(100);

	// should fail: perform an accept on expired offer
	BOOST_CHECK_THROW(r = CallRPC("node2", "offeraccept buyeralias4 " + offerguid + " 1 message"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node2", "offeraccept_nocheck buyeralias4 " + offerguid + " 1 message"), runtime_error);

	// should fail: offer update on an expired offer
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate SYS_RATES selleralias4 " + offerguid + " category title 90 0.15 description"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate_nocheck selleralias4 " + offerguid + " category title 90 0.15 description"), runtime_error);

	// should fail: link to an expired offer
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerlink buyeralias4 " + offerguid + " 5 newdescription"), runtime_error);	
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerlink_nocheck buyeralias4 " + offerguid + " 5 newdescription"), runtime_error);	
}

BOOST_AUTO_TEST_CASE (generate_offerexpiredexmode)
{
	UniValue r;

	GenerateBlocks(200);
	GenerateBlocks(200, "node2");
	GenerateBlocks(200, "node3");

	AliasNew("node1", "selleralias10", "changeddata1");
	AliasNew("node2", "selleralias11", "changeddata1");

	// generate a good offer in exclusive mode
	string offerguid = OfferNew("node1", "selleralias10", "category", "title", "100", "0.05", "description", "USD", "", true);

	// should succeed: offer seller adds affiliate to whitelist
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offeraddwhitelist " + offerguid + " selleralias11 10"));

	// this will expire the offer
	GenerateBlocks(100);

	// should fail: remove whitelist item from expired offer
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerremovewhitelist " + offerguid + " selleralias11"), runtime_error);

	// should fail: clear whitelist from expired offer
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerclearwhitelist " + offerguid), runtime_error);
}

BOOST_AUTO_TEST_CASE (generate_certofferexpired)
{
	UniValue r;

	GenerateBlocks(200);
	GenerateBlocks(200, "node2");
	GenerateBlocks(200, "node3");

	AliasNew("node1", "node1alias2", "node1aliasdata");
	AliasNew("node2", "node2alias2", "node2aliasdata");

	string certguid  = CertNew("node1", "node1alias2", "title", "data");

	// this leaves 50 blocks remaining before cert expires
	GenerateBlocks(40);

	// generate a good cert offer - after this, 40 blocks to cert expire
	string offerguid = OfferNew("node1", "node1alias2", "category", "title", "1", "0.05", "description", "USD", certguid);

	// this expires the cert but not the offer
	GenerateBlocks(50);

	// should fail: offer update on offer with expired cert
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate SYS_RATES node1alias2 " + offerguid + " category title 1 0.05 newdescription"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate_nocheck SYS_RATES node1alias2 " + offerguid + " category title 1 0.05 newdescription"), runtime_error);

	// should fail: offer accept on offer with expired cert
	BOOST_CHECK_THROW(r = CallRPC("node2", "offeraccept node2alias2 " + offerguid + " 1 message"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node2", "offeraccept_nocheck node2alias2 " + offerguid + " 1 message"), runtime_error);	

	// should fail: generate a cert offer using an expired cert
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew SYS_RATES node1alias2 category title 1 0.05 description USD " + certguid), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew_nocheck node1alias2 category title 1 0.05 description USD " + certguid), runtime_error);	
}

BOOST_AUTO_TEST_SUITE_END ()