#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "rpcserver.h"
#include <boost/test/unit_test.hpp>
BOOST_FIXTURE_TEST_SUITE (syscoin_escrow_tests, BasicSyscoinTestingSetup)

BOOST_AUTO_TEST_CASE (generate_escrow_release)
{
	UniValue r;
	GenerateBlocks(200);
	GenerateBlocks(200, "node2");
	GenerateBlocks(200, "node3");
	AliasNew("node1", "buyeralias", "changeddata1");
	AliasNew("node2", "selleralias", "changeddata2");
	AliasNew("node3", "arbiteralias", "changeddata3");
	string qty = "3";
	string message = "paymentmessage";
	string offerguid = OfferNew("node2", "selleralias", "category", "title", "100", "0.05", "description", "USD");
	string guid = EscrowNew("node1", "buyeralias", offerguid, qty, message, "arbiteralias", "selleralias");
	EscrowRelease("node1", guid);	
	EscrowClaimRelease("node2", guid);
}
BOOST_AUTO_TEST_CASE (generate_escrow_big)
{
	UniValue r;
	// 255 bytes long
	string goodname1 = "SgsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	string goodname2 = "TgsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	string goodname3 = "UgsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	// 1023 bytes long
	string gooddata = "agdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsd";	
	// 1024 bytes long
	string baddata = "azsdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsd";	
				
	AliasNew("node1", goodname1, "changeddata1");
	AliasNew("node2", goodname2, "changeddata2");
	AliasNew("node3", goodname3, "changeddata3");
	string qty = "3";

	string offerguid = OfferNew("node2", goodname2, "category", "title", "100", "0.05", "description", "USD");
	// payment message too long
	BOOST_CHECK_THROW(r = CallRPC("node1", "escrownew " + goodname1 + " " + offerguid + " " + qty + " " + baddata + " " + goodname3), runtime_error);
	string guid = EscrowNew("node1", goodname1, offerguid, qty, gooddata, goodname3, goodname2);
	EscrowRelease("node1", guid);	
	EscrowClaimRelease("node2", guid);
}
BOOST_AUTO_TEST_CASE (generate_escrowrefund_seller)
{
	string qty = "4";
	string message = "paymentmessage";
	string offerguid = OfferNew("node2", "selleralias", "category", "title", "100", "1.22", "description", "CAD");
	string guid = EscrowNew("node1", "buyeralias", offerguid, qty, message, "arbiteralias", "selleralias");
	EscrowRefund("node2", guid);
	EscrowRefund("node1", guid);
}
BOOST_AUTO_TEST_CASE (generate_escrowrefund_arbiter)
{
	string qty = "5";
	string offerguid = OfferNew("node2", "selleralias", "category", "title", "100", "0.25", "description", "EUR");
	string message = "paymentmessage";
	string guid = EscrowNew("node1", "buyeralias", offerguid, qty, message, "arbiteralias", "selleralias");
	EscrowRefund("node3", guid);
	EscrowRefund("node1", guid);
}
BOOST_AUTO_TEST_CASE (generate_escrowrefund_invalid)
{
	string qty = "2";
	string offerguid = OfferNew("node2", "selleralias", "category", "title", "100", "1.45", "description", "EUR");
	string guid = EscrowNew("node1", "buyeralias", offerguid, qty, "message", "arbiteralias", "selleralias");
	// try to claim refund even if not refunded
	BOOST_CHECK_THROW(CallRPC("node2", "escrowclaimrefund " + guid), runtime_error);
	// buyer cant refund to himself
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrefund " + guid), runtime_error);
	EscrowRefund("node2", guid);
	// try to release already refunded escrow
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrelease " + guid), runtime_error);
	// cant refund already refunded escrow
	BOOST_CHECK_THROW(CallRPC("node2", "escrowrefund " + guid), runtime_error);
	// noone other than buyer can claim release
	BOOST_CHECK_THROW(CallRPC("node3", "escrowclaimrefund " + guid), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node2", "escrowclaimrefund " + guid), runtime_error);
	EscrowRefund("node1", guid);
	// cant inititate another refund after claimed already
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrefund " + guid), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_escrowrelease_invalid)
{
	string qty = "4";
	string offerguid = OfferNew("node2", "selleralias", "category", "title", "100", "1.45", "description", "SYS");
	string guid = EscrowNew("node1", "buyeralias", offerguid, qty, "message", "arbiteralias", "selleralias");
	// try to claim release even if not released
	BOOST_CHECK_THROW(CallRPC("node2", "escrowclaimrelease " + guid), runtime_error);
	// seller cant release buyers funds
	BOOST_CHECK_THROW(CallRPC("node2", "escrowrelease " + guid), runtime_error);
	EscrowRelease("node1", guid);
	// try to refund already released escrow
	BOOST_CHECK_THROW(CallRPC("node2", "escrowrefund " + guid), runtime_error);
	// cant release already released escrow
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrelease " + guid), runtime_error);
	// noone other than seller can claim release
	BOOST_CHECK_THROW(CallRPC("node3", "escrowclaimrelease " + guid), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "escrowclaimrelease " + guid), runtime_error);
	EscrowClaimRelease("node2", guid);
	// cant inititate another release after claimed already
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrelease " + guid), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_escrowrelease_arbiter)
{
	GenerateBlocks(200);
	GenerateBlocks(200, "node2");
	GenerateBlocks(200, "node3");
	UniValue r;
	string qty = "1";
	string offerguid = OfferNew("node2", "selleralias", "category", "title", "100", "0.05", "description", "GBP");
	string guid = EscrowNew("node1", "buyeralias", offerguid, qty, "message", "arbiteralias", "selleralias");
	EscrowRelease("node3", guid);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + guid));
	CAmount escrowfee = AmountFromValue(find_value(r.get_obj(), "sysfee"));
	// get arbiter balance (ensure he gets escrow fees, since he stepped in and released)
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "getinfo"));
	CAmount balanceBeforeArbiter = AmountFromValue(find_value(r.get_obj(), "balance"));
	EscrowClaimRelease("node2", guid);
	// get arbiter balance after release
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "getinfo"));
	// 10 mined block subsidy + escrow fee
	balanceBeforeArbiter += 10*0.545*COIN + escrowfee;
	CAmount balanceAfterArbiter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceBeforeArbiter, balanceAfterArbiter);


}
BOOST_AUTO_TEST_CASE (generate_escrow_linked_release)
{
	GenerateBlocks(200);
	GenerateBlocks(200, "node2");
	GenerateBlocks(200, "node3");
	string qty = "3";
	string message = "paymentmessage";
	string offerguid = OfferNew("node2", "selleralias", "category", "title", "100", "0.04", "description", "EUR");
	string commission = "10";
	string description = "newdescription";
	// offer should be set to exclusive mode by default so linking isn't allowed
	BOOST_CHECK_THROW(CallRPC("node3", "offerlink arbiteralias " + offerguid + " " + commission + " " + description), runtime_error);
	offerguid = OfferNew("node2", "selleralias", "category", "title", "100", "0.04", "description", "EUR", "", false);
	string offerlinkguid = OfferLink("node3", "arbiteralias", offerguid, commission, description);
	string guid = EscrowNew("node1", "buyeralias", offerlinkguid, qty, message, "arbiteralias", "selleralias");
	EscrowRelease("node1", guid);
	// reseller cant claim escrow, seller must claim it
	BOOST_CHECK_THROW(CallRPC("node3", "escrowclaimrelease " + guid), runtime_error);
	EscrowClaimReleaseLink("node2", guid, "node3");
}
BOOST_AUTO_TEST_CASE (generate_escrow_linked_release_with_peg_update)
{
	GenerateBlocks(100);
	GenerateBlocks(100, "node2");
	GenerateBlocks(100, "node3");
	string qty = "3";
	string message = "paymentmessage";
	string offerguid = OfferNew("node2", "selleralias", "category", "title", "100", "0.05", "description", "EUR", "", false);
	string commission = "3";
	string description = "newdescription";
	string offerlinkguid = OfferLink("node3", "arbiteralias", offerguid, commission, description);
	string guid = EscrowNew("node1", "buyeralias", offerlinkguid, qty, message, "arbiteralias", "selleralias");
	EscrowRelease("node1", guid);
	// update the EUR peg twice before claiming escrow
	string data = "{\\\"rates\\\":[{\\\"currency\\\":\\\"USD\\\",\\\"rate\\\":2690.1,\\\"precision\\\":2},{\\\"currency\\\":\\\"EUR\\\",\\\"rate\\\":269.2,\\\"precision\\\":2},{\\\"currency\\\":\\\"GBP\\\",\\\"rate\\\":2697.3,\\\"precision\\\":2},{\\\"currency\\\":\\\"CAD\\\",\\\"rate\\\":2698.0,\\\"precision\\\":2},{\\\"currency\\\":\\\"BTC\\\",\\\"rate\\\":100000.0,\\\"precision\\\":8},{\\\"currency\\\":\\\"SYS\\\",\\\"rate\\\":1.0,\\\"precision\\\":2}]}";
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate SYS_RATES " + data));
	GenerateBlocks(100);
	GenerateBlocks(100, "node2");
	GenerateBlocks(100, "node3");
	data = "{\\\"rates\\\":[{\\\"currency\\\":\\\"USD\\\",\\\"rate\\\":2690.1,\\\"precision\\\":2},{\\\"currency\\\":\\\"EUR\\\",\\\"rate\\\":218.2,\\\"precision\\\":2},{\\\"currency\\\":\\\"GBP\\\",\\\"rate\\\":2697.3,\\\"precision\\\":2},{\\\"currency\\\":\\\"CAD\\\",\\\"rate\\\":2698.0,\\\"precision\\\":2},{\\\"currency\\\":\\\"BTC\\\",\\\"rate\\\":100000.0,\\\"precision\\\":8},{\\\"currency\\\":\\\"SYS\\\",\\\"rate\\\":1.0,\\\"precision\\\":2}]}";
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate SYS_RATES " + data));
	GenerateBlocks(100);
	GenerateBlocks(100, "node2");
	GenerateBlocks(100, "node3");
	EscrowClaimReleaseLink("node2", guid, "node3");
	// restore EUR peg
	data = "{\\\"rates\\\":[{\\\"currency\\\":\\\"USD\\\",\\\"rate\\\":2690.1,\\\"precision\\\":2},{\\\"currency\\\":\\\"EUR\\\",\\\"rate\\\":2695.2,\\\"precision\\\":2},{\\\"currency\\\":\\\"GBP\\\",\\\"rate\\\":2697.3,\\\"precision\\\":2},{\\\"currency\\\":\\\"CAD\\\",\\\"rate\\\":2698.0,\\\"precision\\\":2},{\\\"currency\\\":\\\"BTC\\\",\\\"rate\\\":100000.0,\\\"precision\\\":8},{\\\"currency\\\":\\\"SYS\\\",\\\"rate\\\":1.0,\\\"precision\\\":2}]}";
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate SYS_RATES " + data));
	GenerateBlocks(10);
	GenerateBlocks(10, "node2");
	GenerateBlocks(10, "node3");
}
BOOST_AUTO_TEST_SUITE_END ()