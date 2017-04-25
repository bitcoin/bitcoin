#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "util.h"
#include "rpc/server.h"
#include "alias.h"
#include "cert.h"
#include "base58.h"
#include "chainparams.h"
#include <boost/test/unit_test.hpp>
BOOST_GLOBAL_FIXTURE( SyscoinTestingSetup );

BOOST_FIXTURE_TEST_SUITE (syscoin_alias_tests, BasicSyscoinTestingSetup)

BOOST_AUTO_TEST_CASE (generate_sysrates_alias)
{
	printf("Running generate_sysrates_alias...\n");
	ECC_Start();
	CreateSysRatesIfNotExist();
	CreateSysBanIfNotExist();
	CreateSysCategoryIfNotExist();
}
BOOST_AUTO_TEST_CASE (generate_big_aliasdata)
{
	printf("Running generate_big_aliasdata...\n");
	GenerateBlocks(5);
	// 1024 bytes long
	string gooddata = "dasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsd";
	// 1025 bytes long
	string baddata =   "dasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsdz";
	string strCipherBadPrivData = "";
	string strCipherGoodPrivData = "";
	CKey privKey;
	privKey.MakeNewKey(true);
	CPubKey pubKey = privKey.GetPubKey();
	vector<unsigned char> vchPubKey(pubKey.begin(), pubKey.end());
	BOOST_CHECK_EQUAL(EncryptMessage(vchPubKey, baddata, strCipherBadPrivData), true);		
	BOOST_CHECK_EQUAL(EncryptMessage(vchPubKey, gooddata, strCipherGoodPrivData), true);	
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasnew sysrates.peg jag1 " + HexStr(vchFromString("password")) +  " " + gooddata + " " + HexStr(vchFromString(strCipherGoodPrivData))));
	GenerateBlocks(5);
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg jag2 " + HexStr(vchFromString("password")) +  " " + gooddata + " " + HexStr(vchFromString(strCipherBadPrivData))), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg jag2 " + HexStr(vchFromString("password")) +  " " + baddata + " " + HexStr(vchFromString(strCipherGoodPrivData))), runtime_error);
	
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
	AliasNew("node1", goodname, goodname, gooddata);
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg " + badname + " " + HexStr(vchFromString("password")) +  " 3d"), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_big_aliaspassword)
{
	printf("Running generate_big_aliaspassword...\n");
	GenerateBlocks(5);
	CKey privKey;
	privKey.MakeNewKey(true);
	CPubKey pubKey = privKey.GetPubKey();
	vector<unsigned char> vchPubKey(pubKey.begin(), pubKey.end());
	// 256 bytes long
	string gooddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";	
	// 257 bytes long
	string baddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddz";	
		
	string strCipherBadPassword = "";
	BOOST_CHECK_EQUAL(EncryptMessage(vchPubKey, baddata, strCipherBadPassword), true);	
	AliasNew("node1", "aliasname", gooddata, "a");
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasname1 " + HexStr(vchFromString(strCipherBadPassword)) + " pubdata"), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_aliasupdate)
{
	printf("Running generate_aliasupdate...\n");
	GenerateBlocks(1);
	AliasNew("node1", "jagupdate", "password", "data");
	AliasNew("node1", "jagupdate1", "password", "data");
	// update an alias that isn't yours
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagupdate"), runtime_error);
	// only update alias, no data
	AliasUpdate("node1", "jagupdate");
	AliasUpdate("node1", "jagupdate1");
	// update password only
	AliasUpdate("node1", "jagupdate", "\"\"", "\"\"", "newpass");
	AliasUpdate("node1", "jagupdate", "\"\"", "\"\"", "newpass1");
	AliasUpdate("node1", "jagupdate1", "\"\"", "\"\"","newpass");
	AliasUpdate("node1", "jagupdate1", "\"\"", "\"\"","newpass1");

}
BOOST_AUTO_TEST_CASE (generate_aliasmultiupdate)
{
	printf("Running generate_aliasmultiupdate...\n");
	GenerateBlocks(1);
	UniValue r;
	AliasNew("node1", "jagmultiupdate", "password", "data");
	AliasUpdate("node1", "jagmultiupdate", "changeddata", "privdata");
	// can do 5 free updates, 1 above and 4 below
	for(unsigned int i=0;i<MAX_ALIAS_UPDATES_PER_BLOCK-1;i++)
		BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg jagmultiupdate changedata1"));

	GenerateBlocks(10, "node1");
	GenerateBlocks(10, "node1");

	AliasTransfer("node1", "jagmultiupdate", "node2", "changeddata2", "pvtdata2");

	// after transfer it can't update alias even though there are utxo's available from old owner
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdate sysrates.peg jagmultiupdate changedata3"), runtime_error);

	// new owner can update
	for(unsigned int i=0;i<MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagmultiupdate changedata4"));

	// after generation MAX_ALIAS_UPDATES_PER_BLOCK utxo's should be available
	GenerateBlocks(10, "node2");
	GenerateBlocks(10, "node2");
	for(unsigned int i=0;i<MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagmultiupdate changedata5"));

	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagmultiupdate changedata6"), runtime_error);
	GenerateBlocks(10, "node2");
	GenerateBlocks(10, "node2");
	// transfer sends utxo's to new owner
	AliasTransfer("node2", "jagmultiupdate", "node1", "changeddata7", "");
	// ensure can't update after transfer
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagmultiupdate changedata8"), runtime_error);
	for(unsigned int i=0;i<MAX_ALIAS_UPDATES_PER_BLOCK;i++)
		BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg jagmultiupdate changedata9"));
	
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdate sysrates.peg jagmultiupdate changedata10"), runtime_error);
	GenerateBlocks(10, "node1");
	GenerateBlocks(10, "node1");
	AliasUpdate("node1", "jagmultiupdate", "changeddata11", "privdata");
}

BOOST_AUTO_TEST_CASE (generate_sendmoneytoalias)
{
	printf("Running generate_sendmoneytoalias...\n");
	GenerateBlocks(5, "node2");
	AliasNew("node2", "sendnode2", "password", "changeddata2");
	AliasNew("node3", "sendnode3", "password", "changeddata2");
	UniValue r;
	// get balance of node2 first to know we sent right amount oater
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo sendnode2"));
	CAmount balanceBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	string node2address = find_value(r.get_obj(), "address").get_str();
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress sendnode2 1.335"), runtime_error);
	GenerateBlocks(1);
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo sendnode3"));
	string node3address = find_value(r.get_obj(), "address").get_str();

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

	// pay to node2/node3 wallets for alias funding for tests
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress " + node2address + " 500000"), runtime_error);
	GenerateBlocks(10);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress " + node3address + " 500000"), runtime_error);
	GenerateBlocks(10);

}
BOOST_AUTO_TEST_CASE (generate_aliaspay)
{
	printf("Running generate_aliaspay...\n");
	UniValue r;
	GenerateBlocks(5);
	AliasNew("node1", "alias1.aliaspay.tld", "password", "changeddata2");
	AliasNew("node2", "alias2.aliaspay.tld", "password", "changeddata2");
	AliasNew("node3", "alias3.aliaspay.tld", "password", "changeddata2");
	GenerateBlocks(10);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress alias2.aliaspay.tld  2000"), runtime_error);
	GenerateBlocks(10);

	// get balance of node2 and node3 first to know we sent right amount later
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo alias2.aliaspay.tld"));
	string node2address = find_value(r.get_obj(), "address").get_str();
	CAmount balanceBeforeFrom = AmountFromValue(find_value(r.get_obj(), "balance"));

	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasinfo alias3.aliaspay.tld"));
	string node3address = find_value(r.get_obj(), "address").get_str();
	CAmount balanceBeforeTo = AmountFromValue(find_value(r.get_obj(), "balance"));

	//send amount
	BOOST_CHECK_NO_THROW(CallRPC("node2", "aliaspay alias2.aliaspay.tld USD \"{\\\"alias3.aliaspay.tld\\\":0.4}\""));
	GenerateBlocks(10, "node1");
	GenerateBlocks(10, "node2");
	GenerateBlocks(10, "node3");

	CAmount sysDiff = 0.4 * 2690.1 * COIN;
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo alias2.aliaspay.tld"));
	CAmount balanceAfterFrom = AmountFromValue(find_value(r.get_obj(), "balance"));
	CAmount balanceTestAfterFrom = balanceBeforeFrom - sysDiff;
	BOOST_CHECK_EQUAL(balanceAfterFrom, balanceTestAfterFrom);

	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasinfo alias3.aliaspay.tld"));
	CAmount balanceAfterTo = AmountFromValue(find_value(r.get_obj(), "balance"));
	CAmount balanceTestAfterTo = balanceBeforeTo + sysDiff;
	BOOST_CHECK_EQUAL(balanceAfterTo, balanceTestAfterTo);

	// pay to node2/node3 wallets for alias funding for tests
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress " + node2address + " 500000"), runtime_error);
	GenerateBlocks(10);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress " + node3address + " 500000"), runtime_error);
	GenerateBlocks(10);
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

	string offerguid = OfferNew("node1", "aliasnew", "category", "title", "1", "0.05", "description", "USD");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + offerguid));
	
	BOOST_CHECK_EQUAL(aliasnewexpiry ,  find_value(r.get_obj(), "expires_on").get_int64());
	OfferUpdate("node1", "aliasold", offerguid, "category", "title", "1", "0.05", "description", "USD");
	GenerateBlocks(5, "node1");
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + offerguid));
	BOOST_CHECK_EQUAL(aliasoldexpiry ,  find_value(r.get_obj(), "expires_on").get_int64());
	
	
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "offerinfo " + offerguid));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "alias").get_str(), "aliasold");	
	
	ExpireAlias("aliasold");
	GenerateBlocks(1, "node1");

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
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), 0);	
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo aliasnew"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), 0);	
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasinfo aliasnew"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), 0);	


	// node 3 doesn't download the offer since it expired while node 3 was offline
	BOOST_CHECK_THROW(r = CallRPC("node3", "offerinfo " + offerguid), runtime_error);
	BOOST_CHECK_EQUAL(OfferFilter("node3", offerguid, "Off"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node3", offerguid, "On"), false);

	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "offerinfo " + offerguid));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), 1);	
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "alias").get_str(), "aliasold");	
	BOOST_CHECK_EQUAL(aliasoldexpiry ,  find_value(r.get_obj(), "expires_on").get_int64());

}
BOOST_AUTO_TEST_CASE (generate_aliastransfer)
{
	printf("Running generate_aliastransfer...\n");
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	UniValue r;
	string strAddress1 = AliasNew("node1", "jagnode1", "password", "changeddata1");
	string strAddress2 = AliasNew("node2", "jagnode2", "password", "changeddata2");
	CKey privKey;
	privKey.MakeNewKey(true);
	CPubKey pubKey = privKey.GetPubKey();
	vector<unsigned char> vchPubKey(pubKey.begin(), pubKey.end());
	vector<unsigned char> vchPrivKey(privKey.begin(), privKey.end());
	CSyscoinAddress aliasAddress(pubKey.GetID());
	BOOST_CHECK(pubKey.IsFullyValid());
	BOOST_CHECK_NO_THROW(CallRPC("node2", "importprivkey " + CSyscoinSecret(privKey).ToString() + " \"\" false", true, false));	

	AliasTransfer("node1", "jagnode1", "node2", "changeddata1", "pvtdata");

	// xfer an alias that isn't yours
	BOOST_CHECK_THROW(r = CallRPC("node1", "aliasupdate sysrates.peg jagnode1 changedata1 \"\" \"\" " + aliasAddress.ToString()), runtime_error);

	// xfer alias and update it at the same time
	AliasTransfer("node2", "jagnode2", "node3", "changeddata4", "pvtdata");

	// update xferred alias
	AliasUpdate("node2", "jagnode1", "changeddata5", "pvtdata1");

	// rexfer alias
	AliasTransfer("node2", "jagnode1", "node3", "changeddata5", "pvtdata2");

	// xfer an alias to another alias is prohibited
	BOOST_CHECK_THROW(r = CallRPC("node2", "aliasupdate sysrates.peg jagnode2 changedata1 \"\" \"\" " + strAddress1), runtime_error);
	
}
BOOST_AUTO_TEST_CASE (generate_aliasbalance)
{
	printf("Running generate_aliasbalance...\n");
	UniValue r;
	// create alias and check balance is 0
	AliasNew("node2", "jagnodebalance1", "password", "changeddata1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodebalance1"));
	CAmount balanceBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceBefore, 10*COIN);

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
	AliasUpdate("node2", "jagnodebalance1", "pubdata1", "privdata1", "newpassword");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo jagnodebalance1"));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs(balanceBefore -  balanceAfter) < COIN);
	GenerateBlocks(5);
	ExpireAlias("jagnodebalance1");
	// renew alias, should clear balance
	AliasNew("node2", "jagnodebalance1", "newpassword123", "changeddata1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo jagnodebalance1"));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceAfter, 10*COIN);
}
BOOST_AUTO_TEST_CASE (generate_aliasbalancewithtransfer)
{
	printf("Running generate_aliasbalancewithtransfer...\n");
	UniValue r;
	AliasNew("node2", "jagnodebalance2", "password", "changeddata1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodebalance2"));
	CAmount balanceBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceBefore, 10*COIN);

	// send money to alias and check balance

	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance2 9"), runtime_error);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodebalance2"));
	balanceBefore += 9*COIN;
	CAmount balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceBefore, balanceAfter);

	// transfer alias to someone else and balance should be same
	AliasTransfer("node2", "jagnodebalance2", "node3", "changeddata4", "pvtdata");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo jagnodebalance2"));
	CAmount balanceAfterTransfer = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(balanceAfterTransfer >= (balanceBefore-COIN));

	// send money to alias and balance updates
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodebalance2 12.1"), runtime_error);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "aliasinfo jagnodebalance2"));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceAfter, 12.1*COIN+balanceAfterTransfer);

	// edit and balance should remain the same
	AliasUpdate("node3", "jagnodebalance2", "pubdata1", "privdata1", "newpassword");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo jagnodebalance2"));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs((12.1*COIN+balanceAfterTransfer) -  balanceAfter) < COIN);

	// transfer again and balance is same
	AliasTransfer("node3", "jagnodebalance2", "node2", "changeddata4", "pvtdata");
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo jagnodebalance2"));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(balanceAfter >= (12.1*COIN+balanceAfterTransfer)-COIN);

}
BOOST_AUTO_TEST_CASE (generate_multisigalias)
{
	printf("Running generate_multisigalias...\n");
	UniValue r;
	AliasNew("node1", "jagnodemultisig1", "password", "changeddata1");
	AliasNew("node2", "jagnodemultisig2", "password", "changeddata1");
	AliasNew("node3", "jagnodemultisig3", "password", "changeddata1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	// used for when setting multisig alias back to normal alias
	string oldAddressStr = find_value(r, "address").get_str();
	UniValue arrayParams(UniValue::VARR);
	UniValue arrayOfKeys(UniValue::VARR);

	// create 2 of 2
	UniValue resCreate;
	string redeemScript, addressStr;
	BOOST_CHECK_NO_THROW(resCreate = CallRPC("node1", "createmultisig 2 \"[\\\"jagnodemultisig1\\\",\\\"jagnodemultisig2\\\"]\""));	
	UniValue redeemScript_value = find_value(resCreate, "redeemScript");
	UniValue address_value = find_value(resCreate, "address");
	BOOST_CHECK(redeemScript_value.isStr());
	BOOST_CHECK(address_value.isStr());
	redeemScript = redeemScript_value.get_str();
	addressStr = address_value.get_str();
		
	string tmp = AliasUpdate("node1", "jagnodemultisig1", "pubdata", "privdata", "password", "\"\"", addressStr);
	BOOST_CHECK_EQUAL(tmp, "");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str(), addressStr);
	// change the multisigs pw and public data
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasaddscript " + redeemScript));
	// if updating p2sh alias and changing pw, must always pass in address aswell otherwise new address will be generated for you
	string hex_str = AliasUpdate("node1", "jagnodemultisig1", "pubdata1", "privdata1", "newpassword", "\"\"", addressStr);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str(), addressStr);
	BOOST_CHECK(!hex_str.empty());
	BOOST_CHECK_NO_THROW(CallRPC("node2", "aliasaddscript " + redeemScript));
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "syscoinsignrawtransaction " + hex_str));
	GenerateBlocks(5, "node2");
	GenerateBlocks(5);
	// pay to multisig and check balance
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	// make sure alias was updated with new public data
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str(), "pubdata1");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodemultisig1 9"), runtime_error);
	GenerateBlocks(5);
	GenerateBlocks(5, "node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	CAmount balanceBefore = 19*COIN;
	CAmount balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs(balanceBefore - balanceAfter) < COIN);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str(), addressStr);
	hex_str = AliasUpdate("node2", "jagnodemultisig1", "\"\"", "\"\"", "\"\"", "\"\"", addressStr);
	BOOST_CHECK(hex_str != "");

	// create 1 of 2
	BOOST_CHECK_NO_THROW(resCreate = CallRPC("node1", "createmultisig 1 \"[\\\"jagnodemultisig1\\\",\\\"jagnodemultisig2\\\"]\""));	
	redeemScript_value = find_value(resCreate, "redeemScript");
	address_value = find_value(resCreate, "address");
	BOOST_CHECK(redeemScript_value.isStr());
	BOOST_CHECK(address_value.isStr());
	redeemScript = redeemScript_value.get_str();
	addressStr = address_value.get_str();
	hex_str = AliasUpdate("node1", "jagnodemultisig1", "pubdata", "privdata", "password", "\"\"", addressStr);
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "syscoinsignrawtransaction " + hex_str));
	GenerateBlocks(5, "node2");
	GenerateBlocks(5);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str(), addressStr);
	// ensure only one signature is needed
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasaddscript " + redeemScript));
	hex_str = AliasUpdate("node1", "jagnodemultisig1", "\"\"", "\"\"", "newpassword");
	BOOST_CHECK_EQUAL(hex_str, "");
	// pay to multisig and check balance
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodemultisig1 8"), runtime_error);
	GenerateBlocks(5);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	balanceBefore += 8*COIN;
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs(balanceBefore - balanceAfter) < COIN);
	// create 2 of 3
	BOOST_CHECK_NO_THROW(resCreate = CallRPC("node1", "createmultisig 2 \"[\\\"jagnodemultisig1\\\",\\\"jagnodemultisig2\\\", \\\"jagnodemultisig3\\\"]\""));	
	redeemScript_value = find_value(resCreate, "redeemScript");
	address_value = find_value(resCreate, "address");
	BOOST_CHECK(redeemScript_value.isStr());
	BOOST_CHECK(address_value.isStr());
	redeemScript = redeemScript_value.get_str();
	addressStr = address_value.get_str();
	tmp = AliasUpdate("node1", "jagnodemultisig1", "pubdata", "privdata", "password", "\"\"", addressStr);
	BOOST_CHECK_EQUAL(tmp, "");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str(), addressStr);
	// 2 sigs needed, remove redeemScript to make it a normal alias
	BOOST_CHECK_NO_THROW(CallRPC("node3", "aliasaddscript " + redeemScript));
	hex_str = AliasUpdate("node3", "jagnodemultisig1", "\"\"", "\"\"", "\"\"", "\"\"", oldAddressStr);
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasaddscript " + redeemScript));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "syscoinsignrawtransaction " + hex_str));
	GenerateBlocks(5, "node3");
	GenerateBlocks(5);
	// pay to multisig and check balance
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str(), oldAddressStr);
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodemultisig1 7"), runtime_error);
	GenerateBlocks(5);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	balanceBefore += 7*COIN;
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs(balanceBefore - balanceAfter) < COIN);

	// no multisig so update as normal
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg jagnodemultisig1 changedata1 " + HexStr(vchFromString("pvtdata"))), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node3", "aliasupdate sysrates.peg jagnodemultisig1 changedata1 " + HexStr(vchFromString("pvtdata"))), runtime_error);
	hex_str = AliasUpdate("node1", "jagnodemultisig1", "\"\"", "\"\"", "newpassword1");
	BOOST_CHECK_EQUAL(hex_str, "");
	// pay to multisig and check balance
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress jagnodemultisig1 6"), runtime_error);
	GenerateBlocks(5);
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "aliasinfo jagnodemultisig1"));
	balanceBefore += 6*COIN;
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(abs(balanceBefore - balanceAfter) < COIN);
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
	AliasUpdate("node1", "jagsafesearch", "pubdata1", "privdata1","\"\"", "No");
	AliasUpdate("node1", "jagnonsafesearch", "pubdata2", "privdata2", "\"\"", "Yes");

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
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasexpirebuyback " + HexStr(vchFromString("password")) + " data"), runtime_error);
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
	AliasNew("node1", "aliasexpirebuyback1", "passwordnew3", "somedata1", "data1");
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
	AliasNew("node1", "aliasexpirebuyback2", "passwordnew5", "data", "data1");
	GenerateBlocks(10, "node1");
	GenerateBlocks(10, "node1");
	ExpireAlias("aliasexpirebuyback2");
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasexpirebuyback2", "On"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasexpirebuyback2", "On"), false);
	// renew second time
	AliasNew("node1", "aliasexpirebuyback2", "passwordnew6", "data2", "data2");
	GenerateBlocks(10, "node1");
	GenerateBlocks(10, "node1");
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
	GenerateBlocks(10, "node1");
	GenerateBlocks(10, "node2");
	AliasNew("node2", "aliasexpirebuyback", "passwordnew8", "somedata", "data");
	BOOST_CHECK_THROW(CallRPC("node2", "aliasnew sysrates.peg aliasexpirebuyback " + HexStr(vchFromString("passwordnew9")) + " data"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasexpirebuyback " + HexStr(vchFromString("password10")) + " data"), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdate sysrates.peg aliasexpirebuyback changedata1 " + HexStr(vchFromString("pvtdata"))), runtime_error);

	// this time steal the alias and try to recreate at the same time
	ExpireAlias("aliasexpirebuyback");
	AliasNew("node1", "aliasexpirebuyback", "passwordnew11", "somedata", "data");
	ExpireAlias("aliasexpirebuyback");
	AliasNew("node2", "aliasexpirebuyback", "passwordnew12", "somedata", "data");
	GenerateBlocks(5,"node2");
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew sysrates.peg aliasexpirebuyback " + HexStr(vchFromString("passwordnew13")) + " data2"), runtime_error);
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
	// ban both aliases level 1 (only owner of sysban can do this)
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
	
	// ban both aliases level 2 (only owner of sysban can do this)
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

	// unban both aliases (only owner of sysban can do this)
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

	// good case, safe offer with safe alias
	string offerguidsafe1 = OfferNew("node1", "jagbansafesearchoffer", "category", "title", "100", "1.00", "description", "USD", "\"\"", "\"\"", "\"\"", "Yes");
	// good case, unsafe offer with safe alias
	string offerguidsafe2 = OfferNew("node1", "jagbansafesearchoffer", "category", "title", "100", "1.00", "description", "USD", "\"\"", "\"\"", "\"\"", "No");
	// good case, unsafe offer with unsafe alias
	string offerguidsafe3 = OfferNew("node1", "jagbannonsafesearchoffer", "category", "title", "100", "1.00", "description", "USD", "\"\"", "\"\"", "\"\"", "No");

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
	string offerguidunsafe = OfferNew("node1", "jagbannonsafesearchoffer", "category", "title", "100", "1.00", "description", "USD", "\"\"", "\"\"", "\"\"", "Yes");
	// safe offer with unsafe alias should show only in safe search off mode
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidunsafe, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidunsafe, "Off"), true);

	// swap safe search fields on the aliases
	AliasUpdate("node1", "jagbansafesearchoffer", "pubdata1", "privatedata1", "\"\"", "No");	
	AliasUpdate("node1", "jagbannonsafesearchoffer", "pubdata1", "privatedata1", "\"\"", "Yes");

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
	OfferUpdate("node1", "jagbansafesearchoffer", offerguidsafe1, "category", "titlenew", "10", "1.00", "descriptionnew", "USD", "\"\"", "\"\"", "\"\"", "Yes");
	OfferUpdate("node1", "jagbansafesearchoffer", offerguidsafe2, "category", "titlenew", "90", "0.15", "descriptionnew", "USD", "\"\"", "\"\"", "\"\"", "No");
	// swap them back and check filters again
	AliasUpdate("node1", "jagbansafesearchoffer", "pubdata1", "privatedata1", "\"\"","Yes");	
	AliasUpdate("node1", "jagbannonsafesearchoffer", "pubdata1", "privatedata1", "\"\"","No");

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
	OfferUpdate("node1", "jagbannonsafesearchoffer", offerguidsafe3, "category", "titlenew", "10", "1.00", "descriptionnew", "USD", "\"\"", "\"\"", "\"\"", "No");
	// you won't be able to find it unless in safe search off mode because the alias doesn't actually change
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "Off"), true);	

	// unsafe offer with safe alias, edit to safe offer and change alias to unsafe 
	OfferUpdate("node1", "jagbannonsafesearchoffer", offerguidsafe2, "category", "titlenew", "90", "0.15", "descriptionnew", "USD", "\"\"", "\"\"", "\"\"", "Yes");
	// safe offer with unsafe alias should show when safe search off mode only
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe2, "Off"), true);

	// safe offer with safe alias, edit to unsafe offer
	OfferUpdate("node1", "jagbansafesearchoffer", offerguidsafe3, "category", "titlenew", "90", "0.15", "descriptionnew", "USD", "\"\"", "\"\"", "\"\"", "No");

	// keep alive and revert settings
	OfferUpdate("node1", "jagbansafesearchoffer", offerguidsafe1, "category", "titlenew", "10", "1.00", "descriptionnew", "USD", "\"\"", "\"\"", "\"\"", "Yes");
	AliasUpdate("node1", "jagbansafesearchoffer", "pubdata1", "privatedata1", "\"\"","Yes");	
	AliasUpdate("node1", "jagbannonsafesearchoffer", "pubdata1", "privatedata1", "\"\"","No");

	// unsafe offer with safe alias should show in safe off mode only
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "On"), false);
	BOOST_CHECK_EQUAL(OfferFilter("node1", offerguidsafe3, "Off"), true);

	// revert settings of offers
	OfferUpdate("node1", "jagbansafesearchoffer", offerguidsafe2, "category", "titlenew", "10", "1.00", "descriptionnew", "USD", "\"\"", "\"\"", "\"\"", "No");
	OfferUpdate("node1", "jagbannonsafesearchoffer", offerguidsafe3, "category", "titlenew", "10", "1.00", "descriptionnew", "USD", "\"\"", "\"\"", "\"\"", "No");	

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
	AliasNew("node1", "aliasprune", "password", "pubdata", "privdata");
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
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), 1);	

	// node2 shouldn't find the service at all (meaning node2 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node2", "aliasinfo aliasprune"), runtime_error);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasprune", "Off"), false);

	// stop node3
	StopNode("node3");
	// create a new service
	AliasNew("node1", "aliasprune1", "password", "pubdata", "privdata");
	GenerateBlocks(5, "node1");
	// stop and start node1
	StopNode("node1");
	StartNode("node1");
	GenerateBlocks(5, "node1");
	// ensure you can still update before expiry
	AliasUpdate("node1", "aliasprune1", "newdata", "privdata");

	// you can search it still on node1/node2
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasprune1", "Off"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasprune1", "Off"), true);
	GenerateBlocks(5, "node1");
	// ensure service is still active since its supposed to expire at 100 blocks of non updated services
	AliasUpdate("node1", "aliasprune1", "newdata1", "privdata1");

	// you can search it still on node1/node2
	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasprune1", "Off"), true);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasprune1", "Off"), true);
	ExpireAlias("aliasprune1");
	// now it should be expired
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg aliasprune1 newdata2 " + HexStr(vchFromString("privatedata"))), runtime_error);

	BOOST_CHECK_EQUAL(AliasFilter("node1", "aliasprune1", "Off"), false);
	BOOST_CHECK_EQUAL(AliasFilter("node2", "aliasprune1", "Off"), false);
	// and it should say its expired
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "aliasinfo aliasprune1"));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), 1);	

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
	AliasNew("node1", "aliasprunewithoffer", "password", "pubdata", "privdata");
	AliasNew("node1", "aliasprunewithoffer1", "password", "pubdata", "privdata");
	AliasNew("node2", "aliasprunewithoffer2", "password", "pubdata", "privdata");
	string offerguid = OfferNew("node1", "aliasprunewithoffer", "category", "title", "1", "0.05", "description", "SYS");
	string escrowguid = EscrowNew("node2", "node1", "aliasprunewithoffer2", offerguid, "1", "message", "aliasprunewithoffer1", "aliasprunewithoffer");
	EscrowRelease("node2", "buyer", escrowguid);
	EscrowClaimRelease("node1", escrowguid);
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
	AliasNew("node1", "aliasprunewithcertoffer", "password", "pubdata", "privdata");
	AliasNew("node2", "aliasprunewithcertoffer2", "password", "pubdata", "privdata");
	string certguid = CertNew("node1", "aliasprunewithcertoffer", "jag1", "privdata", "pubdata");
	string certofferguid = OfferNew("node1", "aliasprunewithcertoffer", "certificates", "title", "1", "0.05", "description", "SYS", certguid);	
	string offerguid = OfferNew("node1", "aliasprunewithcertoffer", "category", "title", "1", "0.05", "description", "SYS");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress aliasprunewithcertoffer2 300"), runtime_error);
	GenerateBlocks(10);	
	OfferUpdate("node1", "aliasprunewithcertoffer", offerguid, "category", "titlenew", "1", "0.05", "descriptionnew", "USD");
	OfferUpdate("node1", "aliasprunewithcertoffer", certofferguid, "certificates", "titlenew", "1", "0.05", "descriptionnew", "USD", "No", certguid);
	OfferAccept("node1", "node2", "aliasprunewithcertoffer2", certofferguid, "1", "message");
	OfferAccept("node1", "node2", "aliasprunewithcertoffer2", offerguid, "1", "message");
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
	AliasNew("node1", "aliasprunewithcert", "password", "pubdata", "privdata");
	AliasNew("node2", "aliasprunewithcert2", "password", "pubdata", "privdata");
	string certguid = CertNew("node1", "aliasprunewithcert", "jag1", "privdata", "pubdata");
	CertUpdate("node1", certguid, "\"\"", "\"\"", "newdata");
	CertTransfer("node1", "node2", certguid, "aliasprunewithcert2");
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
	string aliasexpirenode2address = AliasNew("node2", "aliasexpirednode2", "password", "somedata");
	string offerguid = OfferNew("node1", "aliasexpire0", "category", "title", "100", "0.01", "description", "USD");
	OfferAddWhitelist("node1", offerguid, "aliasexpirednode2", "5");
	string certguid = CertNew("node1", "aliasexpire", "certtitle", "privdata", "pubdata", "Yes");
	StopNode("node3");
	AliasNew("node1", "aliasexpire2", "password", "pubdata", "privdata");
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress aliasexpirednode2 300"), runtime_error);
	GenerateBlocks(10);	
	string escrowguid = EscrowNew("node2", "node1", "aliasexpirednode2", offerguid, "1", "message", "aliasexpire", "aliasexpire0", "5");
	string aliasexpire2node2address = AliasNew("node2", "aliasexpire2node2", "password", "pubdata", "privdata");
	string certgoodguid = CertNew("node1", "aliasexpire2", "certtitle", "privdata", "pubdata");
	ExpireAlias("aliasexpirednode2");
	GenerateBlocks(5, "node2");

	AliasNew("node1", "aliasexpire", "passwordnew", "pubdata", "privdata");
	AliasNew("node1", "aliasexpire0", "passwordnew", "pubdata", "privdata");
	AliasNew("node2", "aliasexpire1", "passwordnew", "pubdata", "privdata");
	CKey privKey;
	privKey.MakeNewKey(true);
	CPubKey pubKey = privKey.GetPubKey();
	vector<unsigned char> vchPubKey(pubKey.begin(), pubKey.end());
	vector<unsigned char> vchPrivKey(privKey.begin(), privKey.end());	
	BOOST_CHECK(pubKey.IsFullyValid());
	CSyscoinAddress aliasAddress(pubKey.GetID());
	// should fail: alias update on expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg aliasexpirednode2 newdata1"), runtime_error);
	// should fail: alias transfer from expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate sysrates.peg aliasexpirednode2 changedata1 \"\" \"\" " + aliasAddress.ToString()), runtime_error);
	// should fail: alias transfer to another alias
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdate sysrates.peg aliasexpire2 changedata1 \"\" \"\" " + aliasexpire2node2address), runtime_error);

	// should fail: link to an expired alias in offer
	BOOST_CHECK_THROW(CallRPC("node2", "offerlink aliasexpirednode2 " + offerguid + " 5 newdetails"), runtime_error);
	// should fail: generate an offer using expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "offernew aliasexpirednode2 category title 1 0.05 description USD"), runtime_error);

	// should fail: send message from expired alias to expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "messagenew " + HexStr(vchFromString("message")) + " pubdata aliasexpirednode2 aliasexpirednode2 01 01 01"), runtime_error);
	// should fail: send message from expired alias to non-expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "messagenew " + HexStr(vchFromString("message")) + " pubdata aliasexpirednode2 aliasexpire 01 01 01"), runtime_error);
	// should fail: send message from non-expired alias to expired alias
	BOOST_CHECK_THROW(CallRPC("node1", "messagenew " + HexStr(vchFromString("message")) + " pubdata aliasexpire aliasexpirednode2 01 01 01"), runtime_error);

	// should fail: new escrow with expired arbiter alias
	BOOST_CHECK_THROW(CallRPC("node2", "escrownew aliasexpire2node2 " + offerguid + " 1 " + HexStr(vchFromString("message")) + " aliasexpirednode2"), runtime_error);
	// should fail: new escrow with expired alias
	BOOST_CHECK_THROW(CallRPC("node2", "escrownew aliasexpirednode2 " + offerguid + " 1 " + HexStr(vchFromString("message")) + " aliasexpire"), runtime_error);

	BOOST_CHECK_NO_THROW(CallRPC("node1", "messagenew " + HexStr(vchFromString("message")) + " pubdata aliasexpire aliasexpire2 01 01 01"));
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg aliasexpire newdata1"));
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg aliasexpire2 newdata1"));
	GenerateBlocks(5, "node1");
	
	BOOST_CHECK_NO_THROW(CallRPC("node1", "certupdate " + certgoodguid + " titlenew privdata pubdata"));
	BOOST_CHECK_NO_THROW(CallRPC("node1", "offerupdate aliasexpire0 " + offerguid + " category title 100 0.05 description"));
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(CallRPC("node1", "certupdate " + certguid + " jag1 data pubdata"));
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
	BOOST_CHECK(aliasexpirenode2address != AliasNew("node2", "aliasexpirednode2", "passwordnew3", "somedata"));
	CertUpdate("node1", certgoodguid, "pubdata", "newdata");
	// able to release and claim release on escrow with non-expired aliases with new pubkeys
	EscrowRelease("node2", "buyer", escrowguid);	 
	EscrowClaimRelease("node1", escrowguid); 

	ExpireAlias("aliasexpire2");
	// should fail: update cert with expired alias
	BOOST_CHECK_THROW(CallRPC("node1", "certupdate " + certgoodguid + " title pubdata"), runtime_error);
	// should fail: xfer an cert with expired alias
	BOOST_CHECK_THROW(CallRPC("node1", "certtransfer " + certgoodguid + " aliasexpire2"), runtime_error);
	// should fail: xfer an cert to an expired alias even though transferring cert is good
	BOOST_CHECK_THROW(CallRPC("node1", "certtransfer " + certguid + " aliasexpire2"), runtime_error);

	AliasNew("node2", "aliasexpire2", "passwordnew3", "somedata");
	// should fail: cert alias not owned by node1
	BOOST_CHECK_THROW(CallRPC("node1", "certtransfer " + certgoodguid + " aliasexpirednode2"), runtime_error);
	ExpireAlias("aliasexpire2");
	AliasNew("node2", "aliasexpirednode2", "passwordnew3a", "somedataa");
	AliasNew("node1", "aliasexpire2", "passwordnew3b", "somedatab");
	// should pass: confirm that the transferring cert is good by transferring to a good alias
	CertTransfer("node1", "node2", certgoodguid, "aliasexpirednode2");
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