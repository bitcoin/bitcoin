#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "rpc/server.h"
#include "alias.h"
#include <boost/test/unit_test.hpp>
BOOST_FIXTURE_TEST_SUITE (syscoin_cert_tests, BasicSyscoinTestingSetup)

BOOST_AUTO_TEST_CASE (generate_big_certdata)
{
	printf("Running generate_big_certdata...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagcertbig1", "password", "data");
	// 1024 bytes long
	string gooddata = "dasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsd";
	// 1025 bytes long
	string baddata =  "dasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsdz";
	string guid = CertNew("node1", "jagcertbig1", "jag", gooddata, gooddata);
	// unencrypted 1025 bytes should cause us to trip 1108 bytes once encrypted
	BOOST_CHECK_THROW(CallRPC("node1", "certnew jagcertbig1 jag1 " + baddata + " " + gooddata), runtime_error);
	// update cert with long pub data
	BOOST_CHECK_THROW(CallRPC("node1", "certupdate " + guid + " jagcertbig1 jag1 " + gooddata + " " + baddata), runtime_error);
	MilliSleep(2500);
	// trying to update to bad data for pub and priv
	BOOST_CHECK_THROW(CallRPC("node1", "certupdate " + guid + " jagcertbig1 jag1 " + baddata + " " + baddata), runtime_error);

}
BOOST_AUTO_TEST_CASE (generate_big_certtitle)
{
	printf("Running generate_big_certtitle...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagcertbig2", "password", "data");
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
	AliasNew("node1", "jagcertupdate", "password", "data");
	string guid = CertNew("node1", "jagcertupdate", "password", "title", "data");
	// update an cert that isn't yours
	BOOST_CHECK_THROW(CallRPC("node2", "certupdate " + guid + " jagcertupdate title data pubdata"), runtime_error);
	CertUpdate("node1", guid, "jagcertupdate", "changedtitle", "changeddata", "pub1");
	// shouldnt update data, just uses prev data because it hasnt changed
	CertUpdate("node1", guid, "jagcertupdate", "changedtitle", "changeddata", "pub2");

}
BOOST_AUTO_TEST_CASE (generate_certtransfer)
{
	printf("Running generate_certtransfer...\n");
	GenerateBlocks(5, "node2");
	GenerateBlocks(5, "node3");
	AliasNew("node1", "jagcert1", "password", "changeddata1");
	AliasNew("node2", "jagcert2", "password", "changeddata2");
	AliasNew("node3", "jagcert3", "password", "changeddata3");
	string guid, pvtguid, certtitle, certdata;
	certtitle = "certtitle";
	certdata = "certdata";
	guid = CertNew("node1", "jagcert1", certtitle, certdata, "pubdata");
	// private cert
	pvtguid = CertNew("node1", "jagcert1", certtitle, certdata, "pubdata");
	CertUpdate("node1", pvtguid, "jagcert1", certtitle, certdata, "pub3");
	UniValue r;
	CertTransfer("node1", guid, "jagcert2");
	CertTransfer("node1", pvtguid, "jagcert3");
	// it got xferred to right person
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "certinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "true");
	BOOST_CHECK(find_value(r.get_obj(), "alias").get_str() == "jagcert2");
	BOOST_CHECK(find_value(r.get_obj(), "data").get_str() == certdata);

	BOOST_CHECK_NO_THROW(r = CallRPC("node3", "certinfo " + pvtguid));
	BOOST_CHECK(find_value(r.get_obj(), "ismine").get_str() == "true");
	BOOST_CHECK(find_value(r.get_obj(), "alias").get_str() == "jagcert3");
	BOOST_CHECK(find_value(r.get_obj(), "data").get_str() == certdata);
	// xfer an cert that isn't yours
	BOOST_CHECK_THROW(CallRPC("node1", "certtransfer " + guid + " jagcert2"), runtime_error);

	// update xferred cert
	certdata = "newdata";
	certtitle = "newtitle";
	CertUpdate("node2", guid, "jagcert2", certtitle, certdata, "public");

	// retransfer cert
	CertTransfer("node2", guid, "jagcert3");
}
BOOST_AUTO_TEST_CASE (generate_certsafesearch)
{
	printf("Running generate_certsafesearch...\n");
	UniValue r;
	GenerateBlocks(1);
	AliasNew("node1", "jagsafesearch1", "password", "changeddata1");
	// cert is safe to search
	string certguidsafe = CertNew("node1", "jagsafesearch1", "certtitle", "certdata", "public", "Yes");
	// not safe to search
	string certguidnotsafe = CertNew("node1", "jagsafesearch1", "certtitle", "certdata", "public", "No");
	// should include result in both safe search mode on and off
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "On"), true);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "Off"), true);

	// should only show up if safe search is off
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "On"), false);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "Off"), true);

	// shouldn't affect certinfo
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certguidsafe));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certguidnotsafe));

	// reverse the rolls
	CertUpdate("node1", certguidsafe, "jagsafesearch1", "certtitle", "certdata", "pub", "No");
	CertUpdate("node1", certguidnotsafe,  "jagsafesearch1", "certtitle", "certdata", "pub1", "Yes");

	// should include result in both safe search mode on and off
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "Off"), true);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "On"), false);

	// should only show up if safe search is off
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "Off"), true);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "On"), true);

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
	string certguidsafe = CertNew("node1", "jagsafesearch1", "certtitle", "certdata", "pub", "Yes");
	// not safe to search
	string certguidnotsafe = CertNew("node1", "jagsafesearch1", "certtitle", "certdata", "pub", "No");
	// can't ban on any other node than one that created sysban
	BOOST_CHECK_THROW(CertBan("node2",certguidnotsafe,SAFETY_LEVEL1), runtime_error);
	BOOST_CHECK_THROW(CertBan("node3",certguidsafe,SAFETY_LEVEL1), runtime_error);
	// ban both certs level 1 (only owner of syscategory can do this)
	BOOST_CHECK_NO_THROW(CertBan("node1",certguidsafe,SAFETY_LEVEL1));
	BOOST_CHECK_NO_THROW(CertBan("node1",certguidnotsafe,SAFETY_LEVEL1));
	// should only show level 1 banned if safe search filter is not used
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "On"), false);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "Off"), true);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "On"), false);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "Off"), true);
	// should be able to certinfo on level 1 banned certs
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certguidsafe));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certguidnotsafe));
	
	// ban both certs level 2 (only owner of syscategory can do this)
	BOOST_CHECK_NO_THROW(CertBan("node1",certguidsafe,SAFETY_LEVEL2));
	BOOST_CHECK_NO_THROW(CertBan("node1",certguidnotsafe,SAFETY_LEVEL2));
	// no matter what filter won't show banned certs
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "On"), false);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "Off"), false);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "On"), false);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "Off"), false);

	// shouldn't be able to certinfo on level 2 banned certs
	BOOST_CHECK_THROW(r = CallRPC("node1", "certinfo " + certguidsafe), runtime_error);
	BOOST_CHECK_THROW(r = CallRPC("node1", "certinfo " + certguidnotsafe), runtime_error);

	// unban both certs (only owner of syscategory can do this)
	BOOST_CHECK_NO_THROW(CertBan("node1",certguidsafe,0));
	BOOST_CHECK_NO_THROW(CertBan("node1",certguidnotsafe,0));
	// safe to search regardless of filter
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "On"), true);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidsafe, "Off"), true);

	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "On"), false);
	BOOST_CHECK_EQUAL(CertFilter("node1", certguidnotsafe, "Off"), true);

	// should be able to certinfo on non banned certs
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certguidsafe));
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + certguidnotsafe));
	
}

