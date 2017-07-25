#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "rpc/server.h"
#include "alias.h"
#include "cert.h"
#include "base58.h"
#include <boost/test/unit_test.hpp>
BOOST_FIXTURE_TEST_SUITE (syscoin_cert_tests, BasicSyscoinTestingSetup)

BOOST_AUTO_TEST_CASE (generate_big_certdata)
{
	printf("Running generate_big_certdata...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagcertbig1", "data");
	// 1024 bytes long
	string gooddata = "dasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsd";
	// 1110 bytes long
	string baddata = "dasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadddfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsddfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdf";
	string strCipherBadData = "";
	string strCipherGoodData = "";
	CKey privKey;
	privKey.MakeNewKey(true);
	CPubKey pubKey = privKey.GetPubKey();
	vector<unsigned char> vchPubKey(pubKey.begin(), pubKey.end());
	strCipherBadData = baddata;
	strCipherGoodData = gooddata;
	string guid = CertNew("node1", "jagcertbig1", "title", gooddata, gooddata);
	BOOST_CHECK_NO_THROW(CallRPC("node1", "certnew jagcertbig1 title \"\" " + strCipherGoodData));
	BOOST_CHECK_THROW(CallRPC("node1", "certnew jagcertbig1 title \"\" " + strCipherBadData), runtime_error);
	BOOST_CHECK_THROW(CallRPC("node1", "certnew jagcertbig1 title " + gooddata + " " + strCipherBadData), runtime_error);
	// update cert with long pub data
	BOOST_CHECK_THROW(CallRPC("node1", "certupdate " + guid + " title " + baddata + " " + strCipherGoodData), runtime_error);
	MilliSleep(2500);
	// trying to update to bad data for pub and priv
	BOOST_CHECK_THROW(CallRPC("node1", "certupdate " + guid + " title " + baddata + " " + strCipherBadData), runtime_error);

}
BOOST_AUTO_TEST_CASE (generate_big_certtitle)
{
	printf("Running generate_big_certtitle...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagcertbig2", "data");
	// 256 bytes long
	string goodtitle = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	// 1024 bytes long
	string gooddata = "asdfadsdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsd";	
	// 257 bytes long
	string badtitle =   "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddz";
	CertNew("node1", "jagcertbig2", goodtitle, "a", "pub");
	BOOST_CHECK_THROW(CallRPC("node1", "certnew jagcertbig2 " + badtitle + " priv pub"), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_certupdate)
{
	printf("Running generate_certupdate...\n");
	AliasNew("node1", "jagcertupdate", "data");
	string guid = CertNew("node1", "jagcertupdate", "private", "title", "data");
	// update an cert that isn't yours
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "certupdate " + guid + " title pubdata data"));
	const UniValue& resArray = r.get_array();
	if(resArray.size() > 1)
	{
		const UniValue& complete_value = resArray[1];
		bool bComplete = false;
		if (complete_value.isStr())
			bComplete = complete_value.get_str() == "true";
		BOOST_CHECK(!bComplete);
	}
	CertUpdate("node1", guid, "pub1");
	// shouldnt update data, just uses prev data because it hasnt changed
	CertUpdate("node1", guid);

}
BOOST_AUTO_TEST_CASE (generate_certtransfer)
{
	printf("Running generate_certtransfer...\n");
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	AliasNew("node1", "jagcert1", "changeddata1");
	AliasNew("node2", "jagcert2", "changeddata2");
	AliasNew("node3", "jagcert3", "changeddata3");
	string guid, pvtguid, certtitle, certdata;
	certtitle = "certtitle";
	certdata = "certdata";
	guid = CertNew("node1", "jagcert1", certtitle, certdata, "pubdata");
	// private cert
	pvtguid = CertNew("node1", "jagcert1", certtitle, certdata, "pubdata");
	CertUpdate("node1", pvtguid, certtitle, certdata, "pub3");
	UniValue r;
	CertTransfer("node1", "node2", guid, "jagcert2");
	CertTransfer("node1", "node3", pvtguid, "jagcert3");

	// xfer an cert that isn't yours
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certtransfer " + guid + " jagcert2"));
	const UniValue& resArray = r.get_array();
	if(resArray.size() > 1)
	{
		const UniValue& complete_value = resArray[1];
		bool bComplete = false;
		if (complete_value.isStr())
			bComplete = complete_value.get_str() == "true";
		BOOST_CHECK(!bComplete);
	}
	// update xferred cert
	certdata = "newdata";
	certtitle = "newtitle";
	CertUpdate("node2", guid, certtitle, certdata, "public");

	// retransfer cert
	CertTransfer("node2","node3", guid, "jagcert3");
}
BOOST_AUTO_TEST_CASE (generate_certsafesearch)
{
	printf("Running generate_certsafesearch...\n");
	UniValue r;
	GenerateBlocks(1);
	AliasNew("node1", "jagsafesearch1", "changeddata1");
	// cert is safe to search
	string certguidsafe = CertNew("node1", "jagsafesearch1", "certtitle", "certdata", "public", "true");
	// not safe to search
	string certguidnotsafe = CertNew("node1", "jagsafesearch1", "certtitle", "certdata", "public", "false");
	// should include result in both safe search mode on and off
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "true"), true);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "false"), true);

	// should only show up if safe search is off
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "true"), false);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "false"), true);

	// shouldn't affect certinfo
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certguidsafe));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certguidnotsafe));

	// reverse the rolls
	CertUpdate("node1", certguidsafe, "certtitle", "certdata", "pub", "false");
	CertUpdate("node1", certguidnotsafe,  "certtitle", "certdata", "pub1", "true");

	// should include result in both safe search mode on and off
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "false"), true);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "true"), false);

	// should only show up if safe search is off
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "false"), true);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "true"), true);

	// shouldn't affect certinfo
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certguidsafe));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certguidnotsafe));


}
BOOST_AUTO_TEST_CASE (generate_certban)
{
	printf("Running generate_certban...\n");
	UniValue r;
	GenerateBlocks(1);
	// cert is safe to search
	string certguidsafe = CertNew("node1", "jagsafesearch1", "certtitle", "certdata", "pub", "true");
	// not safe to search
	string certguidnotsafe = CertNew("node1", "jagsafesearch1", "certtitle", "certdata", "pub", "false");
	// ban both certs level 1 (only owner of syscategory can do this)
	BOOST_CHECK_NO_THROW(CertBan("node1",certguidsafe,SAFETY_LEVEL1));
	BOOST_CHECK_NO_THROW(CertBan("node1",certguidnotsafe,SAFETY_LEVEL1));
	// should only show level 1 banned if safe search filter is not used
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "true"), false);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "false"), true);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "true"), false);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "false"), true);
	// should be able to certinfo on level 1 banned certs
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certguidsafe));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certguidnotsafe));
	
	// ban both certs level 2 (only owner of syscategory can do this)
	BOOST_CHECK_NO_THROW(CertBan("node1",certguidsafe,SAFETY_LEVEL2));
	BOOST_CHECK_NO_THROW(CertBan("node1",certguidnotsafe,SAFETY_LEVEL2));
	// no matter what filter won't show banned certs
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "true"), false);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "false"), false);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "true"), false);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "false"), false);

	// shouldn't be able to certinfo on level 2 banned certs
	BOOST_CHECK_THROW(r = CallRPC("node1", "certinfo " + certguidsafe), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "certinfo " + certguidnotsafe), runtime_error);

	// unban both certs (only owner of syscategory can do this)
	BOOST_CHECK_NO_THROW(CertBan("node1",certguidsafe,0));
	BOOST_CHECK_NO_THROW(CertBan("node1",certguidnotsafe,0));
	// safe to search regardless of filter
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "true"), true);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "false"), true);

	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "true"), false);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "false"), true);

	// should be able to certinfo on non banned certs
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certguidsafe));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certguidnotsafe));
	
}

