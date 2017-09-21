#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "rpc/server.h"
#include <boost/test/unit_test.hpp>
#include "feedback.h"
#include "alias.h"
#include "cert.h"
#include "base58.h"
#include <boost/lexical_cast.hpp>
BOOST_FIXTURE_TEST_SUITE (syscoin_escrow_tests, BasicSyscoinTestingSetup)

BOOST_AUTO_TEST_CASE (generate_escrow_release)
{
	printf("Running generate_escrow_release...\n");
	UniValue r;
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	AliasNew("node1", "buyeralias", "changeddata1");
	AliasNew("node2", "selleralias", "changeddata2");
	AliasNew("node3", "arbiteralias", "changeddata3");
	string qty = "3";
	string offerguid = OfferNew("node2", "selleralias", "category", "title", "100", "0.05", "description", "USD");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeralias 500"), runtime_error);
	GenerateBlocks(10);
	string guid = EscrowNew("node1", "node2", "buyeralias", offerguid, qty, "arbiteralias");
	EscrowRelease("node1", "buyer", guid);	
	EscrowClaimRelease("node2", "seller", guid);
}
BOOST_AUTO_TEST_CASE (generate_escrowrefund_seller)
{
	AliasNew("node1", "buyeraliasrefund", "changeddata1");
	AliasNew("node2", "selleraliasrefund", "changeddata2");
	AliasNew("node3", "arbiteraliasrefund", "changeddata3");
	printf("Running generate_escrowrefund_seller...\n");
	string qty = "4";
	string offerguid = OfferNew("node2", "selleraliasrefund", "category", "title", "100", "1.22", "description", "CAD");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeraliasrefund 5000"), runtime_error);
	GenerateBlocks(10);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeraliasrefund 5000"), runtime_error);
	GenerateBlocks(10);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeraliasrefund 5000"), runtime_error);
	GenerateBlocks(10);
	string guid = EscrowNew("node1", "node2", "buyeraliasrefund", offerguid, qty, "arbiteraliasrefund");
	EscrowRefund("node2", "seller", guid);
	EscrowClaimRefund("node1", "buyer", guid);
}
BOOST_AUTO_TEST_CASE (generate_escrowrefund_arbiter)
{
	printf("Running generate_escrowrefund_arbiter...\n");
	string qty = "5";
	string offerguid = OfferNew("node2", "selleraliasrefund", "category", "title", "100", "0.25", "description", "EUR");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeraliasrefund 2000"), runtime_error);
	GenerateBlocks(10);
	string guid = EscrowNew("node1", "node2", "buyeraliasrefund", offerguid, qty, "arbiteraliasrefund");
	EscrowRefund("node3", "arbiter", guid);
	EscrowClaimRefund("node1", "buyer", guid);
}
BOOST_AUTO_TEST_CASE (generate_escrowrefund_invalid)
{
	UniValue r;
	printf("Running generate_escrowrefund_invalid...\n");
	AliasNew("node1", "buyeraliasrefund2", "changeddata1");
	AliasNew("node2", "selleraliasrefund2", "changeddata2");
	AliasNew("node3", "arbiteraliasrefund2", "changeddata3");
	string qty = "2";
	string offerguid = OfferNew("node2", "selleraliasrefund2", "category", "title", "100", "1.45", "description", "EUR");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeraliasrefund2 10000"), runtime_error);
	GenerateBlocks(10);
	string guid = EscrowNew("node1", "node2", "buyeraliasrefund2", offerguid, qty,"arbiteraliasrefund2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + guid));
	string escrowaddress = find_value(r.get_obj(), "escrowaddress").get_str();

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getaddressutxos \"{\\\"addresses\\\": [\\\"" + escrowaddress + "\\\"]}\""));
	UniValue addressUTXOsArray = r.get_array();
	string inputStr = "\"[";
	for (unsigned int i = 0; i < addressUTXOsArray.size(); i++)
	{
		const UniValue& utxoObj = addressUTXOsArray[i].get_obj();
		const string& txidStr = find_value(utxoObj.get_obj(), "txid").get_str();
		const int& nOut = find_value(utxoObj.get_obj(), "outputIndex").get_int();
		CAmount satoshis = find_value(utxoObj.get_obj(), "satoshis").get_int64();
		inputStr += "{\\\"txid\\\":\\\"" + txidStr + "\\\",\\\"vout\\\":" + boost::lexical_cast<string>(nOut) + ",\\\"satoshis\\\":" + boost::lexical_cast<string>(satoshis) + "}";
	}
	inputStr += "]\"";

	// try to claim refund even if not refunded
	BOOST_CHECK_THROW(CallRPC("node2", "escrowclaimrefund " + guid + " buyer " + inputStr), runtime_error);
	// buyer cant refund to himself
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrefund " + guid + " buyer"), runtime_error);
	EscrowRefund("node2", "seller", guid);
	// cant refund already refunded escrow
	BOOST_CHECK_THROW(CallRPC("node2", "escrowrefund " + guid + " seller"), runtime_error);
	// seller cannot
	BOOST_CHECK_THROW(r = CallRPC("node2", "escrowclaimrefund " + guid + " buyer " + inputStr), runtime_error);
	// arbiter can resend claim refund to buyer
	EscrowRefund("node3", "arbiter", guid);
	EscrowClaimRefund("node1", "buyer", guid);
	// cant inititate another refund after claimed already
	BOOST_CHECK_THROW(CallRPC("node2", "escrowrefund " + guid + " seller"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrefund " + guid + " buyer"), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_escrowrelease_invalid)
{
	UniValue r;
	printf("Running generate_escrowrelease_invalid...\n");
	string qty = "4";
	AliasNew("node1", "buyeraliasrefund3", "changeddata1");
	AliasNew("node2", "selleraliasrefund3", "changeddata2");
	AliasNew("node3", "arbiteraliasrefund3", "changeddata3");
	string offerguid = OfferNew("node2", "selleraliasrefund3", "category", "title", "100", "1.45", "description", "SYS");
	string guid = EscrowNew("node1", "node2", "buyeraliasrefund3", offerguid, qty, "arbiteraliasrefund3");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + guid));
	string escrowaddress = find_value(r.get_obj(), "escrowaddress").get_str();

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getaddressutxos \"{\\\"addresses\\\": [\\\"" + escrowaddress + "\\\"]}\""));
	UniValue addressUTXOsArray = r.get_array();
	string inputStr = "\"[";
	for (unsigned int i = 0; i < addressUTXOsArray.size(); i++)
	{
		const UniValue& utxoObj = addressUTXOsArray[i].get_obj();
		const string& txidStr = find_value(utxoObj.get_obj(), "txid").get_str();
		const int& nOut = find_value(utxoObj.get_obj(), "outputIndex").get_int();
		CAmount satoshis = find_value(utxoObj.get_obj(), "satoshis").get_int64();
		inputStr += "{\\\"txid\\\":\\\"" + txidStr + "\\\",\\\"vout\\\":" + boost::lexical_cast<string>(nOut) + ",\\\"satoshis\\\":" + boost::lexical_cast<string>(satoshis) + "}";
	}
	inputStr += "]\"";

	// try to claim release even if not released
	BOOST_CHECK_THROW(CallRPC("node2", "escrowclaimrelease " + guid + " seller " + inputStr), runtime_error);
	// seller cant release buyers funds
	BOOST_CHECK_THROW(CallRPC("node2", "escrowrelease " + guid + " seller"), runtime_error);
	EscrowRelease("node1", "buyer", guid);
	// cant release already released escrow
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrelease " + guid + " buyer"), runtime_error);
	// buyer cant
	BOOST_CHECK_THROW(r = CallRPC("node1", "escrowclaimrelease " + guid + " seller " + inputStr), runtime_error);

	EscrowRelease("node3", "arbiter", guid);

	EscrowClaimRelease("node2", "seller", guid);
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
	AliasNew("node1", "buyeralias1", "changeddata1");
	AliasNew("node2", "selleralias111", "changeddata2");
	AliasNew("node3", "arbiteralias1", "changeddata3");
	UniValue r;
	string qty = "1";
	string offerguid = OfferNew("node2", "selleralias111", "category", "title", "100", "0.05", "description", "GBP");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeralias1 1000"), runtime_error);
	GenerateBlocks(10);
	string guid = EscrowNew("node1", "node2", "buyeralias1", offerguid, qty, "arbiteralias1");
	EscrowRelease("node3", "arbiter", guid);
	EscrowClaimRelease("node2", "seller", guid);
}
BOOST_AUTO_TEST_CASE (generate_escrowfeedback)
{
	printf("Running generate_escrowfeedback...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "sellerescrowfeedback", "somedata");
	AliasNew("node2", "buyerescrowfeedback", "somedata");
	AliasNew("node3", "arbiterescrowfeedback", "somedata");

	string qty = "1";
	string offerguid = OfferNew("node1", "sellerescrowfeedback", "category", "title", "100", "0.05", "description", "GBP");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyerescrowfeedback 1000"), runtime_error);
	GenerateBlocks(10);
	string guid = EscrowNew("node2", "node1", "buyerescrowfeedback", offerguid, qty, "arbiterescrowfeedback");
	EscrowRelease("node3", "arbiter", guid);
	EscrowClaimRelease("node1", "seller", guid);
	// seller leaves feedback first to the other parties
	EscrowFeedback("node1", "seller", guid,"feedbackbuyer", "1", FEEDBACKBUYER);
	EscrowFeedback("node1", "seller", guid, "feedbackarbiter", "1", FEEDBACKARBITER);

	// leave another feedback and notice that you can't find it in the indexer (first feedback will be indexed only per user per guid+touser combination)
	string escrowfeedbackstr = "escrowfeedback " + guid + " seller feedback 1";
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", escrowfeedbackstr));
	UniValue arr = r.get_array();
	string feedbackTxid = arr[0].get_str();
	GenerateBlocks(10, "node1");
	r = FindFeedback("node1", feedbackTxid);
	BOOST_CHECK(r.isNull());

	// buyer can leave feedback
	EscrowFeedback("node2", "buyer", guid, "feedbackseller", "1", FEEDBACKSELLER);
	EscrowFeedback("node2", "buyer", guid, "feedbackarbiter", "1", FEEDBACKARBITER);

	// leave another feedback and notice that you can't find it in the indexer (first feedback will be indexed only per user per guid+touser combination)
	escrowfeedbackstr = "escrowfeedback " + guid + " buyer feedback 1";
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", escrowfeedbackstr));
	arr = r.get_array();
	feedbackTxid = arr[0].get_str();
	GenerateBlocks(10, "node2");
	r = FindFeedback("node2", feedbackTxid);
	BOOST_CHECK(r.isNull());

	// arbiter leaves feedback
	EscrowFeedback("node3", "arbiter", guid, "feedbackbuyer", "4", FEEDBACKBUYER);
	EscrowFeedback("node3", "arbiter", guid, "feedbackseller", "4", FEEDBACKSELLER);

	// leave another feedback and notice that you can't find it in the indexer (first feedback will be indexed only per user per guid+touser combination)
	escrowfeedbackstr = "escrowfeedback " + guid + " arbiter feedback 1";
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", escrowfeedbackstr));
	arr = r.get_array();
	feedbackTxid = arr[0].get_str();
	GenerateBlocks(10, "node3");
	r = FindFeedback("node3", feedbackTxid);
	BOOST_CHECK(r.isNull());
}
BOOST_AUTO_TEST_CASE (generate_escrow_linked_release)
{
	UniValue r;
	printf("Running generate_escrow_linked_release...\n");
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	AliasNew("node1", "buyeralias2", "changeddata1");
	AliasNew("node2", "selleralias22", "changeddata2");
	AliasNew("node3", "arbiteralias2", "changeddata3");
	string qty = "3";
	string offerguid = OfferNew("node2", "selleralias22", "category", "title", "100", "0.04", "description", "EUR");
	string commission = "10";
	string description = "newdescription";
	// by default linking isn't allowed
	BOOST_CHECK_THROW(CallRPC("node3", "offerlink arbiteralias2 " + offerguid + " " + commission + " " + description), runtime_error);
	offerguid = OfferNew("node2", "selleralias22", "category", "title", "100", "0.04", "description", "EUR");
	OfferAddWhitelist("node2", offerguid, "arbiteralias2", "5");
	string offerlinkguid = OfferLink("node3", "arbiteralias2", offerguid, commission, description);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeralias2 1000"), runtime_error);
	GenerateBlocks(10);
	string guid = EscrowNew("node1", "node2", "buyeralias2", offerlinkguid, qty, "arbiteralias2");
	EscrowRelease("node1", "buyer", guid);

	string hex_str = AliasUpdate("node1", "buyeralias2", "changeddata1", "priv");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node2", "selleralias22", "changeddata1", "priv");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node3", "arbiteralias2", "changeddata1", "priv");
	BOOST_CHECK(hex_str.empty());
	OfferUpdate("node2", "selleralias22", offerguid, "category", "titlenew", "100", "0.04", "descriptionnew", "EUR");
	EscrowClaimRelease("node2", "seller", guid);
}
BOOST_AUTO_TEST_CASE (generate_escrow_linked_release_with_peg_update)
{
	printf("Running generate_escrow_linked_release_with_peg_update...\n");
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	AliasNew("node1", "buyeralias33", "changeddata1");
	AliasNew("node2", "selleralias33", "changeddata2");
	AliasNew("node3", "arbiteralias333", "changeddata3");
	string qty = "3";
	string offerguid = OfferNew("node2", "selleralias33", "category", "title", "100", "0.05", "description", "EUR");
	OfferAddWhitelist("node2", offerguid, "arbiteralias333", "5");
	string commission = "3";
	string description = "newdescription";
	string offerlinkguid = OfferLink("node3", "arbiteralias333", offerguid, commission, description);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeralias33 40000"), runtime_error);
	GenerateBlocks(10);
	string guid = EscrowNew("node1", "node2", "buyeralias33", offerlinkguid, qty, "arbiteralias333");
	EscrowRelease("node1", "buyer", guid);
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + guid));
	CAmount nTotal = AmountFromValue(find_value(r.get_obj(), "total"));
	// 2695.2 SYS/EUR
	BOOST_CHECK_EQUAL(nTotal, AmountFromValue(3 * 0.05*1.03*2695.2));
	// update the EUR peg twice before claiming escrow
	string data = "{\\\"rates\\\":[{\\\"currency\\\":\\\"USD\\\",\\\"rate\\\":2690.1,\\\"precision\\\":2},{\\\"currency\\\":\\\"EUR\\\",\\\"rate\\\":269.2,\\\"precision\\\":2},{\\\"currency\\\":\\\"GBP\\\",\\\"rate\\\":2697.3,\\\"precision\\\":2},{\\\"currency\\\":\\\"CAD\\\",\\\"rate\\\":2698.0,\\\"precision\\\":2},{\\\"currency\\\":\\\"BTC\\\",\\\"rate\\\":100000.0,\\\"precision\\\":8},{\\\"currency\\\":\\\"SYS\\\",\\\"rate\\\":1.0,\\\"precision\\\":2}]}";
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg " + data));
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	data = "{\\\"rates\\\":[{\\\"currency\\\":\\\"USD\\\",\\\"rate\\\":2690.1,\\\"precision\\\":2},{\\\"currency\\\":\\\"EUR\\\",\\\"rate\\\":218.2,\\\"precision\\\":2},{\\\"currency\\\":\\\"GBP\\\",\\\"rate\\\":2697.3,\\\"precision\\\":2},{\\\"currency\\\":\\\"CAD\\\",\\\"rate\\\":2698.0,\\\"precision\\\":2},{\\\"currency\\\":\\\"BTC\\\",\\\"rate\\\":100000.0,\\\"precision\\\":8},{\\\"currency\\\":\\\"SYS\\\",\\\"rate\\\":1.0,\\\"precision\\\":2}]}";
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg " + data));
	// ensure dependent services don't expire
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate buyeralias33 data"));
	BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasupdate selleralias33 data"));
	BOOST_CHECK_NO_THROW(CallRPC("node3", "aliasupdate arbiteralias333 data"));
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	guid = EscrowNew("node1", "node2", "buyeralias33", offerlinkguid, "2", "arbiteralias333");
	EscrowRelease("node1", "buyer", guid);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + guid));
	nTotal = AmountFromValue(find_value(r.get_obj(), "total"));
	// 218.2 SYS/EUR
	BOOST_CHECK_EQUAL(nTotal, AmountFromValue(2 * 0.05*1.03*218.2));


	OfferUpdate("node2", "selleralias33", offerguid, "category", "titlenew", "100", "0.07", "descriptionnew", "EUR", "\"\"", "\"\"", "\"\"", "6");

	guid = EscrowNew("node1", "node2", "buyeralias33", offerlinkguid, "4", "arbiteralias333");
	EscrowRelease("node1", "buyer", guid);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + guid));
	nTotal = AmountFromValue(find_value(r.get_obj(), "total"));
	// 218.2SYS/EUR
	BOOST_CHECK_EQUAL(nTotal, AmountFromValue(4 * 0.07*1.06*218.2));

	GenerateBlocks(5, "node2");
	EscrowClaimRelease("node2", "seller", guid);
	// restore EUR peg
	data = "{\\\"rates\\\":[{\\\"currency\\\":\\\"USD\\\",\\\"rate\\\":2690.1,\\\"precision\\\":2},{\\\"currency\\\":\\\"EUR\\\",\\\"rate\\\":2695.2,\\\"precision\\\":2},{\\\"currency\\\":\\\"GBP\\\",\\\"rate\\\":2697.3,\\\"precision\\\":2},{\\\"currency\\\":\\\"CAD\\\",\\\"rate\\\":2698.0,\\\"precision\\\":2},{\\\"currency\\\":\\\"BTC\\\",\\\"rate\\\":100000.0,\\\"precision\\\":8},{\\\"currency\\\":\\\"SYS\\\",\\\"rate\\\":1.0,\\\"precision\\\":2}]}";
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg " + data));
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	guid = EscrowNew("node1", "node2", "buyeralias33", offerlinkguid, "3", "arbiteralias333");
	EscrowRelease("node1", "buyer", guid);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + guid));
	nTotal = AmountFromValue(find_value(r.get_obj(), "total"));
	// 2695.2SYS/EUR
	BOOST_CHECK_EQUAL(nTotal, AmountFromValue(3 * 0.07*1.06*2695.2));

}
BOOST_AUTO_TEST_CASE (generate_escrowpruning)
{
	UniValue r;
	// makes sure services expire in 100 blocks instead of 1 year of blocks for testing purposes
	printf("Running generate_escrowpruning...\n");
	AliasNew("node1", "selleraliasprune", "changeddata2");
	AliasNew("node2", "buyeraliasprune", "changeddata2");
	string offerguid = OfferNew("node1", "selleraliasprune", "category", "title", "100", "0.05", "description", "USD");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeraliasprune 200"), runtime_error);
	GenerateBlocks(10);
	// stop node3
	StopNode("node3");
	// create a new service
	string guid1 = EscrowNew("node2", "node1", "buyeraliasprune", offerguid, "1", "selleraliasprune");
	OfferUpdate("node1", "selleraliasprune", offerguid, "category", "titlenew", "100", "0.05", "descriptionnew");
	// stop and start node1
	StopNode("node1");
	StartNode("node1");
	GenerateBlocks(5, "node1");
	// ensure you can still update because escrow hasn't been completed yet
	string hex_str = AliasUpdate("node1", "selleraliasprune");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node2", "buyeraliasprune");
	BOOST_CHECK(hex_str.empty());

	EscrowRelease("node2", "buyer", guid1);
	OfferUpdate("node1", "selleraliasprune", offerguid);
	EscrowClaimRelease("node1", "seller", guid1);
	OfferUpdate("node1", "selleraliasprune", offerguid);
	GenerateBlocks(5, "node1");
	
	// leave some feedback (escrow is complete but not expired yet)
	EscrowFeedback("node1", "seller", guid1,"feedbackbuyer", "1", FEEDBACKBUYER);

	// try to leave feedback it should let you because aliases not expired
	EscrowFeedback("node2", "buyer", guid1,"feedbackseller", "1", FEEDBACKSELLER);

	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "escrowinfo " + guid1));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), false);	
	ExpireAlias("buyeraliasprune");
	StartNode("node3");
	ExpireAlias("buyeraliasprune");
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service
	BOOST_CHECK_THROW(CallRPC("node3", "escrowinfo " + guid1), runtime_error);
	BOOST_CHECK_EQUAL(EscrowFilter("node3", guid1), false);
	// cannot leave feedback on expired escrow
	BOOST_CHECK_THROW(CallRPC("node2",  "escrowfeedback " + guid1 + " buyer 1 2 3 4"), runtime_error);
	ECC_Stop();
}

BOOST_AUTO_TEST_SUITE_END ()