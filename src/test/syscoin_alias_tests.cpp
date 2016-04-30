#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "rpcserver.h"
#include <boost/test/unit_test.hpp>
BOOST_GLOBAL_FIXTURE( SyscoinTestingSetup );

BOOST_FIXTURE_TEST_SUITE (syscoin_alias_tests, BasicSyscoinTestingSetup)

BOOST_AUTO_TEST_CASE (generate_sysrates_alias)
{
	CreateSysRatesIfNotExist();
}
BOOST_AUTO_TEST_CASE (generate_big_aliasdata)
{
	GenerateBlocks(50);
	// 1023 bytes long
	string gooddata = "asdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsd";
	// 1024 bytes long
	string baddata =   "asdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsdz";
	AliasNew("node1", "jag", gooddata);
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew jag1 " + baddata), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_big_aliasname)
{
	GenerateBlocks(50);
	// 255 bytes long
	string goodname = "SfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	// 1023 bytes long
	string gooddata = "asdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsd";	
	// 256 bytes long
	string badname =   "SfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddz";
	AliasNew("node1", goodname, "a");
	BOOST_CHECK_THROW(CallRPC("node1", "aliasnew " + badname + " 3d"), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_aliasupdate)
{
	GenerateBlocks(1);
	// update an alias that isn't yours
	BOOST_CHECK_THROW(CallRPC("node2", "aliasupdate jag test"), runtime_error);
	AliasUpdate("node1", "jag", "changeddata", "privdata");
	// shouldnt update data, just uses prev data because it hasnt changed
	AliasUpdate("node1", "jag", "changeddata", "privdata");

}
BOOST_AUTO_TEST_CASE (generate_sendmoneytoalias)
{
	GenerateBlocks(200, "node2");
	AliasNew("node2", "sendnode2", "changeddata2");
	UniValue r;
	// get balance of node2 first to know we sent right amount oater
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "getinfo"));
	CAmount balanceBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_THROW(CallRPC("node1", "sendtoaddress sendnode2 1.335"), runtime_error);
	GenerateBlocks(1);
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "getinfo"));
	// 54.13 since 1 block matures
	balanceBefore += 1.335*COIN + 54.13*COIN;
	CAmount balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_EQUAL(balanceBefore, balanceAfter);
}
BOOST_AUTO_TEST_CASE (generate_aliastransfer)
{
	GenerateBlocks(200, "node2");
	GenerateBlocks(200, "node3");

	string strPubKey1 = AliasNew("node1", "jagnode1", "changeddata1");
	string strPubKey2 = AliasNew("node2", "jagnode2", "changeddata2");

	BOOST_CHECK_NO_THROW(AliasTransfer("node1", "jagnode1", "node2", "changeddata1", "pvtdata"));

	// xfer an alias that isn't yours
	BOOST_CHECK_THROW(AliasTransfer("node1", "jagnode1", "node2", "changeddata1", "pvtdata"), runtime_error);

	// trasnfer alias and update it at the same time
	BOOST_CHECK_NO_THROW(AliasTransfer("node2", "jagnode2", "node3", "changeddata4", "pvtdata"));

	// update xferred alias
	BOOST_CHECK_NO_THROW(AliasUpdate("node2", "jagnode1", "changeddata5", "pvtdata1"));

	// retransfer alias
	BOOST_CHECK_NO_THROW(AliasTransfer("node2", "jagnode1", "node3", "changeddata5", "pvtdata2"));

	// xfer an alias to another alias is prohibited
	BOOST_CHECK_THROW(AliasTransfer("node2", "jagnode2", "node1", "changeddata1", "pvtdata", strPubKey1), runtime_error);
	
}
BOOST_AUTO_TEST_SUITE_END ()