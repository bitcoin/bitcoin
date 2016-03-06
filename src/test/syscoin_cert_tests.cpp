#include "test/test_syscoin_services.h"
#include "utiltime.h"
#include "rpcserver.h"
#include <boost/test/unit_test.hpp>
BOOST_FIXTURE_TEST_SUITE (syscoin_cert_tests, BasicSyscoinTestingSetup)

BOOST_AUTO_TEST_CASE (generate_big_certdata)
{
	GenerateBlocks(200);
	AliasNew("node1", "jagcertbig1", "data");
	// 1023 bytes long
	string gooddata = "asdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsd";
	// 1024 bytes long
	string baddata =  "asdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsdz";
	string guid = CertNew("node1", "jagcertbig1", "jag", gooddata);
	// unencrypted we are allowed up to 1108 bytes
	BOOST_CHECK_NO_THROW(CallRPC("node1", "certnew jagcertbig1 jag1 " + baddata + " 0"));
	// but unencrypted 1024 bytes should cause us to trip 1108 bytes once encrypted
	BOOST_CHECK_THROW(CallRPC("node1", "certnew jagcertbig1 jag1 " + baddata + " 1"), runtime_error);
	// update cert with long data, public (good) vs private (bad)
	BOOST_CHECK_NO_THROW(CallRPC("node1", "certupdate " + guid + " jag1 " + baddata + " 0"));
	// trying to update the public cert to a private one with 1024 bytes should fail aswell
	BOOST_CHECK_THROW(CallRPC("node1", "certupdate " + guid + " jag1 " + baddata + " 1"), runtime_error);

}
BOOST_AUTO_TEST_CASE (generate_big_certtitle)
{
	GenerateBlocks(50);
	AliasNew("node1", "jagcertbig2", "data");
	// 255 bytes long
	string goodtitle = "SfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	// 1023 bytes long
	string gooddata = "asdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfasdfasdfsadfsadassdsfsdfsdfsdfsdfsdsdfssdsfsdfsdfsdfsdfsdsdfdfsdfsdfsdfsd";	
	// 256 bytes long
	string badtitle =   "SfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddz";
	CertNew("node1", "jagcertbig2", goodtitle, "a");
	BOOST_CHECK_THROW(CallRPC("node1", "certnew jagcertbig2 " + badtitle + " 3d"), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_certupdate)
{
	AliasNew("node1", "jagcertupdate", "data");
	string guid = CertNew("node1", "jagcertupdate", "title", "data");
	// update an cert that isn't yours
	BOOST_CHECK_THROW(CallRPC("node2", "certupdate " + guid + " title data 0"), runtime_error);
	CertUpdate("node1", guid, "changedtitle", "changeddata");
	// shouldnt update data, just uses prev data because it hasnt changed
	CertUpdate("node1", guid, "changedtitle", "changeddata");

}
BOOST_AUTO_TEST_CASE (generate_certtransfer)
{
	GenerateBlocks(200, "node2");
	GenerateBlocks(200, "node3");
	AliasNew("node1", "jagcert1", "changeddata1");
	AliasNew("node2", "jagcert2", "changeddata2");
	AliasNew("node3", "jagcert3", "changeddata3");
	string guid, pvtguid, certtitle, certdata;
	certtitle = "certtitle";
	certdata = "certdata";
	guid = CertNew("node1", "jagcert1", certtitle, certdata);
	// private cert
	pvtguid = CertNew("node1", "jagcert1", certtitle, certdata);
	CertUpdate("node1", pvtguid, certtitle, certdata, true);
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
	CertUpdate("node2", guid, certtitle, certdata);

	// retransfer cert
	CertTransfer("node2", guid, "jagcert3");
}
BOOST_AUTO_TEST_SUITE_END ()