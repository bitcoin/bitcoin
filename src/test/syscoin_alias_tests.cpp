#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "rpc/server.h"
#include "alias.h"
#include <boost/test/unit_test.hpp>
BOOST_GLOBAL_FIXTURE( SyscoinTestingSetup );

BOOST_FIXTURE_TEST_SUITE (syscoin_alias_tests, BasicSyscoinTestingSetup)

BOOST_AUTO_TEST_CASE (generate_sysrates_alias)
{
	printf("Running generate_sysrates_alias...\n");
	CreateSysRatesIfNotExist();
	CreateSysBanIfNotExist();
}
BOOST_AUTO_TEST_CASE (generate_big_aliasdata)
{
	printf("Running generate_big_aliasdata...\n");
	GenerateBlocks(5);
	// 1024 bytes long
	string gooddata = "dasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsd";
	// 1025 bytes long
	string baddata =   "dasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsdz";
	AliasNew("node1", "jag",  "password", gooddata);
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg jag1 password temp " + baddata), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_big_aliasname)
{
	printf("Running generate_big_aliasname...\n");
	GenerateBlocks(5);
	// 64 bytes long
	string goodname = "sfsdfdfsdsfsfsdfdfsdsfdsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdsfsdfdd";
	// 1024 bytes long
	string gooddata = "dasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsd";	
	// 65 bytes long
	string badname =  "sfsdfdfsdsfsfsdfdfsdsfdsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdsfsddfda";
	AliasNew("node1", goodname, "password", "a");
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg " + badname + " password 3d"), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_big_aliaspassword)
{
	printf("Running generate_big_aliaspassword...\n");
	GenerateBlocks(5);
	// 256 bytes long
	string gooddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";	
	// 257 bytes long
	string baddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddz";	
		
	AliasNew("node1", "aliasname", gooddata, "a");
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasname1 " + baddata + " 3d"), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_aliasupdate)
{
	printf("Running generate_aliasupdate...\n");
	GenerateBlocks(1);
	AliasNew("node1", "jagupdate", "password", "data");
	// update an alias that isn't yours
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagupdate test"), runtime_error);
	AliasUpdate("node1", "jagupdate", "changeddata", "privdata");
	// shouldnt update data, just uses prev data because it hasnt changed
	AliasUpdate("node1", "jagupdate", "changeddata", "privdata");

}
BOOST_AUTO_TEST_CASE (generate_aliasmultiupdate)
{
	printf("Running generate_aliasmultiupdate...\n");
	GenerateBlocks(1);
	UniValue r;
	AliasNew("node1", "jagmultiupdate", "password", "data");
	AliasUpdate("node1", "jagmultiupdate", "changeddata", "privdata");
	for(unsigned int i=0;i<MAX_ALIAS_UPDATES_PER_BLOCK-1;i++)
		BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg jagmultiupdate changedata1 pvtdata"));

	GenerateBlocks(10, "node1");
	GenerateBlocks(10, "node1");

	AliasTransfer("node1", "jagmultiupdate", "node2", "changeddata5", "pvtdata2");

	// after transfer it can't update alias even though there are utxo's available from old owner
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdate sysrates.peg jagmultiupdate changedata1 pvtdata"), runtime_error);

	// new owner can update
	BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagmultiupdate changedata1b pvtdata"));
	// on transfer only 1 utxo available, to create more you have to run through an update first with a confirmation
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagmultiupdate changedata1b pvtdata"), runtime_error);
	// after generation MAX_ALIAS_UPDATES_PER_BLOCK utxo's should be available
	GenerateBlocks(10, "node2");
	GenerateBlocks(10, "node2");
	for(unsigned int i=0;i<MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagmultiupdate changedata1b pvtdata"));

	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagmultiupdate changedata10b pvtdata"), runtime_error);
	GenerateBlocks(10, "node2");
	GenerateBlocks(10, "node2");
	// try transfers and updates in parallel
	UniValue pkr = CallRPC("node1", "generatepublickey");
	BOOST_CHECK(pkr.type() == UniValue::VARR);
	const UniValue &resultArray = pkr.get_array();
	string newPubkey = resultArray[0].get_str();	
	for(unsigned int i=0;i<MAX_ALIAS_UPDATES_PER_BLOCK;i++)
	{
		if((i%2) == 0)
		{
			BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagmultiupdate changedata1 pvtdata"));
		}
		else
		{
			BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagmultiupdate changedata1 pvtdata Yes " + newPubkey));
		}

	}
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagmultiupdate changedata10b pvtdata"), runtime_error);
	GenerateBlocks(10, "node2");
	GenerateBlocks(10, "node2");
}

BOOST_AUTO_TEST_CASE (generate_sendmoneytoalias)
{
	printf("Running generate_sendmoneytoalias...\n");
	GenerateBlocks(5, "node2");
	AliasNew("node2", "sendnode2", "password", "changeddata2");
	UniValue r;
	// get balance of node2 first to know we sent right amount oater
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo sendnode2"));
	CAmount balanceBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress sendnode2 1.335"), runtime_error);
	GenerateBlocks(1);
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo sendnode2"));
	balanceBefore += 1.335*COIN;
	CAmount balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceBefore, balanceAfter);
	// after expiry can still send money to it
	GenerateBlocks(101);	
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress sendnode2 1.335"), runtime_error);
	GenerateBlocks(1);
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo sendnode2"));
	balanceBefore += 1.335*COIN;
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceBefore, balanceAfter);


}
BOOST_AUTO_TEST_CASE (generate_alias_offerexpiry_resync)
{
	printf("Running generate_offer_aliasexpiry_resync...\n");
	UniValue r;
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	// change offer to an older alias, expire the alias and ensure that on resync the offer seems to be expired still
	AliasNew("node1", "aliasold", "password", "changeddata1");
	printf("Sleeping 5 seconds between the creation of the two aliases for this test...\n");
	MilliSleep(5000);
	GenerateBlocks(5, "node1");
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	GenerateBlocks(50);
	AliasNew("node1", "aliasnew", "passworda", "changeddata1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo aliasold"));
	int64_t aliasoldexpiry = find_value(r.get_obj(), "expires_on").get_int64();	
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo aliasnew"));
	int64_t aliasnewexpiry = find_value(r.get_obj(), "expires_on").get_int64();	
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getblockchaininfo"));
	int64_t mediantime = find_value(r.get_obj(), "mediantime").get_int64();	
	BOOST_CHECK(aliasoldexpiry > mediantime);
	BOOST_CHECK(aliasoldexpiry < aliasnewexpiry);
	StopNode("node3");
	GenerateBlocks(5, "node1");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offernew aliasnew category title 1 0.05 description USD"));
	const UniValue &arr = r.get_array();
	string offerguid = arr[1].get_str();
	GenerateBlocks(5, "node1");
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + offerguid));
	BOOST_CHECK_EQUAL(aliasnewexpiry ,  find_value(r.get_obj(), "expires_on").get_int64());
	
	BOOST_CHECK_NO_THROW(CallRPC("node1", "offerupdate aliasold " + offerguid + " category title 1 0.05 description"));
	GenerateBlocks(5, "node1");
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + offerguid));
	BOOST_CHECK_EQUAL(aliasoldexpiry ,  find_value(r.get_obj(), "expires_on").get_int64());
	
	
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + offerguid));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "alias").get_str(), "aliasold");	
	
	ExpireAlias("aliasold");
	GenerateBlocks(5, "node1");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getblockchaininfo"));
	mediantime = find_value(r.get_obj(), "mediantime").get_int64();	
	BOOST_CHECK(aliasoldexpiry <= mediantime);
	BOOST_CHECK(aliasnewexpiry > mediantime);

	StopNode("node1");
	StartNode("node1");

	// aliasnew should still be active, but offer was set to aliasold so it should be expired
	ExpireAlias("aliasold");
	GenerateBlocks(5, "node1");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getblockchaininfo"));
	mediantime = find_value(r.get_obj(), "mediantime").get_int64();	
	BOOST_CHECK(aliasoldexpiry <= mediantime);
	BOOST_CHECK(aliasnewexpiry > mediantime);

	BOOST_CHECK_THROW(r = CallRPC("node1", "aliasinfo aliasold"), runtime_error);


	BOOST_CHECK_THROW(r = CallRPC("node1", "offerinfo " + offerguid), runtime_error);


	StartNode("node3");
	ExpireAlias("aliasold");
	GenerateBlocks(5, "node3");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getblockchaininfo"));
	mediantime = find_value(r.get_obj(), "mediantime").get_int64();	
	BOOST_CHECK(aliasoldexpiry <= mediantime);
	BOOST_CHECK(aliasnewexpiry > mediantime);

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo aliasnew"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_int(), 0);	
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo aliasnew"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_int(), 0);	
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasinfo aliasnew"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_int(), 0);	


	// node 3 doesn't download the offer since it expired while node 3 was offline
	BOOST_CHECK_THROW(r = CallRPC("node3", "offerinfo " + offerguid), runtime_error);
	BOOST_CHECK_EQUAL(OfferFilter("node3", offerguid, "Off"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node3", offerguid, "On"), false);

	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "offerinfo " + offerguid));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_int(), 1);	
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "alias").get_str(), "aliasold");	
	BOOST_CHECK_EQUAL(aliasoldexpiry ,  find_value(r.get_obj(), "expires_on").get_int64());

}
BOOST_AUTO_TEST_CASE (generate_aliastransfer)
{
	printf("Running generate_aliastransfer...\n");
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	UniValue r;
	string strPubKey1 = AliasNew("node1", "jagnode1", "password", "changeddata1");
	string strPubKey2 = AliasNew("node2", "jagnode2", "password", "changeddata2");
	UniValue pkr = CallRPC("node2", "generatepublickey");
	BOOST_CHECK(pkr.type() == UniValue::VARR);
	const UniValue &resultArray = pkr.get_array();
	string newPubkey = resultArray[0].get_str();	
	AliasTransfer("node1", "jagnode1", "node2", "changeddata1", "pvtdata");

	// xfer an alias that isn't yours
	BOOST_CHECK_THROW(r = CallRPC("node1", "aliasupdate sysrates.peg jagnode1 changedata1 pvtdata Yes " + newPubkey), runtime_error);

	// xfer alias and update it at the same time
	AliasTransfer("node2", "jagnode2", "node3", "changeddata4", "pvtdata");

	// update xferred alias
	AliasUpdate("node2", "jagnode1", "changeddata5", "pvtdata1");

	// rexfer alias
	AliasTransfer("node2", "jagnode1", "node3", "changeddata5", "pvtdata2");

	// xfer an alias to another alias is prohibited
	BOOST_CHECK_THROW(r = CallRPC("node2", "aliasupdate sysrates.peg jagnode2 changedata1 pvtdata Yes " + strPubKey1), runtime_error);
	
}
BOOST_AUTO_TEST_CASE (generate_aliasbalance)
{
	printf("Running generate_aliasbalance...\n");
	UniValue r;
	// create alias and check balance is 0
	AliasNew("node2", "jagnodebalance1", "password", "changeddata1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodebalance1"));
	CAmount balanceBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceBefore, 0);

	// send money to alias and check balance is updated
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 1.5"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 1.6"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 1"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 2"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 3"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 4"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 5"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 2"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 9"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 11"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 10.45"), runtime_error);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 10"), runtime_error);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance1 20"), runtime_error);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodebalance1"));
	CAmount balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	balanceBefore += 80.55*COIN;
	BOOST_CHECK_EQUAL(balanceBefore, balanceAfter);

	// edit password and see balance is same
	AliasUpdate("node2", "jagnodebalance1", "pubdata1", "privdata1", "No", "newpassword");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo jagnodebalance1"));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs(balanceBefore -  balanceAfter) < COIN);
	GenerateBlocks(5);
	ExpireAlias("jagnodebalance1");
	// renew alias, should transfer balances
	AliasNew("node2", "jagnodebalance1", "newpassword123", "changeddata1");
	// ensure balance is transferred on renewal
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo jagnodebalance1"));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs(balanceBefore -  balanceAfter) < COIN);
}
BOOST_AUTO_TEST_CASE (generate_aliasbalancewithtransfer)
{
	printf("Running generate_aliasbalancewithtransfer...\n");
	UniValue r;
	AliasNew("node2", "jagnodebalance2", "password", "changeddata1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodebalance2"));
	CAmount balanceBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceBefore, 0);

	// send money to alias and check balance

	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance2 9"), runtime_error);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodebalance2"));
	balanceBefore += 9*COIN;
	CAmount balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceBefore, balanceAfter);

	// transfer alias to someone else and balance should be 0
	AliasTransfer("node2", "jagnodebalance2", "node3", "changeddata4", "pvtdata");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo jagnodebalance2"));
	CAmount balanceAfterTransfer = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceAfterTransfer, 0);

	// send money to alias and balance updates
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance2 12.1"), runtime_error);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasinfo jagnodebalance2"));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceAfter, 12.1*COIN);

	// edit and balance should remain the same
	AliasUpdate("node3", "jagnodebalance2", "pubdata1", "privdata1", "No", "newpassword");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo jagnodebalance2"));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs(12.1*COIN -  balanceAfter) < COIN);

	// transfer again and balance is 0 again
	AliasTransfer("node3", "jagnodebalance2", "node2", "changeddata4", "pvtdata");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo jagnodebalance2"));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceAfter, 0);

}
BOOST_AUTO_TEST_CASE (generate_multisigalias)
{
	printf("Running generate_multisigalias...\n");
	AliasNew("node2", "jagnodemultisig2", "password", "changeddata1");
	AliasNew("node3", "jagnodemultisig3", "password", "changeddata1");
	// create 2 of 2
	//UniValue v(UniValue::VARR);
	//v.push_back("jagnodemultisig2");
	//AliasNew("node1", "jagnodemultisig1", "password", "changeddata1", "privdata", "Yes", "2", v.write());
	//AliasNew("node1", "jagnodemultisig1", "password", "changeddata1", "privdata", "Yes", "2", "[{\\\"jagnodemultisig1\\\",\\\"jagnodemultisig2\\\"}]");
	// create 1 of 2
	// create 2 of 3

	// change the multisigs pw
	// pay to multisig and check balance
	// remove multisig and update as normal
}
BOOST_AUTO_TEST_CASE (generate_aliasbalancewithtransfermultisig)
{
	printf("Running generate_aliasbalancewithtransfermultisig...\n");
	// create 2 of 3 alias
	// send money to alias and check balance
	// transfer alias to non multisig  and balance should be 0
	// send money to alias and balance updates
	// edit and balance should remain the same
	// transfer again and balance is 0 again

}
BOOST_AUTO_TEST_CASE (generate_aliasauthentication)
{
	// create alias with some password and try to get authentication key
	// update the password and try again

	// edit alias with authentication key from wallet that doesnt own that alias
}
BOOST_AUTO_TEST_CASE (generate_aliasauthenticationmultisig)
{
	// create 2 of 3 alias with some password and try to get authentication key
	// update the password and try again

	// edit alias with authentication key from wallet that doesnt own that alias
	// pass that tx to another alias and verify it got signed and pushed to network
}
BOOST_AUTO_TEST_CASE (generate_aliassafesearch)
{
	printf("Running generate_aliassafesearch...\n");
	UniValue r;
	GenerateBlocks(1);
	// alias is safe to search
	AliasNew("node1", "jagsafesearch", "pubdata", "password", "privdata", "Yes");
	// not safe to search
	AliasNew("node1", "jagnonsafesearch", "pubdata", "password", "privdata", "No");
	// should include result in both safe search mode on and off
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagsafesearch", "On"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagsafesearch", "Off"), true);

	// should only show up if safe search is off
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagnonsafesearch", "On"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagnonsafesearch", "Off"), true);

	// shouldn't affect aliasinfo
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagsafesearch"));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnonsafesearch"));

	// reverse the rolls
	AliasUpdate("node1", "jagsafesearch", "pubdata1", "privdata1", "No");
	AliasUpdate("node1", "jagnonsafesearch", "pubdata2", "privdata2", "Yes");

	// should include result in both safe search mode on and off
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagsafesearch", "Off"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagsafesearch", "On"), false);

	// should only regardless of safesearch
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagnonsafesearch", "Off"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagnonsafesearch", "On"), true);

	// shouldn't affect aliasinfo
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagsafesearch"));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnonsafesearch"));


}