BOOST_AUTO_TEST_CASE (generate_certpruning)
{
	UniValue r;
	// makes sure services expire in 100 blocks instead of 1 year of blocks for testing purposes
	printf("Running generate_certpruning...\n");
	AliasNew("node1", "jagprune1", "changeddata1");
	// stop node2 create a service,  mine some blocks to expire the service, when we restart the node the service data won't be synced with node2
	StopNode("node2");
	string guid = CertNew("node1", "jagprune1", "jag1", "privdata", "pubdata");
	// we can find it as normal first
	BOOST_CHECK_EQUAL(CertFilter("node1", guid, "false"), true);
	// make sure our offer alias doesn't expire
	string hex_str = AliasUpdate("node1", "jagprune1");
	BOOST_CHECK(hex_str.empty());
	GenerateBlocks(5, "node1");
	ExpireAlias("jagprune1");
	StartNode("node2");
	ExpireAlias("jagprune1");
	GenerateBlocks(5, "node2");

	BOOST_CHECK_EQUAL(CertFilter("node1", guid, "false"), false);

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + guid));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), 1);	

	// should be pruned
	BOOST_CHECK_THROW(CallRPC("node2", "offerinfo " + guid), runtime_error);

	// stop node3
	StopNode("node3");
	// should fail: already expired alias
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdate sysrates.peg jagprune1 newdata"), runtime_error);
	GenerateBlocks(5, "node1");
	// create a new service
	AliasNew("node1", "jagprune1", "temp", "data");
	string guid1 = CertNew("node1", "jagprune1", "title", "privdata", "pubdata");
	// stop and start node1
	StopNode("node1");
	StartNode("node1");
	GenerateBlocks(5, "node1");
	// ensure you can still update before expiry
	CertUpdate("node1", guid1, "pubdata1", "privdata");
	// you can search it still on node1/node2
	BOOST_CHECK_EQUAL(CertFilter("node1", guid1, "false"), true);
	BOOST_CHECK_EQUAL(CertFilter("node2", guid1, "false"), true);
	// make sure our offer alias doesn't expire
	hex_str = AliasUpdate("node1", "jagprune1");
	BOOST_CHECK(hex_str.empty());
	GenerateBlocks(5, "node1");
	ExpireAlias("jagprune1");
	// now it should be expired
	BOOST_CHECK_THROW(CallRPC("node1",  "certupdate " + guid1 + " title pubdata3 newdata1"), runtime_error);
	GenerateBlocks(5, "node1");
	BOOST_CHECK_EQUAL(CertFilter("node1", guid1, "false"), false);
	BOOST_CHECK_EQUAL(CertFilter("node2", guid1, "false"), false);
	// and it should say its expired
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "certinfo " + guid1));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), 1);	
	GenerateBlocks(5, "node1");
	StartNode("node3");
	ExpireAlias("jagprune1");
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "certinfo " + guid1), runtime_error);
	BOOST_CHECK_EQUAL(CertFilter("node3", guid1, "false"), false);
}
BOOST_AUTO_TEST_SUITE_END ()