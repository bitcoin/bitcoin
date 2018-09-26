// Copyright (c) 2016-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
	// 512 bytes long
	string gooddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddSfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	// 513 bytes long
	string baddata = gooddata + "a";
	string guid = CertNew("node1", "jagcertbig1", "title", gooddata);
	BOOST_CHECK_THROW(CallRPC("node1", "certnew jagcertbig1 title " + baddata + " certificates ''"), runtime_error);
	// update cert with long pub data
	BOOST_CHECK_THROW(CallRPC("node1", "certupdate " + guid + " title  " + baddata + " certificates ''"), runtime_error);

}
BOOST_AUTO_TEST_CASE (generate_big_certtitle)
{
	printf("Running generate_big_certtitle...\n");
	GenerateBlocks(5);
	AliasNew("node1", "jagcertbig2", "data");
	// 256 bytes long
	string goodtitle = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";
	// 512 bytes long
	string gooddata = "SfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfddSfsddfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsfDsdsdsdsfsfsdsfsdsfdsfsdsfdsfsdsfsdSfsdfdfsdsfSfsdfdfsdsDfdfdd";	
	// 257 bytes long
	string badtitle = goodtitle + "a";
	CertNew("node1", "jagcertbig2", goodtitle, gooddata);
	BOOST_CHECK_THROW(CallRPC("node1", "certnew jagcertbig2 " + badtitle + " pub certificates ''"), runtime_error);
}
BOOST_AUTO_TEST_CASE (generate_certupdate)
{
	printf("Running generate_certupdate...\n");
	AliasNew("node1", "jagcertupdate", "data");
	string guid = CertNew("node1", "jagcertupdate", "title", "data");
	// update an cert that isn't yours
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "certupdate " + guid + " title pubdata certificates ''"));
	UniValue arr = r.get_array();
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "signrawtransaction " + arr[0].get_str()));
	BOOST_CHECK(!find_value(r.get_obj(), "complete").get_bool());

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
	guid = CertNew("node1", "jagcert1", certtitle, "pubdata");
	// private cert
	pvtguid = CertNew("node1", "jagcert1", certtitle, "pubdata");
	CertUpdate("node1", pvtguid, certtitle, "pub3");
	UniValue r;
	CertTransfer("node1", "node2", guid, "jagcert2");
	CertTransfer("node1", "node3", pvtguid, "jagcert3");

	// xfer an cert that isn't yours
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certtransfer " + guid + " jagcert2 '' 2 ''"));
	UniValue arr = r.get_array();
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "signrawtransaction " + arr[0].get_str()));
	BOOST_CHECK(!find_value(r.get_obj(), "complete").get_bool());
	// update xferred cert
	certdata = "newdata";
	certtitle = "newtitle";
	CertUpdate("node2", guid, certtitle, "public");

	// retransfer cert
	CertTransfer("node2","node3", guid, "jagcert3");
}
BOOST_AUTO_TEST_CASE (generate_certpruning)
{
	UniValue r;
	// makes sure services expire in 100 blocks instead of 1 year of blocks for testing purposes
	printf("Running generate_certpruning...\n");
	AliasNew("node1", "jagprune1", "changeddata1");
	// stop node2 create a service,  mine some blocks to expire the service, when we restart the node the service data won't be synced with node2
	StopNode("node2");
	string guid = CertNew("node1", "jagprune1", "jag1", "pubdata");
	// we can find it as normal first
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + guid));
	// make sure our offer alias doesn't expire
	string hex_str = AliasUpdate("node1", "jagprune1");
	BOOST_CHECK(hex_str.empty());
	GenerateBlocks(5, "node1");
	ExpireAlias("jagprune1");
	StartNode("node2");
	GenerateBlocks(5, "node2");

	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + guid));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), true);

	// should be pruned
	BOOST_CHECK_THROW(CallRPC("node2", "certinfo " + guid), runtime_error);

	// stop node3
	StopNode("node3");
	// should fail: already expired alias
	BOOST_CHECK_THROW(CallRPC("node1", "aliasupdate jagprune1 newdata TTVgyEvCfgZFiVL32kD7jMRaBKtGCHqwbD 3 0 '' '' ''"), runtime_error);
	GenerateBlocks(5, "node1");

	// stop and start node1
	StopNode("node1");
	StartNode("node1");
	GenerateBlocks(5, "node1");

	BOOST_CHECK_THROW(CallRPC("node1", "certinfo " + guid), runtime_error);

	// create a new service
	AliasNew("node1", "jagprune1", "temp");
	string guid1 = CertNew("node1", "jagprune1", "title", "pubdata");
	// ensure you can still update before expiry
	CertUpdate("node1", guid1, "pubdata1");
	// you can search it still on node1/node2
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + guid1));
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "certinfo " + guid1));
	// make sure our offer alias doesn't expire
	hex_str = AliasUpdate("node1", "jagprune1");
	BOOST_CHECK(hex_str.empty());
	GenerateBlocks(5, "node1");
	ExpireAlias("jagprune1");
	// now it should be expired
	BOOST_CHECK_THROW(CallRPC("node1",  "certupdate " + guid1 + " title pubdata3 certificates ''"), runtime_error);
	GenerateBlocks(5, "node1");
	BOOST_CHECK_NO_THROW(r = CallRPC("node1", "certinfo " + guid1));
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "certinfo " + guid1));
	// and it should say its expired
	BOOST_CHECK_NO_THROW(r = CallRPC("node2", "certinfo " + guid1));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), true);
	GenerateBlocks(5, "node1");
	StartNode("node3");
	GenerateBlocks(5, "node3");
	// node3 shouldn't find the service at all (meaning node3 doesn't sync the data)
	BOOST_CHECK_THROW(CallRPC("node3", "certinfo " + guid1), runtime_error);
}
BOOST_AUTO_TEST_SUITE_END ()