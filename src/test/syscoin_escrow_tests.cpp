#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "rpc/server.h"
#include <boost/test/unit_test.hpp>
#include "feedback.h"
BOOST_FIXTURE_TEST_SUITE (syscoin_escrow_tests, BasicSyscoinTestingSetup)

BOOST_AUTO_TEST_CASE (generate_escrow_release)
{
	printf("Running generate_escrow_release...\n");
	UniValue r;
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	AliasNew("node1", "buyeralias", "password", "changeddata1");
	AliasNew("node2", "selleralias", "password", "changeddata2");
	AliasNew("node3", "arbiteralias", "password", "changeddata3");
	string qty = "3";
	string message = "paymentmessage";
	string offerguid = OfferNew("node2", "selleralias", "category", "title", "100", "0.05", "description", "USD");
	string guid = EscrowNew("node1", "node2", "buyeralias", offerguid, qty, message, "arbiteralias", "selleralias");
	EscrowRelease("node1", "buyer", guid);	
	EscrowClaimRelease("node2", guid);
}
BOOST_AUTO_TEST_CASE (generate_escrow_big)
{
	printf("Running generate_escrow_big...\n");
	UniValue r;
	// 64 bytes long
	string goodname1 = "sfsdfddfsdsfsfsdfdfsdsfdsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdsfsdfd";
	string goodname2 = "dfsddfdfsdsfsfsdfdfsdsfdsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdsfsdfd";
	string goodname3 = "ffsdfddfsdsfsfsdfdfsdsfdsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdsfsdfd";
	// 256 bytes long
	string gooddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";	
	// 257 bytes long
	string baddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddz";	
				
	AliasNew("node1", goodname1, "password1", "changeddata1");
	AliasNew("node2", goodname2, "password2", "changeddata2");
	AliasNew("node3", goodname3, "password3", "changeddata3");
	string qty = "3";

	string offerguid = OfferNew("node2", goodname2, "category", "title", "100", "0.05", "description", "USD");
	// payment message too long
	BOOST_CHECK_THROW(r = CallRPC("node1", "escrownew " + goodname1 + " " + offerguid + " " + qty + " " + baddata + " " + goodname3), runtime_error);
	string guid = EscrowNew("node1", "node2", goodname1, offerguid, qty, gooddata, goodname3, goodname2);
	EscrowRelease("node1", "buyer", guid);	
	EscrowClaimRelease("node2", guid);
}
BOOST_AUTO_TEST_CASE (generate_escrowrefund_seller)
{
	AliasNew("node1", "buyeraliasrefund", "password", "changeddata1");
	AliasNew("node2", "selleraliasrefund", "password", "changeddata2");
	AliasNew("node3", "arbiteraliasrefund", "password", "changeddata3");
	printf("Running generate_escrowrefund_seller...\n");
	string qty = "4";
	string message = "paymentmessage";
	string offerguid = OfferNew("node2", "selleraliasrefund", "category", "title", "100", "1.22", "description", "CAD");
	string guid = EscrowNew("node1", "node2", "buyeraliasrefund", offerguid, qty, message, "arbiteraliasrefund", "selleraliasrefund");
	EscrowRefund("node2", "seller", guid);
	EscrowClaimRefund("node1", guid);
}
BOOST_AUTO_TEST_CASE (generate_escrowrefund_arbiter)
{
	printf("Running generate_escrowrefund_arbiter...\n");
	string qty = "5";
	string offerguid = OfferNew("node2", "selleraliasrefund", "category", "title", "100", "0.25", "description", "EUR");
	string message = "paymentmessage";
	string guid = EscrowNew("node1", "node2", "buyeraliasrefund", offerguid, qty, message, "arbiteraliasrefund", "selleraliasrefund");
	EscrowRefund("node3", "arbiter", guid);
	EscrowClaimRefund("node1", guid);
}
BOOST_AUTO_TEST_CASE (generate_escrowrefund_invalid)
{
	UniValue r;
	printf("Running generate_escrowrefund_invalid...\n");
	AliasNew("node1", "buyeraliasrefund2", "password", "changeddata1");
	AliasNew("node2", "selleraliasrefund2", "password", "changeddata2");
	AliasNew("node3", "arbiteraliasrefund2", "password", "changeddata3");
	string qty = "2";
	string offerguid = OfferNew("node2", "selleraliasrefund2", "category", "title", "100", "1.45", "description", "EUR");
	string guid = EscrowNew("node1", "node2", "buyeraliasrefund2", offerguid, qty, "message", "arbiteraliasrefund2", "selleraliasrefund2");
	// try to claim refund even if not refunded
	BOOST_CHECK_THROW(CallRPC("node2", "escrowclaimrefund " + guid), runtime_error);
	// buyer cant refund to himself
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrefund " + guid + " buyer"), runtime_error);
	EscrowRefund("node2", "seller", guid);
	// cant refund already refunded escrow
	BOOST_CHECK_THROW(CallRPC("node2", "escrowrefund " + guid + " seller"), runtime_error);
	// arbiter can also send claim refund to buyer
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "escrowclaimrefund " + guid));
	// seller cannot
	BOOST_CHECK_THROW(r = CallRPC("node2", "escrowclaimrefund " + guid), runtime_error);

	EscrowClaimRefund("node1", guid);
	// cant inititate another refund after claimed already
	BOOST_CHECK_THROW(CallRPC("node2", "escrowrefund " + guid + " seller"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrefund " + guid + " buyer"), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_escrowrelease_invalid)
{
	UniValue r;
	printf("Running generate_escrowrelease_invalid...\n");
	string qty = "4";
	AliasNew("node1", "buyeraliasrefund3", "password", "changeddata1");
	AliasNew("node2", "selleraliasrefund3", "password", "changeddata2");
	AliasNew("node3", "arbiteraliasrefund3", "password", "changeddata3");
	string offerguid = OfferNew("node2", "selleraliasrefund3", "category", "title", "100", "1.45", "description", "SYS");
	string guid = EscrowNew("node1", "node2", "buyeraliasrefund3", offerguid, qty, "message", "arbiteraliasrefund3", "selleraliasrefund3");
	// try to claim release even if not released
	BOOST_CHECK_THROW(CallRPC("node2", "escrowclaimrelease " + guid), runtime_error);
	// seller cant release buyers funds
	BOOST_CHECK_THROW(CallRPC("node2", "escrowrelease " + guid + " seller"), runtime_error);
	EscrowRelease("node1", "buyer", guid);
	// cant release already released escrow
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrelease " + guid + " buyer"), runtime_error);
	// arbiter can send release
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "escrowclaimrelease " + guid));
	// buyer cant
	BOOST_CHECK_THROW(r = CallRPC("node1", "escrowclaimrelease " + guid), runtime_error);

	EscrowClaimRelease("node2", guid);
	// cant inititate another release after claimed already
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrelease " + guid + " buyer"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrelease " + guid + " seller"), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_escrowrelease_arbiter)
{
	printf("Running generate_escrowrelease_arbiter...\n");
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	AliasNew("node1", "buyeralias1", "password", "changeddata1");
	AliasNew("node2", "selleralias111", "password", "changeddata2");
	AliasNew("node3", "arbiteralias1", "password", "changeddata3");
	UniValue r;
	string qty = "1";
	string offerguid = OfferNew("node2", "selleralias111", "category", "title", "100", "0.05", "description", "GBP");
	string guid = EscrowNew("node1", "node2", "buyeralias1", offerguid, qty, "message", "arbiteralias1", "selleralias111");
	EscrowRelease("node3", "arbiter", guid);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + guid));
	CAmount escrowfee = find_value(r.get_obj(), "sysfee").get_int64();
	// get arbiter balance (ensure he gets escrow fees, since he stepped in and released)
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasinfo arbiteralias1"));
	CAmount balanceBeforeArbiter = AmountFromValue(find_value(r.get_obj(), "balance"));
	EscrowClaimRelease("node2", guid);
	// get arbiter balance after release
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasinfo arbiteralias1"));
	balanceBeforeArbiter += escrowfee;
	CAmount balanceAfterArbiter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceBeforeArbiter, balanceAfterArbiter);
	


}
BOOST_AUTO_TEST_CASE (generate_escrowfeedback)
{
	printf("Running generate_escrowfeedback...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "sellerescrowfeedback", "password", "somedata");
	AliasNew("node2", "buyerescrowfeedback", "password", "somedata");
	AliasNew("node3", "arbiterescrowfeedback", "password", "somedata");

	string qty = "1";
	string offerguid = OfferNew("node1", "sellerescrowfeedback", "category", "title", "100", "0.05", "description", "GBP");
	string guid = EscrowNew("node2", "node1", "buyerescrowfeedback", offerguid, qty, "message", "arbiterescrowfeedback", "sellerescrowfeedback");
	EscrowRelease("node3", "arbiter", guid);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + guid));
	EscrowClaimRelease("node1", guid);
	GenerateBlocks(5);
	// seller leaves feedback first
	EscrowFeedback("node1", "seller", guid,"feedbackbuyer", "1", FEEDBACKBUYER, "feedbackarbiter", "2", FEEDBACKARBITER, true);
	// he can more if he wishes to
	EscrowFeedback("node1", "seller", guid, "feedbackbuyer", "1", FEEDBACKBUYER, "feedbackarbiter", "2", FEEDBACKARBITER, false);
	EscrowFeedback("node1", "seller", guid, "feedbackbuyer", "1", FEEDBACKBUYER, "feedbackarbiter", "2", FEEDBACKARBITER, false);


	// then buyer can leave feedback
	EscrowFeedback("node2", "buyer", guid, "feedbackseller", "1", FEEDBACKSELLER, "feedbackarbiter", "3", FEEDBACKARBITER, true);
	// he can more if he wishes to
	EscrowFeedback("node2", "buyer", guid,"feedbackseller", "1", FEEDBACKSELLER, "feedbackarbiter", "3", FEEDBACKARBITER, false);
	EscrowFeedback("node2", "buyer", guid, "feedbackseller", "1", FEEDBACKSELLER, "feedbackarbiter", "3", FEEDBACKARBITER, false);

	// and arbiter can also leave feedback
	EscrowFeedback("node3", "arbiter", guid, "feedbackbuyer", "4", FEEDBACKBUYER, "feedbackseller", "2", FEEDBACKSELLER, true);
	// he can more if he wishes to
	EscrowFeedback("node3", "arbiter", guid, "feedbackbuyer", "4", FEEDBACKBUYER, "feedbackseller", "2", FEEDBACKSELLER, false);
	EscrowFeedback("node3", "arbiter", guid, "feedbackbuyer", "4", FEEDBACKBUYER, "feedbackseller", "2", FEEDBACKSELLER, false);
}
BOOST_AUTO_TEST_CASE (generate_escrow_linked_release)
{
	UniValue r;
	printf("Running generate_escrow_linked_release...\n");
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	AliasNew("node1", "buyeralias2", "password", "changeddata1");
	AliasNew("node2", "selleralias22", "password", "changeddata2");
	AliasNew("node3", "arbiteralias2", "password", "changeddata3");
	string qty = "3";
	string message = "paymentmessage";
	string offerguid = OfferNew("node2", "selleralias22", "category", "title", "100", "0.04", "description", "EUR");
	string commission = "10";
	string description = "newdescription";
	// by default linking isn't allowed
	BOOST_CHECK_THROW(CallRPC("node3", "offerlink arbiteralias2 " + offerguid + " " + commission + " " + description), runtime_error);
	offerguid = OfferNew("node2", "selleralias22", "category", "title", "100", "0.04", "description", "EUR", "nocert");
	OfferAddWhitelist("node2", offerguid, "arbiteralias2", "5");
	string offerlinkguid = OfferLink("node3", "arbiteralias2", offerguid, commission, description);
	string guid = EscrowNew("node1", "node2", "buyeralias2", offerlinkguid, qty, message, "arbiteralias2", "selleralias22");
	EscrowRelease("node1", "buyer", guid);
	// arbiter can send release
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "escrowclaimrelease " + guid));

	AliasUpdate("node1", "buyeralias2", "changeddata1", "priv");
	AliasUpdate("node2", "selleralias22", "changeddata1", "priv");
	AliasUpdate("node3", "arbiteralias2", "changeddata1", "priv");
	OfferUpdate("node2", "selleralias22", offerguid, "category", "titlenew", "100", "0.04", "descriptionnew", "EUR", false, "nocert", "location");
	EscrowClaimRelease("node2", guid);
}
BOOST_AUTO_TEST_CASE (generate_escrow_linked_release_with_peg_update)
{
	printf("Running generate_escrow_linked_release_with_peg_update...\n");
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	AliasNew("node1", "buyeralias33", "password", "changeddata1");
	AliasNew("node2", "selleralias33", "password", "changeddata2");
	AliasNew("node3", "arbiteralias333", "password", "changeddata3");
	string qty = "3";
	string message = "paymentmessage";
	string offerguid = OfferNew("node2", "selleralias33", "category", "title", "100", "0.05", "description", "EUR", "nocert");
	OfferAddWhitelist("node2", offerguid, "arbiteralias333", "5");
	string commission = "3";
	string description = "newdescription";
	string offerlinkguid = OfferLink("node3", "arbiteralias333", offerguid, commission, description);
	string guid = EscrowNew("node1", "node2", "buyeralias33", offerlinkguid, qty, message, "arbiteralias333", "selleralias33");
	EscrowRelease("node1", "buyer", guid);
	// update the EUR peg twice before claiming escrow
	string data = "{\\\"rates\\\":[{\\\"currency\\\":\\\"USD\\\",\\\"rate\\\":2690.1,\\\"precision\\\":2},{\\\"currency\\\":\\\"EUR\\\",\\\"rate\\\":269.2,\\\"precision\\\":2},{\\\"currency\\\":\\\"GBP\\\",\\\"rate\\\":2697.3,\\\"precision\\\":2},{\\\"currency\\\":\\\"CAD\\\",\\\"rate\\\":2698.0,\\\"precision\\\":2},{\\\"currency\\\":\\\"BTC\\\",\\\"rate\\\":100000.0,\\\"precision\\\":8},{\\\"currency\\\":\\\"SYS\\\",\\\"rate\\\":1.0,\\\"precision\\\":2}]}";
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg sysrates.peg " + data));
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	data = "{\\\"rates\\\":[{\\\"currency\\\":\\\"USD\\\",\\\"rate\\\":2690.1,\\\"precision\\\":2},{\\\"currency\\\":\\\"EUR\\\",\\\"rate\\\":218.2,\\\"precision\\\":2},{\\\"currency\\\":\\\"GBP\\\",\\\"rate\\\":2697.3,\\\"precision\\\":2},{\\\"currency\\\":\\\"CAD\\\",\\\"rate\\\":2698.0,\\\"precision\\\":2},{\\\"currency\\\":\\\"BTC\\\",\\\"rate\\\":100000.0,\\\"precision\\\":8},{\\\"currency\\\":\\\"SYS\\\",\\\"rate\\\":1.0,\\\"precision\\\":2}]}";
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg sysrates.peg " + data));
	// ensure dependent services don't expire
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg buyeralias33 data"));
	BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasupdate sysrates.peg selleralias33 data"));
	BOOST_CHECK_NO_THROW(CallRPC("node3", "aliasupdate sysrates.peg arbiteralias333 data"));
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	OfferUpdate("node2", "selleralias33", offerguid, "category", "titlenew", "100", "0.05", "descriptionnew", "EUR", false, "nocert", "location");
	GenerateBlocks(5, "node2");
	EscrowClaimRelease("node2", guid);
	// restore EUR peg
	data = "{\\\"rates\\\":[{\\\"currency\\\":\\\"USD\\\",\\\"rate\\\":2690.1,\\\"precision\\\":2},{\\\"currency\\\":\\\"EUR\\\",\\\"rate\\\":2695.2,\\\"precision\\\":2},{\\\"currency\\\":\\\"GBP\\\",\\\"rate\\\":2697.3,\\\"precision\\\":2},{\\\"currency\\\":\\\"CAD\\\",\\\"rate\\\":2698.0,\\\"precision\\\":2},{\\\"currency\\\":\\\"BTC\\\",\\\"rate\\\":100000.0,\\\"precision\\\":8},{\\\"currency\\\":\\\"SYS\\\",\\\"rate\\\":1.0,\\\"precision\\\":2}]}";
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg sysrates.peg " + data));
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
}
BOOST_AUTO_TEST_CASE (generate_escrowpruning)
{
	UniValue r;
	// makes sure services expire in 100 blocks instead of 1 year of blocks for testing purposes
	printf("Running generate_escrowpruning...\n");
	AliasNew("node1", "selleraliasprune", "password", "changeddata2");
	AliasNew("node2", "buyeraliasprune", "password", "changeddata2");
	string offerguid = OfferNew("node1", "selleraliasprune", "category", "title", "100", "0.05", "description", "USD");
	// stop node3
	StopNode("node3");
	// create a new service
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "escrownew buyeraliasprune " + offerguid + " 1 message selleraliasprune"));
	const UniValue &arr1 = r.get_array();
	string guid1 = arr1[1].get_str();
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node1");

	BOOST_CHECK_NO_THROW(CallRPC("node1", "offerupdate selleraliasprune " + offerguid + " category title 100 0.05 description"));
	GenerateBlocks(5, "node1");
	// stop and start node1
	StopNode("node1");
	StartNode("node1");
	GenerateBlocks(5, "node1");
	// ensure you can still update because escrow hasn't been completed yet
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg selleraliasprune data"));
	BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasupdate sysrates.peg buyeraliasprune data"));
	GenerateBlocks(5, "node1");
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(CallRPC("node2", "escrowrelease " + guid1 + " buyer"));
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(CallRPC("node1", "offerupdate selleraliasprune " + offerguid + " category title 100 0.05 description"));
	GenerateBlocks(5, "node1");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowclaimrelease " + guid1));
	UniValue retArray = r.get_array();
	BOOST_CHECK_NO_THROW(CallRPC("node1", "escrowcompleterelease " + guid1 + " " + retArray[0].get_str()));
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(CallRPC("node1", "offerupdate selleraliasprune " + offerguid + " category title 100 0.05 description"));
	GenerateBlocks(5, "node1");
	
	// leave some feedback (escrow is complete but not expired yet)
	BOOST_CHECK_NO_THROW(CallRPC("node1",  "escrowfeedback " + guid1 + " seller 1 2 3 4"));
	GenerateBlocks(5, "node1");

	// try to leave feedback it should let you because aliases not expired
	BOOST_CHECK_NO_THROW(CallRPC("node2",  "escrowfeedback " + guid1 + " buyer 1 2 3 4"));
	GenerateBlocks(5, "node2");

	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "escrowinfo " + guid1));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_int(), 0);	
	ExpireAlias("buyeraliasprune");
	StartNode("node3");
	ExpireAlias("buyeraliasprune");
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service
	BOOST_CHECK_THROW(CallRPC("node3", "escrowinfo " + guid1), runtime_error);
	BOOST_CHECK_EQUAL(EscrowFilter("node3", guid1), false);
	// cannot leave feedback on expired escrow
	BOOST_CHECK_THROW(CallRPC("node2",  "escrowfeedback " + guid1 + " buyer 1 2 3 4"), runtime_error);
}

BOOST_AUTO_TEST_SUITE_END ()