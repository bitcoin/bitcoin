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

	AliasNew("node1", "selleralias", "changeddata1");

	// generate a good offer
	string offerguid = OfferNew("node1", "selleralias", "category", "title", "100", "0.05", "description", "USD");

	// should fail: generate an offer with unknown alias
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew fooalias category title 100 0.05 description USD"), runtime_error);

	// should fail: generate an offer with negative quantity
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew selleralias category title -1 0.05 description USD"), runtime_error);

	// should fail: generate an offer with zero quantity
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew selleralias category title 0 0.05 description USD"), runtime_error);

	// should fail: generate an offer with zero price
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew selleralias category title 100 0 description USD"), runtime_error);

	// should fail: generate an offer with negative price
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew selleralias category title 100 -0.05 description USD"), runtime_error);

	// should fail: generate an offer too-large category
	string s256bytes =   "SfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddz";
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew selleralias " + s256bytes + " title 100 0.05 description USD"), runtime_error);	

	// should fail: generate an offer too-large title
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew selleralias category " + s256bytes + " 100 0.05 description USD"), runtime_error);	

	// should fail: generate an offer too-large description
	string s1024bytes =   "asdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsdz";
	BOOST_CHECK_THROW(r = CallRPC("node1", "offernew selleralias category title 100 0.05 " + s1024bytes + " USD"), runtime_error);
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
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerupdate selleralias2 " + offerguid + " category title 90 0.15 description"), runtime_error);

	// should fail: generate an offer with unknown alias
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate fooalias " + offerguid + " category title 90 0.15 description"), runtime_error);

	// should fail: generate an offer with negative quantity
	//BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate selleralias2 " + offerguid + " category title -1 0.05 description"), runtime_error);

	// should fail: generate an offer with zero quantity
	//BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate selleralias2 " + offerguid + " category title 0 0.05 description"), runtime_error);

	// should fail: generate an offer with zero price
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate selleralias2 " + offerguid + " category title 90 0 description"), runtime_error);

	// should fail: generate an offer with negative price
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate selleralias2 " + offerguid + " category title 90 -0.05 description"), runtime_error);

	// should fail: generate an offer too-large category
	string s256bytes =   "SfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddz";
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate selleralias2 " + offerguid + " " + s256bytes + " title 90 0.15 description"), runtime_error);	

	// should fail: generate an offer too-large title
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate selleralias2 " + offerguid + " category " + s256bytes + " 90 0.15 description"), runtime_error);	

	// should fail: generate an offer too-large description
	string s1024bytes =   "asdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsdz";
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerupdate selleralias2 " + offerguid + " category title 90 0.15 " + s1024bytes), runtime_error);
}


BOOST_AUTO_TEST_SUITE_END ()