BOOST_AUTO_TEST_CASE (generate_aliasexpiredbuyback)
{
	printf("Running generate_aliasexpiredbuyback...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	
	AliasNew("node1", "aliasexpirebuyback", "passwordnew1", "somedata", "data");
	// can't renew aliases that aren't expired
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasexpirebuyback password data"), runtime_error);
	ExpireAlias("aliasexpirebuyback");
	// expired aliases shouldnt be searchable
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasexpirebuyback", "On"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasexpirebuyback", "On"), false);
	
	// renew alias (with same password) and now its searchable
	AliasNew("node1", "aliasexpirebuyback", "passwordnew1", "somedata1", "data1");
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasexpirebuyback", "On"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasexpirebuyback", "On"), true);

	ExpireAlias("aliasexpirebuyback");
	// try to renew alias again second time
	AliasNew("node1", "aliasexpirebuyback", "passwordnew3", "somedata2", "data2");
	// run the test with node3 offline to test pruning with renewing alias
	StopNode("node3");
	MilliSleep(500);
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasexpirebuyback1 passwordnew4 data"));
	GenerateBlocks(5, "node1");
	ExpireAlias("aliasexpirebuyback1");
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasexpirebuyback1", "On"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasexpirebuyback1", "On"), false);

	StartNode("node3");
	ExpireAlias("aliasexpirebuyback1");
	GenerateBlocks(5, "node3");
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasexpirebuyback1", "On"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasexpirebuyback1", "On"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node3", "aliasexpirebuyback1", "On"), false);
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "aliasinfo aliasexpirebuyback1"), runtime_error);

	// run the test with node3 offline to test pruning with renewing alias twice
	StopNode("node3");
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasexpirebuyback2 passwordnew5 data"));
	GenerateBlocks(5, "node1");
	ExpireAlias("aliasexpirebuyback2");
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasexpirebuyback2", "On"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasexpirebuyback2", "On"), false);
	// renew second time
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasexpirebuyback2 passwordnew6 data"));
	GenerateBlocks(5, "node1");
	ExpireAlias("aliasexpirebuyback2");
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasexpirebuyback2", "On"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasexpirebuyback2", "On"), false);
	StartNode("node3");
	ExpireAlias("aliasexpirebuyback2");
	GenerateBlocks(5, "node3");
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasexpirebuyback2", "On"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasexpirebuyback2", "On"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node3", "aliasexpirebuyback2", "On"), false);
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "aliasinfo aliasexpirebuyback2"), runtime_error);
	ExpireAlias("aliasexpirebuyback");
	// steal alias after expiry and original node try to recreate or update should fail
	AliasNew("node1", "aliasexpirebuyback", "passwordnew7", "somedata", "data");
	ExpireAlias("aliasexpirebuyback");
	AliasNew("node2", "aliasexpirebuyback", "passwordnew8", "somedata", "data");
	BOOST_CHECK_THROW(CallRPC("node2", "aliasnew sysrates.peg aliasexpirebuyback passwordnew9 data"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasexpirebuyback passwordnew10 data"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdate sysrates.peg aliasexpirebuyback changedata1 pvtdata"), runtime_error);

	// this time steal the alias and try to recreate at the same time
	ExpireAlias("aliasexpirebuyback");
	AliasNew("node1", "aliasexpirebuyback", "passwordnew11", "somedata", "data");
	ExpireAlias("aliasexpirebuyback");
	AliasNew("node2", "aliasexpirebuyback", "passwordnew12", "somedata", "data");
	GenerateBlocks(5,"node2");
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasexpirebuyback passwordnew13 data2"), runtime_error);
	GenerateBlocks(5);
}

