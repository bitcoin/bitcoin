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
BOOST_AUTO_TEST_CASE(generate_auction_regular)
{
	// create regular auction, bid and do buynow 
	printf("Running generate_auction_regular...\n");
	UniValue r;
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	AliasNew("node1", "buyerauction", "changeddata1");
	AliasNew("node1", "buyerauction1a", "changeddata1");
	AliasNew("node2", "sellerauction", "changeddata2");
	AliasNew("node3", "arbiterauction", "changeddata3");
	AliasNew("node3", "arbiterauctiona", "changeddata3");
	string qty = "3";
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getblockchaininfo"));
	uint64_t mediantime = find_value(r.get_obj(), "mediantime").get_int64() + 3600;
	string expiry = boost::lexical_cast<string>(mediantime);
	string offerguid = OfferNew("node2", "sellerauction", "category", "title", "100", "0.05", "description", "USD", "\"\"" /*certguid*/, "\"\"" /*paymentoptions*/, "BUYNOW+AUCTION", expiry);
	// can't update offer auction settings until auction expires
	//						"offerupdate <alias> <guid> [category] [title] [quantity] [price] [description] [currency] [private=false] [cert. guid] [commission] [paymentOptions] [offerType=BUYNOW] [auction_expires] [auction_reserve] [auction_require_witness] [auction_deposit] [witness]\n"
	BOOST_CHECK_THROW(r = CallRPC("node2", "offerupdate sellerauction " + offerguid + " category title 90 0.15 description USD \"\" \"\" \"\" \"\" \"\" \"\" \"\" true"), runtime_error);

	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyerauction 500"), runtime_error);
	GenerateBlocks(10);
	string guid = EscrowNewAuction("node1", "node2", "buyerauction", offerguid, qty, "0.005", "arbiterauction");
	EscrowBid("node1", "buyerauction", guid, "0.01");
	EscrowBid("node1", "buyerauction", guid, "0.02");
	// must bid higher
	BOOST_CHECK_THROW(CallRPC("node1", "escrowbid buyerauction " + guid + " 0 0.03"), runtime_error);
	// must bid higher
	BOOST_CHECK_THROW(CallRPC("node1", "escrowbid buyerauction " + guid + " 9 0.02"), runtime_error);
	// must bid higher
	BOOST_CHECK_THROW(CallRPC("node1", "escrowbid buyerauction " + guid + " 9 0.01"), runtime_error);

	EscrowBid("node1", "buyerauction", guid, "0.03");

	EscrowNewBuyItNow("node1", "node2", "buyerauction", offerguid, qty, "arbiterauctiona");
	EscrowRelease("node1", "buyer", guid);
	EscrowClaimRelease("node2", guid);
	// after expiry can update
	SetSysMocktime(mediantime + 1);
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "offerupdate sellerauction " + offerguid + " category title 90 0.15 description USD \"\" \"\" \"\" \"\" \"\" \"\" \"\" true"));
}
BOOST_AUTO_TEST_CASE(generate_auction_reserve)
{
	// create regular auctio with reserve and try to underbid
	printf("Running generate_auction_reserve...\n");
	UniValue r;
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	AliasNew("node1", "buyerauction1", "changeddata1");
	AliasNew("node1", "buyerauction1b", "changeddata1");
	AliasNew("node2", "sellerauction1", "changeddata2");
	AliasNew("node3", "arbiterauction1", "changeddata3");
	AliasNew("node3", "arbiterauction1a", "changeddata3");
	string qty = "3";
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getblockchaininfo"));
	int64_t mediantime = find_value(r.get_obj(), "mediantime").get_int64() + 3600;
	string expiry = boost::lexical_cast<string>(mediantime);
	string offerguid = OfferNew("node2", "sellerauction1", "category", "title", "100", "0.05", "description", "USD", "\"\"" /*certguid*/, "\"\"" /*paymentoptions*/, "BUYNOW+AUCTION", expiry, "0.01");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyerauction1 500"), runtime_error);
	GenerateBlocks(10);
	string exttxid = "\"\"";
	string paymentoptions = "\"\"";
	string redeemscript = "\"\"";
	string buyNowStr = "false";
	string networkFee = "\"\"";
	string arbiterFee = "\"\"";
	string witness = "\"\"";
	string witnessFee = "\"\"";
	string shippingFee = "\"\"";
	string bid_in_payment_option = "0.11";
	string bid_in_offer_currency = "0.009";
	string total_in_payment_option = strprintf("%.*f", 2, pegRates["USD"] * 0.01);
	// try to underbid in offer currency
	string query = "escrownew false buyerauction1 arbiterauction1 " + offerguid + " " + qty + " " + buyNowStr + " " + total_in_payment_option + " " +  shippingFee + " " + networkFee + " " + arbiterFee + " " + witnessFee + " " + exttxid + " " + paymentoptions + " " + bid_in_payment_option + " " + bid_in_offer_currency + " " + witness;
	BOOST_CHECK_THROW(r = CallRPC("node1", query), runtime_error);

	string guid = EscrowNewAuction("node1", "node2", "buyerauction1", offerguid, qty, "0.01", "arbiterauction1");
	EscrowBid("node1", "buyerauction1", guid, "0.02");
	EscrowBid("node1", "buyerauction1", guid, "0.03");
	// must bid higher
	BOOST_CHECK_THROW(CallRPC("node1", "escrowbid buyerauction1 " + guid + " 9.002 0.02"), runtime_error);
	// must bid higher
	BOOST_CHECK_THROW(CallRPC("node1", "escrowbid buyerauction1 " + guid + " 9.001 0.01"), runtime_error);
	
	EscrowBid("node1", "buyerauction1", guid, "0.04");

	EscrowNewBuyItNow("node1", "node2", "buyerauction1", offerguid, qty, "arbiterauction1a");
	EscrowRelease("node1", "buyer", guid);
	EscrowClaimRelease("node2", guid);
}
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
	string guid = EscrowNewBuyItNow("node1", "node2", "buyeralias", offerguid, qty, "arbiteralias");
	EscrowRelease("node1", "buyer", guid);	
	EscrowClaimRelease("node2", guid);
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
	string guid = EscrowNewBuyItNow("node1", "node2", "buyeraliasrefund", offerguid, qty, "arbiteraliasrefund");
	EscrowRefund("node2", "seller", guid);
	EscrowClaimRefund("node1", guid);
}
BOOST_AUTO_TEST_CASE (generate_escrowrefund_arbiter)
{
	printf("Running generate_escrowrefund_arbiter...\n");
	string qty = "5";
	string offerguid = OfferNew("node2", "selleraliasrefund", "category", "title", "100", "0.25", "description", "EUR");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeraliasrefund 2000"), runtime_error);
	GenerateBlocks(10);
	string guid = EscrowNewBuyItNow("node1", "node2", "buyeraliasrefund", offerguid, qty, "arbiteraliasrefund");
	EscrowRefund("node3", "arbiter", guid);
	EscrowClaimRefund("node1", guid);
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
	string guid = EscrowNewBuyItNow("node1", "node2", "buyeraliasrefund2", offerguid, qty,"arbiteraliasrefund2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + guid));
	string escrowaddress = find_value(r.get_obj(), "escrowaddress").get_str();

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getaddressutxos \"{\\\"addresses\\\": [\\\"" + escrowaddress + "\\\"]}\""));
	UniValue addressUTXOsArray = r.get_array();
	string inputStr = "\"[";
	for (unsigned int i = 0; i < addressUTXOsArray.size(); i++)
	{
		if (i > 0)
			inputStr += ",";
		const UniValue& utxoObj = addressUTXOsArray[i].get_obj();
		const string& txidStr = find_value(utxoObj.get_obj(), "txid").get_str();
		const int& nOut = find_value(utxoObj.get_obj(), "outputIndex").get_int();
		CAmount satoshis = find_value(utxoObj.get_obj(), "satoshis").get_int64();
		inputStr += "{\\\"txid\\\":\\\"" + txidStr + "\\\",\\\"vout\\\":" + boost::lexical_cast<string>(nOut) + ",\\\"satoshis\\\":" + boost::lexical_cast<string>(satoshis) + "}";
	}
	inputStr += "]\"";

	// buyer cant refund to himself
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowcreaterawtransaction refund " + guid + " " + inputStr + " buyer"));
	const UniValue &arr = r.get_array();
	string rawtx = arr[0].get_str();
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrefund " + guid + " buyer " + rawtx), runtime_error);
	EscrowRefund("node2", "seller", guid);
	// cant refund already refunded escrow
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "escrowcreaterawtransaction refund " + guid + " " + inputStr + "  seller"));
	const UniValue &arr1 = r.get_array();
	rawtx = arr1[0].get_str();
	BOOST_CHECK_THROW(CallRPC("node2", "escrowrefund " + guid + " seller " + rawtx), runtime_error);

	// arbiter can resend claim refund to buyer
	EscrowRefund("node3", "arbiter", guid);
	EscrowClaimRefund("node1", guid);
	// cant inititate another refund after claimed already
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "escrowcreaterawtransaction refund " + guid + " " + inputStr + "  seller"));
	const UniValue &arr2 = r.get_array();
	rawtx = arr2[0].get_str();
	BOOST_CHECK_THROW(CallRPC("node2", "escrowrefund " + guid + " seller " + rawtx), runtime_error);
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
	string guid = EscrowNewBuyItNow("node1", "node2", "buyeraliasrefund3", offerguid, qty, "arbiteraliasrefund3");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + guid));
	string escrowaddress = find_value(r.get_obj(), "escrowaddress").get_str();

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getaddressutxos \"{\\\"addresses\\\": [\\\"" + escrowaddress + "\\\"]}\""));
	UniValue addressUTXOsArray = r.get_array();
	string inputStr = "\"[";
	for (unsigned int i = 0; i < addressUTXOsArray.size(); i++)
	{
		if (i > 0)
			inputStr += ",";
		const UniValue& utxoObj = addressUTXOsArray[i].get_obj();
		const string& txidStr = find_value(utxoObj.get_obj(), "txid").get_str();
		const int& nOut = find_value(utxoObj.get_obj(), "outputIndex").get_int();
		CAmount satoshis = find_value(utxoObj.get_obj(), "satoshis").get_int64();
		inputStr += "{\\\"txid\\\":\\\"" + txidStr + "\\\",\\\"vout\\\":" + boost::lexical_cast<string>(nOut) + ",\\\"satoshis\\\":" + boost::lexical_cast<string>(satoshis) + "}";
	}
	inputStr += "]\"";

	// seller cant release buyers funds
	BOOST_CHECK_NO_THROW(CallRPC("node2", "escrowcreaterawtransaction release " + guid + " " + inputStr + " seller"));
	const UniValue &arr = r.get_array();
	string rawtx = arr[0].get_str();
	BOOST_CHECK_THROW(CallRPC("node2", "escrowrelease " + guid + " seller " + rawtx), runtime_error);
	EscrowRelease("node1", "buyer", guid);
	// cant release already released escrow
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowcreaterawtransaction release " + guid + " " + inputStr + " buyer"));
	const UniValue &arr1 = r.get_array();
	rawtx = arr1[0].get_str();
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrelease " + guid + " buyer " + rawtx), runtime_error);

	EscrowRelease("node3", "arbiter", guid);

	EscrowClaimRelease("node2", guid);
	// cant inititate another release after claimed already
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowcreaterawtransaction release " + guid + " " + inputStr + " buyer"));
	const UniValue &arr2 = r.get_array();
	rawtx = arr2[0].get_str();
	BOOST_CHECK_THROW(CallRPC("node1", "escrowrelease " + guid + " buyer " + rawtx), runtime_error);
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
	string guid = EscrowNewBuyItNow("node1", "node2", "buyeralias1", offerguid, qty, "arbiteralias1");
	EscrowRelease("node3", "arbiter", guid);
	EscrowClaimRelease("node2", guid);
}
BOOST_AUTO_TEST_CASE(generate_escrow_linked_release_with_peg_update)
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
	AliasAddWhitelist("node2", offerguid, "arbiteralias333", "5");
	string commission = "3";
	string description = "newdescription";
	string offerlinkguid = OfferLink("node3", "arbiteralias333", offerguid, commission, description);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeralias33 40000"), runtime_error);
	GenerateBlocks(10);
	string guid = EscrowNewBuyItNow("node1", "node2", "buyeralias33", offerlinkguid, qty, "arbiteralias333");
	EscrowRelease("node1", "buyer", guid);
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + guid));
	CAmount nTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	// 2695.2 SYS/EUR
	BOOST_CHECK_EQUAL(nTotal, AmountFromValue(3 * 0.05*1.03*2695.2));
	pegRates["EUR"] = 218;
	guid = EscrowNewBuyItNow("node1", "node2", "buyeralias33", offerlinkguid, "2", "arbiteralias333");
	EscrowRelease("node1", "buyer", guid);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + guid));
	nTotal = AmountFromValue(find_value(r.get_obj(), "total"));
	// 218.2 SYS/EUR
	BOOST_CHECK_EQUAL(nTotal, AmountFromValue(2 * 0.05*1.03*218.2));

	OfferUpdate("node2", "selleralias33", offerguid, "category", "titlenew", "100", "0.07", "descriptionnew", "EUR", "\"\"", "\"\"", "6");

	guid = EscrowNewBuyItNow("node1", "node2", "buyeralias33", offerlinkguid, "4", "arbiteralias333");
	EscrowRelease("node1", "buyer", guid);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + guid));
	nTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	// 218.2SYS/EUR
	BOOST_CHECK_EQUAL(nTotal, AmountFromValue(4 * 0.07*1.06*218.2));

	GenerateBlocks(5, "node2");
	EscrowClaimRelease("node2", guid);
	// restore EUR peg
	pegRates["EUR"] = 2695.2;

	guid = EscrowNewBuyItNow("node1", "node2", "buyeralias33", offerlinkguid, "3", "arbiteralias333");
	EscrowRelease("node1", "buyer", guid);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowinfo " + guid));
	nTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	// 2695.2SYS/EUR
	BOOST_CHECK_EQUAL(nTotal, AmountFromValue(3 * 0.07*1.06*2695.2));

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
	string guid = EscrowNewBuyItNow("node2", "node1", "buyerescrowfeedback", offerguid, qty, "arbiterescrowfeedback");
	EscrowRelease("node3", "arbiter", guid);
	EscrowClaimRelease("node1", guid);
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
	AliasAddWhitelist("node2", "selleralias22", "arbiteralias2", "5");
	string offerlinkguid = OfferLink("node3", "arbiteralias2", offerguid, commission, description);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress buyeralias2 1000"), runtime_error);
	GenerateBlocks(10);
	string guid = EscrowNewBuyItNow("node1", "node2", "buyeralias2", offerlinkguid, qty, "arbiteralias2");
	EscrowRelease("node1", "buyer", guid);

	string hex_str = AliasUpdate("node1", "buyeralias2", "changeddata1");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node2", "selleralias22", "changeddata1");
	BOOST_CHECK(hex_str.empty());
	hex_str = AliasUpdate("node3", "arbiteralias2", "changeddata1");
	BOOST_CHECK(hex_str.empty());
	OfferUpdate("node2", "selleralias22", offerguid, "category", "titlenew", "100", "0.04", "descriptionnew", "EUR");
	EscrowClaimRelease("node2", guid);
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
	string guid1 = EscrowNewBuyItNow("node2", "node1", "buyeraliasprune", offerguid, "1", "selleraliasprune");
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
	EscrowClaimRelease("node1", guid1);
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
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service
	BOOST_CHECK_THROW(CallRPC("node3", "escrowinfo " + guid1), runtime_error);
	BOOST_CHECK_EQUAL(EscrowFilter("node3", guid1), false);
	// cannot leave feedback on expired escrow
	BOOST_CHECK_THROW(CallRPC("node2",  "escrowfeedback " + guid1 + " buyer 1 2 3 4"), runtime_error);
	ECC_Stop();
}

BOOST_AUTO_TEST_SUITE_END ()