BOOST_AUTO_TEST_CASE (generate_certpruning)
{
	UniValue r;
	// makes sure services expire in 100 blocks instead of 1 year of blocks for testing purposes
	printf("Running generate_certpruning...\n");
	AliasNew("node1", "jagprune1", "password", "changeddata1");
	// stop node2 create a service,  mine some blocks to expire the service, when we restart the node the service data won't be synced with node2
	StopNode("node2");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certnew jagprune1 jag1 data pub"));
	const UniValue &arr = r.get_array();
	string guid = arr[1].get_str();
	GenerateBlocks(5, "node1");
	// we can find it as normal first
	BOOST_CHECK_EQUAL(CertFilter("node1", guid, "Off"), true);
	// make sure our offer alias doesn't expire
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg jagprune1 newdata privdata"));
	GenerateBlocks(5, "node1");
	ExpireAlias("jagprune1");
	StartNode("node2");
	ExpireAlias("jagprune1");
	GenerateBlocks(5, "node2");

	BOOST_CHECK_EQUAL(CertFilter("node1", guid, "Off"), false);

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + guid));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_int(), 1);	

	// should be pruned
	BOOST_CHECK_THROW(CallRPC("node2", "offerinfo " + guid), runtime_error);

	// stop node3
	StopNode("node3");
	// should fail: already expired alias
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdate sysrates.peg jagprune1 newdata privdata"), runtime_error);
	GenerateBlocks(5, "node1");
	// create a new service
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasnew sysrates.peg jagprune1 password1 temp data"));
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certnew jagprune1 jag1 data pub"));
	const UniValue &arr1 = r.get_array();
	string guid1 = arr1[1].get_str();
	GenerateBlocks(5, "node1");
	// stop and start node1
	StopNode("node1");
	StartNode("node1");
	GenerateBlocks(5, "node1");
	// ensure you can still update before expiry
	BOOST_CHECK_NO_THROW(CallRPC("node1", "certupdate " + guid1 + " jagprune1 newdata privdata pubdata1"));
	GenerateBlocks(5, "node1");
	// you can search it still on node1/node2
	BOOST_CHECK_EQUAL(CertFilter("node1", guid1, "Off"), true);
	BOOST_CHECK_EQUAL(CertFilter("node2", guid1, "Off"), true);
	// make sure our offer alias doesn't expire
	BOOST_CHECK_NO_THROW(CallRPC("node1", "aliasupdate sysrates.peg jagprune1 newdata privdata"));
	GenerateBlocks(5, "node1");
	ExpireAlias("jagprune1");
	// now it should be expired
	BOOST_CHECK_THROW(CallRPC("node1",  "certupdate " + guid1 + " jagprune1 newdata1 privdata1 pubdata3"), runtime_error);
	GenerateBlocks(5, "node1");
	BOOST_CHECK_EQUAL(CertFilter("node1", guid1, "Off"), false);
	BOOST_CHECK_EQUAL(CertFilter("node2", guid1, "Off"), false);
	// and it should say its expired
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "certinfo " + guid1));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_int(), 1);	
	GenerateBlocks(5, "node1");
	StartNode("node3");
	ExpireAlias("jagprune1");
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "certinfo " + guid1), runtime_error);
	BOOST_CHECK_EQUAL(CertFilter("node3", guid1, "Off"), false);
}
BOOST_AUTO_TEST_SUITE_END ()