BOOST_AUTO_TEST_CASE (generate_aliasban)
{
	printf("Running generate_aliasban...\n");
	UniValue r;
	GenerateBlocks(10);
	// 2 aliases, one will be banned that is safe searchable other is banned that is not safe searchable
	AliasNew("node1", "jagbansafesearch", "password", "pubdata", "privdata", "Yes");
	AliasNew("node1", "jagbannonsafesearch", "password", "pubdata", "privdata", "No");
	// can't ban on any other node than one that created sysban
	BOOST_CHECK_THROW(AliasBan("node2","jagbansafesearch",SAFETY_LEVEL1), runtime_error);
	BOOST_CHECK_THROW(AliasBan("node3","jagbansafesearch",SAFETY_LEVEL1), runtime_error);
	// ban both aliases level 1 (only owner of syscategory can do this)
	BOOST_CHECK_NO_THROW(AliasBan("node1","jagbansafesearch",SAFETY_LEVEL1));
	BOOST_CHECK_NO_THROW(AliasBan("node1","jagbannonsafesearch",SAFETY_LEVEL1));
	// should only show level 1 banned if safe search filter is not used
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbansafesearch", "On"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbansafesearch", "Off"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbannonsafesearch", "On"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbannonsafesearch", "Off"), true);
	// should be able to aliasinfo on level 1 banned aliases
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagbansafesearch"));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagbannonsafesearch"));
	
	// ban both aliases level 2 (only owner of syscategory can do this)
	BOOST_CHECK_NO_THROW(AliasBan("node1","jagbansafesearch",SAFETY_LEVEL2));
	BOOST_CHECK_NO_THROW(AliasBan("node1","jagbannonsafesearch",SAFETY_LEVEL2));
	// no matter what filter won't show banned aliases
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbansafesearch", "On"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbansafesearch", "Off"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbannonsafesearch", "On"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbannonsafesearch", "Off"), false);

	// shouldn't be able to aliasinfo on level 2 banned aliases
	BOOST_CHECK_THROW(r = CallRPC("node1", "aliasinfo jagbansafesearch"), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "aliasinfo jagbannonsafesearch"), runtime_error);

	// unban both aliases (only owner of syscategory can do this)
	BOOST_CHECK_NO_THROW(AliasBan("node1","jagbansafesearch",0));
	BOOST_CHECK_NO_THROW(AliasBan("node1","jagbannonsafesearch",0));
	// safe to search regardless of filter
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbansafesearch", "On"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbansafesearch", "Off"), true);

	// since safesearch is set to false on this alias, it won't show up in search still
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbannonsafesearch", "On"), false);
	// it will if you are not doing a safe search
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbannonsafesearch", "Off"), true);

	// should be able to aliasinfo on non banned aliases
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagbansafesearch"));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagbannonsafesearch"));
	
}

