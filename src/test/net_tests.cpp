// Copyright (c) 2012-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "addrman.h"
#include "test/test_bitcoin.h"
#include <string>
#include <boost/test/unit_test.hpp>
#include "hash.h"
#include "serialize.h"
#include "streams.h"
#include "net.h"
#include "chainparams.h"

using namespace std;

class CAddrManSerializationMock : public CAddrMan
{
public:
    virtual void Serialize(CDataStream& s, int nType, int nVersionDummy) const = 0;

    //! Ensure that bucket placement is always the same for testing purposes.
    void MakeDeterministic()
    {
        nKey.SetNull();
        seed_insecure_rand(true);
    }
};

class CAddrManUncorrupted : public CAddrManSerializationMock
{
public:
    void Serialize(CDataStream& s, int nType, int nVersionDummy) const
    {
        CAddrMan::Serialize(s, nType, nVersionDummy);
    }
};

class CAddrManCorrupted : public CAddrManSerializationMock
{
public:
    void Serialize(CDataStream& s, int nType, int nVersionDummy) const
    {
        // Produces corrupt output that claims addrman has 20 addrs when it only has one addr.
        unsigned char nVersion = 1;
        s << nVersion;
        s << uint8_t(32);
        s << nKey;
        s << 10; // nNew
        s << 10; // nTried

        int nUBuckets = ADDRMAN_NEW_BUCKET_COUNT ^ (1 << 30);
        s << nUBuckets;

        CAddress addr = CAddress(CService("252.1.1.1", 7777));
        CAddrInfo info = CAddrInfo(addr, CNetAddr("252.2.2.2"));
        s << info;
    }
};

CDataStream AddrmanToStream(CAddrManSerializationMock& addrman)
{
    CDataStream ssPeersIn(SER_DISK, CLIENT_VERSION);
    ssPeersIn << FLATDATA(Params().MessageStart());
    ssPeersIn << addrman;
    std::string str = ssPeersIn.str();
    std::vector<uint8_t> vchData(str.begin(), str.end());
    return CDataStream(vchData, SER_DISK, CLIENT_VERSION);
}

BOOST_FIXTURE_TEST_SUITE(net_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(caddrdb_read)
{
    CAddrManUncorrupted addrmanUncorrupted;
    addrmanUncorrupted.MakeDeterministic();

    CService addr1 = CService("250.7.1.1", 8333);
    CService addr2 = CService("250.7.2.2", 9999);
    CService addr3 = CService("250.7.3.3", 9999);

    // Add three addresses to new table.
    addrmanUncorrupted.Add(CAddress(addr1), CService("252.5.1.1", 8333));
    addrmanUncorrupted.Add(CAddress(addr2), CService("252.5.1.1", 8333));
    addrmanUncorrupted.Add(CAddress(addr3), CService("252.5.1.1", 8333));

    // Test that the de-serialization does not throw an exception.
    CDataStream ssPeers1 = AddrmanToStream(addrmanUncorrupted);
    bool exceptionThrown = false;
    CAddrMan addrman1;

    BOOST_CHECK(addrman1.size() == 0);
    try {
        uint8_t pchMsgTmp[4];
        ssPeers1 >> FLATDATA(pchMsgTmp);
        ssPeers1 >> addrman1;
    } catch (const std::exception& e) {
        exceptionThrown = true;
    }

    BOOST_CHECK(addrman1.size() == 3);
    BOOST_CHECK(exceptionThrown == false);

    // Test that CAddrDB::Read creates an addrman with the correct number of addrs.
    CDataStream ssPeers2 = AddrmanToStream(addrmanUncorrupted);

    CAddrMan addrman2;
    CAddrDB adb;
    BOOST_CHECK(addrman2.size() == 0);
    adb.Read(addrman2, ssPeers2);
    BOOST_CHECK(addrman2.size() == 3);
}


BOOST_AUTO_TEST_CASE(caddrdb_read_corrupted)
{
    CAddrManCorrupted addrmanCorrupted;
    addrmanCorrupted.MakeDeterministic();

    // Test that the de-serialization of corrupted addrman throws an exception.
    CDataStream ssPeers1 = AddrmanToStream(addrmanCorrupted);
    bool exceptionThrown = false;
    CAddrMan addrman1;
    BOOST_CHECK(addrman1.size() == 0);
    try {
        uint8_t pchMsgTmp[4];
        ssPeers1 >> FLATDATA(pchMsgTmp);
        ssPeers1 >> addrman1;
    } catch (const std::exception& e) {
        exceptionThrown = true;
    }
    // Even through de-serialization failed addrman is not left in a clean state.
    BOOST_CHECK(addrman1.size() == 1);
    BOOST_CHECK(exceptionThrown);

    // Test that CAddrDB::Read leaves addrman in a clean state if de-serialization fails.
    CDataStream ssPeers2 = AddrmanToStream(addrmanCorrupted);

    CAddrMan addrman2;
    CAddrDB adb;
    BOOST_CHECK(addrman2.size() == 0);
    adb.Read(addrman2, ssPeers2);
    BOOST_CHECK(addrman2.size() == 0);
}

BOOST_AUTO_TEST_CASE(cnode_simple_test)
{
    SOCKET hSocket = INVALID_SOCKET;

    in_addr ipv4Addr;
    ipv4Addr.s_addr = 0xa0b0c001;

    CAddress addr = CAddress(CService(ipv4Addr, 7777), NODE_NETWORK);
    std::string pszDest = "";
    bool fInboundIn = false;

    // Test that fFeeler is false by default.
    std::unique_ptr<CNode> pnode1(new CNode(hSocket, addr, pszDest, fInboundIn));
    BOOST_CHECK(pnode1->fInbound == false);
    BOOST_CHECK(pnode1->fFeeler == false);

    fInboundIn = true;
    std::unique_ptr<CNode> pnode2(new CNode(hSocket, addr, pszDest, fInboundIn));
    BOOST_CHECK(pnode2->fInbound == true);
    BOOST_CHECK(pnode2->fFeeler == false);

    // NodeRef checks and refcount checks.
    BOOST_CHECK_EQUAL(pnode1->nRefCount, 0);

    // Check null pointers are good
    {
        CNodeRef ref;       // Default constructor
        BOOST_CHECK(!ref);  // operator bool
        ref = 0;
        BOOST_CHECK(!ref);
    }

    // get()
    {
        CNodeRef ref1(pnode1.get());
        CNodeRef ref2;
        BOOST_CHECK(ref1.get() == pnode1.get());
        BOOST_CHECK(ref2.get() == nullptr);
    }

    // Plain constructor and copy constructor
    {
        CNodeRef ref1(pnode1.get());
        BOOST_CHECK_EQUAL(pnode1->nRefCount, 1);

        {
            CNodeRef ref2(ref1);
            BOOST_CHECK_EQUAL(pnode1->nRefCount, 2);
        }

        BOOST_CHECK_EQUAL(pnode1->nRefCount, 1);
    }
    BOOST_CHECK_EQUAL(pnode1->nRefCount, 0);

    // Assignment operator
    {
        CNodeRef ref1;

        ref1 = pnode1.get();
        BOOST_CHECK_EQUAL(pnode1->nRefCount, 1);
        ref1 = ref1;
        BOOST_CHECK_EQUAL(pnode1->nRefCount, 1);
        ref1 = nullptr;
        BOOST_CHECK_EQUAL(pnode1->nRefCount, 0);
    }
    BOOST_CHECK_EQUAL(pnode1->nRefCount, 0);
}

BOOST_AUTO_TEST_SUITE_END()