BOOST_AUTO_TEST_CASE (generate_aliasbanwithoffers)
{
	printf("Running generate_aliasbanwithoffers...\n");
	UniValue r;
	GenerateBlocks(10);
	// 2 aliases, one will be banned that is safe searchable other is banned that is not safe searchable
	AliasNew("node1", "jagbansafesearchoffer", "password", "pubdata", "privdata", "Yes");
	AliasNew("node1", "jagbannonsafesearchoffer", "password", "pubdata", "privdata", "No");
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbansafesearchoffer", "On"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbansafesearchoffer", "Off"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbannonsafesearchoffer", "On"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node1", "jagbannonsafesearchoffer", "Off"), true);

	// good case, safe offer with safe alias
	string offerguidsafe1 = OfferNew("node1", "jagbansafesearchoffer", "category", "title", "100", "1.00", "description", "USD", "nocert", "NONE", "location", "Yes");
	// good case, unsafe offer with safe alias
	string offerguidsafe2 = OfferNew("node1", "jagbansafesearchoffer", "category", "title", "100", "1.00", "description", "USD", "nocert", "NONE", "location", "No");
	// good case, unsafe offer with unsafe alias
	string offerguidsafe3 = OfferNew("node1", "jagbannonsafesearchoffer", "category", "title", "100", "1.00", "description", "USD", "nocert", "NONE", "location", "No");

	// safe offer with safe alias should show regardless of safe search
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe1, "On"), true);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe1, "Off"), true);
	// unsafe offer with safe alias should show only in safe search off mode
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "Off"), true);
	// unsafe offer with unsafe alias should show only in safe search off mode
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "Off"), true);

	// safe offer with unsafe alias
	string offerguidunsafe = OfferNew("node1", "jagbannonsafesearchoffer", "category", "title", "100", "1.00", "description", "USD", "nocert", "NONE", "location", "Yes");
	// safe offer with unsafe alias should show only in safe search off mode
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidunsafe, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidunsafe, "Off"), true);

	// swap safe search fields on the aliases
	AliasUpdate("node1", "jagbansafesearchoffer", "pubdata1", "privatedata1", "No");	
	AliasUpdate("node1", "jagbannonsafesearchoffer", "pubdata1", "privatedata1", "Yes");

	// safe offer with unsafe alias should show only in safe search off mode
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe1, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe1, "Off"), true);
	// unsafe offer with unsafe alias should show only in safe search off mode
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "Off"), true);
	// unsafe offer with safe alias should show only in safe search off mode
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "Off"), true);

	// keep alive
	OfferUpdate("node1", "jagbansafesearchoffer", offerguidsafe1, "category", "titlenew", "10", "1.00", "descriptionnew", "USD", false, "nocert", "location", "Yes");
	OfferUpdate("node1", "jagbansafesearchoffer", offerguidsafe2, "category", "titlenew", "90", "0.15", "descriptionnew", "USD", false, "nocert", "location", "No");
	// swap them back and check filters again
	AliasUpdate("node1", "jagbansafesearchoffer", "pubdata1", "privatedata1", "Yes");	
	AliasUpdate("node1", "jagbannonsafesearchoffer", "pubdata1", "privatedata1", "No");

	// safe offer with safe alias should show regardless of safe search
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe1, "On"), true);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe1, "Off"), true);
	// unsafe offer with safe alias should show only in safe search off mode
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "Off"), true);
	// unsafe offer with unsafe alias should show only in safe search off mode
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "Off"), true);

	// unsafe offer with unsafe alias, edit the offer to safe set offer to not safe
	OfferUpdate("node1", "jagbannonsafesearchoffer", offerguidsafe3, "category", "titlenew", "10", "1.00", "descriptionnew", "USD", false, "nocert", "location", "No");
	// you won't be able to find it unless in safe search off mode because the alias doesn't actually change
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "Off"), true);	

	// unsafe offer with safe alias, edit to safe offer and change alias to unsafe 
	OfferUpdate("node1", "jagbannonsafesearchoffer", offerguidsafe2, "category", "titlenew", "90", "0.15", "descriptionnew", "USD", false, "nocert", "location", "Yes");
	// safe offer with unsafe alias should show when safe search off mode only
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "Off"), true);

	// safe offer with safe alias, edit to unsafe offer
	OfferUpdate("node1", "jagbansafesearchoffer", offerguidsafe3, "category", "titlenew", "90", "0.15", "descriptionnew", "USD", false, "nocert", "location", "No");

	// keep alive and revert settings
	OfferUpdate("node1", "jagbansafesearchoffer", offerguidsafe1, "category", "titlenew", "10", "1.00", "descriptionnew", "USD", false, "nocert", "location", "Yes");
	AliasUpdate("node1", "jagbansafesearchoffer", "pubdata1", "privatedata1", "Yes");	
	AliasUpdate("node1", "jagbannonsafesearchoffer", "pubdata1", "privatedata1", "No");

	// unsafe offer with safe alias should show in safe off mode only
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "Off"), true);

	// revert settings of offers
	OfferUpdate("node1", "jagbansafesearchoffer", offerguidsafe2, "category", "titlenew", "10", "1.00", "descriptionnew", "USD", false, "nocert", "location", "No");
	OfferUpdate("node1", "jagbannonsafesearchoffer", offerguidsafe3, "category", "titlenew", "10", "1.00", "descriptionnew", "USD", false, "nocert", "location", "No");	

	// ban both aliases level 1 (only owner of syscategory can do this)
	BOOST_CHECK_NO_THROW(AliasBan("node1","jagbansafesearchoffer",SAFETY_LEVEL1));
	BOOST_CHECK_NO_THROW(AliasBan("node1","jagbannonsafesearchoffer",SAFETY_LEVEL1));
	// should only show level 1 banned if safe search filter is used
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe1, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe1, "Off"), true);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "Off"), true);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "Off"), true);
	// should be able to offerinfo on level 1 banned aliases
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + offerguidsafe1));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + offerguidsafe2));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + offerguidsafe3));


	// ban both aliases level 2 (only owner of syscategory can do this)
	BOOST_CHECK_NO_THROW(AliasBan("node1","jagbansafesearchoffer",SAFETY_LEVEL2));
	BOOST_CHECK_NO_THROW(AliasBan("node1","jagbannonsafesearchoffer",SAFETY_LEVEL2));
	// no matter what filter won't show banned aliases
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe1, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe1, "Off"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "Off"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "Off"), false);

	// shouldn't be able to offerinfo on level 2 banned aliases
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerinfo " + offerguidsafe1), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerinfo " + offerguidsafe2), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "offerinfo " + offerguidsafe3), runtime_error);


	// unban both aliases (only owner of syscategory can do this)
	BOOST_CHECK_NO_THROW(AliasBan("node1","jagbansafesearchoffer",0));
	BOOST_CHECK_NO_THROW(AliasBan("node1","jagbannonsafesearchoffer",0));
	// back to original settings
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe1, "On"), true);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe1, "Off"), true);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "Off"), true);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "Off"), true);


	// should be able to offerinfo on non banned aliases
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + offerguidsafe1));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + offerguidsafe2));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + offerguidsafe3));
	
}
BOOST_AUTO_TEST_CASE (generate_aliaspruning)
{
	UniValue r;
	// makes sure services expire in 100 blocks instead of 1 year of blocks for testing purposes

	printf("Running generate_aliaspruning...\n");
	// stop node2 create a service,  mine some blocks to expire the service, when we restart the node the service data won't be synced with node2
	StopNode("node2");
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasprune password data"));
	GenerateBlocks(5, "node1");
	// we can find it as normal first
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasprune", "Off"), true);
	// then we let the service expire
	ExpireAlias("aliasprune");
	StartNode("node2");
	ExpireAlias("aliasprune");
	GenerateBlocks(5, "node2");
	// now we shouldn't be able to search it
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasprune", "Off"), false);
	// and it should say its expired
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo aliasprune"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_int(), 1);	

	// node2 shouldn't find the service at all (meaning node2 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node2", "aliasinfo aliasprune"), runtime_error);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasprune", "Off"), false);

	// stop node3
	StopNode("node3");
	// create a new service
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasprune1 password data"));
	GenerateBlocks(5, "node1");
	// stop and start node1
	StopNode("node1");
	StartNode("node1");
	GenerateBlocks(5, "node1");
	// ensure you can still update before expiry
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg aliasprune1 newdata privdata"));
	// you can search it still on node1/node2
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasprune1", "Off"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasprune1", "Off"), true);
	GenerateBlocks(5, "node1");
	// ensure service is still active since its supposed to expire at 100 blocks of non updated services
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg aliasprune1 newdata1 privdata"));
	// you can search it still on node1/node2
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasprune1", "Off"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasprune1", "Off"), true);
	ExpireAlias("aliasprune1");
	// now it should be expired
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg aliasprune1 newdata2 privdata"), runtime_error);
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasprune1", "Off"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasprune1", "Off"), false);
	// and it should say its expired
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo aliasprune1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_int(), 1);	

	StartNode("node3");
	ExpireAlias("aliasprune");
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "aliasinfo aliasprune1"), runtime_error);
	BOOST_CHECK_EQUAL(AliasFilter("node3", "aliasprune1", "Off"), false);
}
BOOST_AUTO_TEST_CASE (generate_aliasprunewithoffer)
{
	printf("Running generate_aliasprunewithoffer...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	StopNode("node3");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasnew sysrates.peg aliasprunewithoffer password somedata"));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasnew sysrates.peg aliasprunewithoffer1 password somedata"));
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasnew sysrates.peg aliasprunewithoffer2 password somedata"));
	GenerateBlocks(5, "node1");
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offernew aliasprunewithoffer category title 1 0.05 description USD"));
	const UniValue &arr = r.get_array();
	string offerguid = arr[1].get_str();
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "escrownew aliasprunewithoffer2 " + offerguid + " 1 message aliasprunewithoffer1"));
	const UniValue &array = r.get_array();
	string escrowguid = array[1].get_str();	
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(CallRPC("node2", "escrowrelease " + escrowguid + " buyer"));
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "escrowclaimrelease " + escrowguid));
	UniValue retArray = r.get_array();
	BOOST_CHECK_NO_THROW(CallRPC("node1", "escrowcompleterelease " + escrowguid + " " + retArray[0].get_str()));
	GenerateBlocks(5, "node1");
	// last created alias should have furthest expiry
	ExpireAlias("aliasprunewithoffer2");
	StartNode("node3");
	ExpireAlias("aliasprunewithoffer2");
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "escrowinfo " + escrowguid), runtime_error);
	BOOST_CHECK_EQUAL(EscrowFilter("node3", escrowguid), false);
}
BOOST_AUTO_TEST_CASE (generate_aliasprunewithcertoffer)
{
	printf("Running generate_aliasprunewithcertoffer...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	StopNode("node3");
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasprunewithcertoffer password somedata"));
	BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasnew sysrates.peg aliasprunewithcertoffer2 password somedata"));
	GenerateBlocks(5, "node1");
	GenerateBlocks(5, "node2");
	MilliSleep(2500);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certnew aliasprunewithcertoffer jag1 data pubdata"));
	const UniValue &arr = r.get_array();
	string certguid = arr[1].get_str();
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offernew aliasprunewithcertoffer certificates title 1 0.05 description USD " + certguid));
	const UniValue &arr1 = r.get_array();
	string certofferguid = arr1[1].get_str();
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offernew aliasprunewithcertoffer category title 1 0.05 description USD"));
	const UniValue &arr2 = r.get_array();
	string offerguid = arr2[1].get_str();
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(CallRPC("node1", "offerupdate aliasprunewithcertoffer " + offerguid + " category title 1 0.05 description"));
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(CallRPC("node1", "offerupdate aliasprunewithcertoffer " + certofferguid + " certificates title 1 0.05 description USD 0 " + certguid));
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "offeraccept aliasprunewithcertoffer2 " + certofferguid + " 1 message"));
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "offeraccept aliasprunewithcertoffer2 " + offerguid + " 1 message"));
	GenerateBlocks(5, "node2");
	ExpireAlias("aliasprunewithcertoffer2");
	StartNode("node3");
	ExpireAlias("aliasprunewithcertoffer2");
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "offerinfo " + offerguid), runtime_error);
	BOOST_CHECK_EQUAL(OfferFilter("node3", offerguid, "Off"), false);
}

BOOST_AUTO_TEST_CASE (generate_aliasprunewithcert)
{
	printf("Running generate_aliasprunewithcert...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	StopNode("node3");
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasprunewithcert password somedata"));
	BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasnew sysrates.peg aliasprunewithcert2 password somedata"));
	GenerateBlocks(5, "node1");
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certnew aliasprunewithcert jag1 data pubdata"));
	const UniValue &arr = r.get_array();
	string certguid = arr[1].get_str();
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(CallRPC("node1", "certupdate " + certguid + " aliasprunewithcert newdata privdata pubdata"));
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(CallRPC("node1", "certtransfer " + certguid + " aliasprunewithcert2"));
	GenerateBlocks(5, "node1");
	ExpireAlias("aliasprunewithcert2");
	StartNode("node3");
	ExpireAlias("aliasprunewithcert2");
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "certinfo " + certguid), runtime_error);
	BOOST_CHECK_EQUAL(OfferFilter("node3", certguid, "Off"), false);
}
BOOST_AUTO_TEST_CASE (generate_aliasexpired)
{
	printf("Running generate_aliasexpired...\n");
	UniValue r;
	
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");

	AliasNew("node1", "aliasexpire", "password", "somedata");
	AliasNew("node1", "aliasexpire0", "password", "somedata");
	AliasNew("node2", "aliasexpire1", "password", "somedata");
	string aliasexpirenode2pubkey = AliasNew("node2", "aliasexpirednode2", "password", "somedata");
	string offerguid = OfferNew("node1", "aliasexpire0", "category", "title", "100", "0.01", "description", "USD");
	OfferAddWhitelist("node1", offerguid, "aliasexpirednode2", "5");
	string certguid = CertNew("node1", "aliasexpire", "certtitle", "certdata", "pubdata", "Yes");
	StopNode("node3");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasnew sysrates.peg aliasexpire2 passwordnew somedata"));
	const UniValue &array1 = r.get_array();
	string aliasexpire2pubkey = array1[1].get_str();
	GenerateBlocks(5, "node1");
	GenerateBlocks(5, "node2");

	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "escrownew aliasexpirednode2 " + offerguid + " 1 message aliasexpire0"));
	const UniValue &array = r.get_array();
	string escrowguid = array[1].get_str();	
	GenerateBlocks(5, "node2");

	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasnew sysrates.peg aliasexpire2node2 passwordnew somedata"));
	const UniValue &array2 = r.get_array();
	string aliasexpire2node2pubkey = array2[1].get_str();	
	GenerateBlocks(5, "node2");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certnew aliasexpire2 certtitle certdata pubdata"));
	const UniValue &array3 = r.get_array();
	string certgoodguid = array3[1].get_str();	
	// expire aliasexpirednode2 and everything before
	ExpireAlias("aliasexpirednode2");
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasnew sysrates.peg aliasexpire passwordnew somedata"));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasnew sysrates.peg aliasexpire0 passwordnew somedata"));
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasnew sysrates.peg aliasexpire1 passwordnew somedata"));
	GenerateBlocks(5, "node1");
	GenerateBlocks(5, "node2");
	UniValue pkr = CallRPC("node2", "generatepublickey");
	if (pkr.type() != UniValue::VARR)
		throw runtime_error("Could not parse rpc results");

	const UniValue &resultArray = pkr.get_array();
	string pubkey = resultArray[0].get_str();		

	// should fail: alias update on expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg aliasexpirednode2 newdata1 privdata"), runtime_error);
	// should fail: alias transfer from expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg aliasexpirednode2 changedata1 pvtdata Yes " + pubkey), runtime_error);
	// should fail: alias transfer to another alias
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdate sysrates.peg aliasexpire2 changedata1 pvtdata Yes " + aliasexpirenode2pubkey), runtime_error);

	// should fail: link to an expired alias in offer
	BOOST_CHECK_THROW(CallRPC("node2", "offerlink aliasexpirednode2 " + offerguid + " 5 newdescription"), runtime_error);
	// should fail: generate an offer using expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "offernew aliasexpirednode2 category title 1 0.05 description USD nocert"), runtime_error);

	// should fail: send message from expired alias to expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "messagenew subject title aliasexpirednode2 aliasexpirednode2"), runtime_error);
	// should fail: send message from expired alias to non-expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "messagenew subject title aliasexpirednode2 aliasexpire"), runtime_error);
	// should fail: send message from non-expired alias to expired alias
	BOOST_CHECK_THROW(CallRPC("node1", "messagenew subject title aliasexpire aliasexpirednode2"), runtime_error);

	// should fail: new escrow with expired arbiter alias
	BOOST_CHECK_THROW(CallRPC("node2", "escrownew aliasexpire2node2 " + offerguid + " 1 message aliasexpirednode2"), runtime_error);
	// should fail: new escrow with expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "escrownew aliasexpirednode2 " + offerguid + " 1 message aliasexpire"), runtime_error);

	// keep alive for later calls
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg aliasexpire newdata1 privdata"));
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg aliasexpire2 newdata1 privdata"));
	GenerateBlocks(5, "node1");
	
	BOOST_CHECK_NO_THROW(CallRPC("node1", "certupdate " + certgoodguid + " aliasexpire2 newdata privdata pubdata"));
	BOOST_CHECK_NO_THROW(CallRPC("node1", "offerupdate aliasexpire0 " + offerguid + " category title 100 0.05 description"));
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(CallRPC("node1", "certupdate " + certguid + " aliasexpire jag1 data pubdata"));
	GenerateBlocks(5, "node1");

	StartNode("node3");
	ExpireAlias("aliasexpirednode2");
	
	GenerateBlocks(5, "node3");
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node1");
	// since all aliases are expired related to that escrow, the escrow was pruned
	BOOST_CHECK_THROW(CallRPC("node3", "escrowinfo " + escrowguid), runtime_error);
	// and node2
	BOOST_CHECK_NO_THROW(CallRPC("node2", "escrowinfo " + escrowguid));
	// this will recreate the alias and give it a new pubkey.. we need to use the old pubkey to sign the multisig, the escrow rpc call must check for the right pubkey
	BOOST_CHECK(aliasexpirenode2pubkey != AliasNew("node2", "aliasexpirednode2", "passwordnew3", "somedata"));
	BOOST_CHECK_NO_THROW(CallRPC("node1", "certupdate " + certgoodguid + " aliasexpire2 newdata privdata pubdata"));
	// able to release and claim release on escrow with non-expired aliases with new pubkeys
	EscrowRelease("node2", "buyer", escrowguid);	 
	EscrowClaimRelease("node1", escrowguid); 

	ExpireAlias("aliasexpire2");
	// should fail: update cert with expired alias
	BOOST_CHECK_THROW(CallRPC("node1", "certupdate " + certguid + " aliasexpire jag1 data pubdata"), runtime_error);
	// should fail: xfer an cert with expired alias
	BOOST_CHECK_THROW(CallRPC("node1", "certtransfer " + certguid + " aliasexpire2"), runtime_error);
	// should fail: xfer an cert to an expired alias even though transferring cert is good
	BOOST_CHECK_THROW(CallRPC("node1", "certtransfer " + certgoodguid + " aliasexpire1"), runtime_error);

	AliasNew("node2", "aliasexpire2", "passwordnew3", "somedata");
	// should fail: cert alias not owned by node1
	BOOST_CHECK_THROW(CallRPC("node1", "certtransfer " + certgoodguid + " aliasexpirednode2"), runtime_error);
	ExpireAlias("aliasexpire2");
	AliasNew("node2", "aliasexpirednode2", "passwordnew3a", "somedataa");
	AliasNew("node1", "aliasexpire2", "passwordnew3b", "somedatab");
	// should pass: confirm that the transferring cert is good by transferring to a good alias
	BOOST_CHECK_NO_THROW(CallRPC("node1", "certtransfer " + certgoodguid + " aliasexpirednode2"));
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certgoodguid));
	// ensure it got transferred
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "alias").get_str(), "aliasexpirednode2");

	ExpireAlias("aliasexpire2");
	// should fail: generate a cert using expired alias
	BOOST_CHECK_THROW(CallRPC("node1", "certnew aliasexpire2 jag1 data pubdata"), runtime_error);
	// renew alias with new password after expiry
	AliasNew("node2", "aliasexpirednode2", "passwordnew4", "somedata");
}
BOOST_AUTO_TEST_SUITE_END ()