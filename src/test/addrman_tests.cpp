// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <addrman.h>
#include <test/test_dash.h>
#include <string>
#include <boost/test/unit_test.hpp>
#include <util/asmap.h>
#include <test/data/asmap.raw.h>

#include <hash.h>
#include <netbase.h>
#include <random.h>

class CAddrManTest : public CAddrMan
{
private:
    bool deterministic;
    uint64_t state;

public:
    explicit CAddrManTest(bool makeDeterministic = true,
        std::vector<bool> asmap = std::vector<bool>())
    {
        state = 1;

        if (makeDeterministic) {
            //  Set addrman addr placement to be deterministic.
            MakeDeterministic();
        }
        deterministic = makeDeterministic;
        m_asmap = asmap;
    }

    //! Ensure that bucket placement is always the same for testing purposes.
    void MakeDeterministic()
    {
        nKey.SetNull();
        insecure_rand = FastRandomContext(true);
    }

    int RandomInt(int nMax) override
    {
        state = (CHashWriter(SER_GETHASH, 0) << state).GetHash().GetCheapHash();
        return (unsigned int)(state % nMax);
    }

    CAddrInfo* Find(const CService& addr, int* pnId = nullptr)
    {
        LOCK(cs);
        return CAddrMan::Find(addr, pnId);
    }

    CAddrInfo* Create(const CAddress& addr, const CNetAddr& addrSource, int* pnId = nullptr)
    {
        LOCK(cs);
        return CAddrMan::Create(addr, addrSource, pnId);
    }

    void Delete(int nId)
    {
        LOCK(cs);
        CAddrMan::Delete(nId);
    }

    // Used to test deserialization
    std::pair<int, int> GetBucketAndEntry(const CAddress& addr)
    {
        LOCK(cs);
        int nId = mapAddr[addr];
        for (int bucket = 0; bucket < ADDRMAN_NEW_BUCKET_COUNT; ++bucket) {
            for (int entry = 0; entry < ADDRMAN_BUCKET_SIZE; ++entry) {
                if (nId == vvNew[bucket][entry]) {
                    return std::pair<int, int>(bucket, entry);
                }
            }
        }
        return std::pair<int, int>(-1, -1);
    }

    // Simulates connection failure so that we can test eviction of offline nodes
    void SimConnFail(CService& addr)
    {
         LOCK(cs);
         int64_t nLastSuccess = 1;
         Good_(addr, true, nLastSuccess); // Set last good connection in the deep past.

         bool count_failure = false;
         int64_t nLastTry = GetAdjustedTime()-61;
         Attempt(addr, count_failure, nLastTry);
     }

    void Clear()
    {
        CAddrMan::Clear();
        if (deterministic) {
            nKey.SetNull();
            insecure_rand = FastRandomContext(true);
        }
    }

};

static CNetAddr ResolveIP(const char* ip)
{
    CNetAddr addr;
    BOOST_CHECK_MESSAGE(LookupHost(ip, addr, false), strprintf("failed to resolve: %s", ip));
    return addr;
}

static CNetAddr ResolveIP(std::string ip)
{
    return ResolveIP(ip.c_str());
}

static CService ResolveService(const char* ip, int port = 0)
{
    CService serv;
    BOOST_CHECK_MESSAGE(Lookup(ip, serv, port, false), strprintf("failed to resolve: %s:%i", ip, port));
    return serv;
}

static CService ResolveService(std::string ip, int port = 0)
{
    return ResolveService(ip.c_str(), port);
}

static std::vector<bool> FromBytes(const unsigned char* source, int vector_size) {
    std::vector<bool> result(vector_size);
    for (int byte_i = 0; byte_i < vector_size / 8; ++byte_i) {
        unsigned char cur_byte = source[byte_i];
        for (int bit_i = 0; bit_i < 8; ++bit_i) {
            result[byte_i * 8 + bit_i] = (cur_byte >> bit_i) & 1;
        }
    }
    return result;
}


BOOST_FIXTURE_TEST_SUITE(addrman_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(addrman_simple)
{
    CAddrManTest addrman;

    CNetAddr source = ResolveIP("252.2.2.2");

    // Test: Does Addrman respond correctly when empty.
    BOOST_CHECK_EQUAL(addrman.size(), 0U);
    CAddrInfo addr_null = addrman.Select();
    BOOST_CHECK_EQUAL(addr_null.ToString(), "[::]:0");

    // Test: Does Addrman::Add work as expected.
    CService addr1 = ResolveService("250.1.1.1", 8333);
    BOOST_CHECK(addrman.Add(CAddress(addr1, NODE_NONE), source));
    BOOST_CHECK_EQUAL(addrman.size(), 1U);
    CAddrInfo addr_ret1 = addrman.Select();
    BOOST_CHECK_EQUAL(addr_ret1.ToString(), "250.1.1.1:8333");

    // Test: Does IP address deduplication work correctly.
    //  Expected dup IP should not be added.
    CService addr1_dup = ResolveService("250.1.1.1", 8333);
    BOOST_CHECK(!addrman.Add(CAddress(addr1_dup, NODE_NONE), source));
    BOOST_CHECK_EQUAL(addrman.size(), 1U);


    // Test: New table has one addr and we add a diff addr we should
    //  have at least one addr.
    // Note that addrman's size cannot be tested reliably after insertion, as
    // hash collisions may occur. But we can always be sure of at least one
    // success.

    CService addr2 = ResolveService("250.1.1.2", 8333);
    BOOST_CHECK(addrman.Add(CAddress(addr2, NODE_NONE), source));
    BOOST_CHECK(addrman.size() >= 1);

    // Test: AddrMan::Clear() should empty the new table.
    addrman.Clear();
    BOOST_CHECK_EQUAL(addrman.size(), 0U);
    CAddrInfo addr_null2 = addrman.Select();
    BOOST_CHECK_EQUAL(addr_null2.ToString(), "[::]:0");

    // Test: AddrMan::Add multiple addresses works as expected
    std::vector<CAddress> vAddr;
    vAddr.push_back(CAddress(ResolveService("250.1.1.3", 8333), NODE_NONE));
    vAddr.push_back(CAddress(ResolveService("250.1.1.4", 8333), NODE_NONE));
    BOOST_CHECK(addrman.Add(vAddr, source));
    BOOST_CHECK(addrman.size() >= 1);
}

BOOST_AUTO_TEST_CASE(addrman_ports)
{
    CAddrManTest addrman;

    CNetAddr source = ResolveIP("252.2.2.2");

    BOOST_CHECK_EQUAL(addrman.size(), 0U);

    // Test 7; Addr with same IP but diff port does not replace existing addr.
    CService addr1 = ResolveService("250.1.1.1", 8333);
    addrman.Add(CAddress(addr1, NODE_NONE), source);
    BOOST_CHECK_EQUAL(addrman.size(), 1U);

    CService addr1_port = ResolveService("250.1.1.1", 8334);
    addrman.Add(CAddress(addr1_port, NODE_NONE), source);
    BOOST_CHECK_EQUAL(addrman.size(), 1U);
    CAddrInfo addr_ret2 = addrman.Select();
    BOOST_CHECK_EQUAL(addr_ret2.ToString(), "250.1.1.1:8333");

    // Test: Add same IP but diff port to tried table, it doesn't get added.
    //  Perhaps this is not ideal behavior but it is the current behavior.
    addrman.Good(CAddress(addr1_port, NODE_NONE));
    BOOST_CHECK_EQUAL(addrman.size(), 1U);
    bool newOnly = true;
    CAddrInfo addr_ret3 = addrman.Select(newOnly);
    BOOST_CHECK_EQUAL(addr_ret3.ToString(), "250.1.1.1:8333");
}


BOOST_AUTO_TEST_CASE(addrman_select)
{
    CAddrManTest addrman;

    CNetAddr source = ResolveIP("252.2.2.2");

    // Test: Select from new with 1 addr in new.
    CService addr1 = ResolveService("250.1.1.1", 8333);
    addrman.Add(CAddress(addr1, NODE_NONE), source);
    BOOST_CHECK_EQUAL(addrman.size(), 1U);

    bool newOnly = true;
    CAddrInfo addr_ret1 = addrman.Select(newOnly);
    BOOST_CHECK_EQUAL(addr_ret1.ToString(), "250.1.1.1:8333");

    // Test: move addr to tried, select from new expected nothing returned.
    addrman.Good(CAddress(addr1, NODE_NONE));
    BOOST_CHECK_EQUAL(addrman.size(), 1U);
    CAddrInfo addr_ret2 = addrman.Select(newOnly);
    BOOST_CHECK_EQUAL(addr_ret2.ToString(), "[::]:0");

    CAddrInfo addr_ret3 = addrman.Select();
    BOOST_CHECK_EQUAL(addr_ret3.ToString(), "250.1.1.1:8333");

    BOOST_CHECK_EQUAL(addrman.size(), 1U);


    // Add three addresses to new table.
    CService addr2 = ResolveService("250.3.1.1", 8333);
    CService addr3 = ResolveService("250.3.2.2", 9999);
    CService addr4 = ResolveService("250.3.3.3", 9999);

    addrman.Add(CAddress(addr2, NODE_NONE), ResolveService("250.3.1.1", 8333));
    addrman.Add(CAddress(addr3, NODE_NONE), ResolveService("250.3.1.1", 8333));
    addrman.Add(CAddress(addr4, NODE_NONE), ResolveService("250.4.1.1", 8333));

    // Add three addresses to tried table.
    CService addr5 = ResolveService("250.4.4.4", 8333);
    CService addr6 = ResolveService("250.4.5.5", 7777);
    CService addr7 = ResolveService("250.4.6.6", 8333);

    addrman.Add(CAddress(addr5, NODE_NONE), ResolveService("250.3.1.1", 8333));
    addrman.Good(CAddress(addr5, NODE_NONE));
    addrman.Add(CAddress(addr6, NODE_NONE), ResolveService("250.3.1.1", 8333));
    addrman.Good(CAddress(addr6, NODE_NONE));
    addrman.Add(CAddress(addr7, NODE_NONE), ResolveService("250.1.1.3", 8333));
    addrman.Good(CAddress(addr7, NODE_NONE));

    // Test: 6 addrs + 1 addr from last test = 7.
    BOOST_CHECK_EQUAL(addrman.size(), 7U);

    // Test: Select pulls from new and tried regardless of port number.
    std::set<uint16_t> ports;
    for (int i = 0; i < 20; ++i) {
        ports.insert(addrman.Select().GetPort());
    }
    BOOST_CHECK_EQUAL(ports.size(), 3U);
}

BOOST_AUTO_TEST_CASE(addrman_new_collisions)
{
    CAddrManTest addrman;

    CNetAddr source = ResolveIP("252.2.2.2");

    BOOST_CHECK_EQUAL(addrman.size(), 0U);

    for (unsigned int i = 1; i < 18; i++) {
        CService addr = ResolveService("250.1.1." + std::to_string(i));
        addrman.Add(CAddress(addr, NODE_NONE), source);

        //Test: No collision in new table yet.
        BOOST_CHECK_EQUAL(addrman.size(), i);
    }

    //Test: new table collision!
    CService addr1 = ResolveService("250.1.1.18");
    addrman.Add(CAddress(addr1, NODE_NONE), source);
    BOOST_CHECK_EQUAL(addrman.size(), 17U);

    CService addr2 = ResolveService("250.1.1.19");
    addrman.Add(CAddress(addr2, NODE_NONE), source);
    BOOST_CHECK_EQUAL(addrman.size(), 18U);
}

BOOST_AUTO_TEST_CASE(addrman_tried_collisions)
{
    CAddrManTest addrman;

    CNetAddr source = ResolveIP("252.2.2.2");

    BOOST_CHECK_EQUAL(addrman.size(), 0U);

    for (unsigned int i = 1; i < 80; i++) {
        CService addr = ResolveService("250.1.1." + std::to_string(i));
        addrman.Add(CAddress(addr, NODE_NONE), source);
        addrman.Good(CAddress(addr, NODE_NONE));

        //Test: No collision in tried table yet.
        BOOST_CHECK_EQUAL(addrman.size(), i);
    }

    //Test: tried table collision!
    CService addr1 = ResolveService("250.1.1.80");
    addrman.Add(CAddress(addr1, NODE_NONE), source);
    BOOST_CHECK_EQUAL(addrman.size(), 79U);

    CService addr2 = ResolveService("250.1.1.81");
    addrman.Add(CAddress(addr2, NODE_NONE), source);
    BOOST_CHECK_EQUAL(addrman.size(), 80U);
}

BOOST_AUTO_TEST_CASE(addrman_find)
{
    CAddrManTest addrman;

    BOOST_CHECK_EQUAL(addrman.size(), 0U);

    CAddress addr1 = CAddress(ResolveService("250.1.2.1", 8333), NODE_NONE);
    CAddress addr2 = CAddress(ResolveService("250.1.2.1", 9999), NODE_NONE);
    CAddress addr3 = CAddress(ResolveService("251.255.2.1", 8333), NODE_NONE);

    CNetAddr source1 = ResolveIP("250.1.2.1");
    CNetAddr source2 = ResolveIP("250.1.2.2");

    addrman.Add(addr1, source1);
    addrman.Add(addr2, source2);
    addrman.Add(addr3, source1);

    // Test: ensure Find returns an IP matching what we searched on.
    CAddrInfo* info1 = addrman.Find(addr1);
    BOOST_REQUIRE(info1);
    BOOST_CHECK_EQUAL(info1->ToString(), "250.1.2.1:8333");

    // Test 18; Find does not discriminate by port number.
    CAddrInfo* info2 = addrman.Find(addr2);
    BOOST_REQUIRE(info2);
    BOOST_CHECK_EQUAL(info2->ToString(), info1->ToString());

    // Test: Find returns another IP matching what we searched on.
    CAddrInfo* info3 = addrman.Find(addr3);
    BOOST_REQUIRE(info3);
    BOOST_CHECK_EQUAL(info3->ToString(), "251.255.2.1:8333");
}

BOOST_AUTO_TEST_CASE(addrman_create)
{
    CAddrManTest addrman;

    BOOST_CHECK_EQUAL(addrman.size(), 0U);

    CAddress addr1 = CAddress(ResolveService("250.1.2.1", 8333), NODE_NONE);
    CNetAddr source1 = ResolveIP("250.1.2.1");

    int nId;
    CAddrInfo* pinfo = addrman.Create(addr1, source1, &nId);

    // Test: The result should be the same as the input addr.
    BOOST_CHECK_EQUAL(pinfo->ToString(), "250.1.2.1:8333");

    CAddrInfo* info2 = addrman.Find(addr1);
    BOOST_CHECK_EQUAL(info2->ToString(), "250.1.2.1:8333");
}


BOOST_AUTO_TEST_CASE(addrman_delete)
{
    CAddrManTest addrman;

    BOOST_CHECK_EQUAL(addrman.size(), 0U);

    CAddress addr1 = CAddress(ResolveService("250.1.2.1", 8333), NODE_NONE);
    CNetAddr source1 = ResolveIP("250.1.2.1");

    int nId;
    addrman.Create(addr1, source1, &nId);

    // Test: Delete should actually delete the addr.
    BOOST_CHECK_EQUAL(addrman.size(), 1U);
    addrman.Delete(nId);
    BOOST_CHECK_EQUAL(addrman.size(), 0U);
    CAddrInfo* info2 = addrman.Find(addr1);
    BOOST_CHECK(info2 == nullptr);
}

BOOST_AUTO_TEST_CASE(addrman_getaddr)
{
    CAddrManTest addrman;

    // Test: Sanity check, GetAddr should never return anything if addrman
    //  is empty.
    BOOST_CHECK_EQUAL(addrman.size(), 0U);
    std::vector<CAddress> vAddr1 = addrman.GetAddr();
    BOOST_CHECK_EQUAL(vAddr1.size(), 0U);

    CAddress addr1 = CAddress(ResolveService("250.250.2.1", 8333), NODE_NONE);
    addr1.nTime = GetAdjustedTime(); // Set time so isTerrible = false
    CAddress addr2 = CAddress(ResolveService("250.251.2.2", 9999), NODE_NONE);
    addr2.nTime = GetAdjustedTime();
    CAddress addr3 = CAddress(ResolveService("251.252.2.3", 8333), NODE_NONE);
    addr3.nTime = GetAdjustedTime();
    CAddress addr4 = CAddress(ResolveService("252.253.3.4", 8333), NODE_NONE);
    addr4.nTime = GetAdjustedTime();
    CAddress addr5 = CAddress(ResolveService("252.254.4.5", 8333), NODE_NONE);
    addr5.nTime = GetAdjustedTime();
    CNetAddr source1 = ResolveIP("250.1.2.1");
    CNetAddr source2 = ResolveIP("250.2.3.3");

    // Test: Ensure GetAddr works with new addresses.
    addrman.Add(addr1, source1);
    addrman.Add(addr2, source2);
    addrman.Add(addr3, source1);
    addrman.Add(addr4, source2);
    addrman.Add(addr5, source1);

    // GetAddr returns 23% of addresses, 23% of 5 is 1 rounded down.
    BOOST_CHECK_EQUAL(addrman.GetAddr().size(), 1U);

    // Test: Ensure GetAddr works with new and tried addresses.
    addrman.Good(CAddress(addr1, NODE_NONE));
    addrman.Good(CAddress(addr2, NODE_NONE));
    BOOST_CHECK_EQUAL(addrman.GetAddr().size(), 1U);

    // Test: Ensure GetAddr still returns 23% when addrman has many addrs.
    for (unsigned int i = 1; i < (8 * 256); i++) {
        int octet1 = i % 256;
        int octet2 = i >> 8 % 256;
        std::string strAddr = std::to_string(octet1) + "." + std::to_string(octet2) + ".1.23";
        CAddress addr = CAddress(ResolveService(strAddr), NODE_NONE);

        // Ensure that for all addrs in addrman, isTerrible == false.
        addr.nTime = GetAdjustedTime();
        addrman.Add(addr, ResolveIP(strAddr));
        if (i % 8 == 0)
            addrman.Good(addr);
    }
    std::vector<CAddress> vAddr = addrman.GetAddr();

    size_t percent23 = (addrman.size() * 23) / 100;
    BOOST_CHECK_EQUAL(vAddr.size(), percent23);
    BOOST_CHECK_EQUAL(vAddr.size(), 461U);
    // (Addrman.size() < number of addresses added) due to address collisions.
    BOOST_CHECK_EQUAL(addrman.size(), 2006U);
}


BOOST_AUTO_TEST_CASE(caddrinfo_get_tried_bucket_legacy)
{
    CAddrManTest addrman;

    CAddress addr1 = CAddress(ResolveService("250.1.1.1", 8333), NODE_NONE);
    CAddress addr2 = CAddress(ResolveService("250.1.1.1", 9999), NODE_NONE);

    CNetAddr source1 = ResolveIP("250.1.1.1");


    CAddrInfo info1 = CAddrInfo(addr1, source1);

    uint256 nKey1 = (uint256)(CHashWriter(SER_GETHASH, 0) << 1).GetHash();
    uint256 nKey2 = (uint256)(CHashWriter(SER_GETHASH, 0) << 2).GetHash();

    std::vector<bool> asmap; // use /16

    BOOST_CHECK_EQUAL(info1.GetTriedBucket(nKey1, asmap), 40);

    // Test: Make sure key actually randomizes bucket placement. A fail on
    //  this test could be a security issue.
    BOOST_CHECK(info1.GetTriedBucket(nKey1, asmap) != info1.GetTriedBucket(nKey2, asmap));

    // Test: Two addresses with same IP but different ports can map to
    //  different buckets because they have different keys.
    CAddrInfo info2 = CAddrInfo(addr2, source1);

    BOOST_CHECK(info1.GetKey() != info2.GetKey());
    BOOST_CHECK(info1.GetTriedBucket(nKey1, asmap) != info2.GetTriedBucket(nKey1, asmap));

    std::set<int> buckets;
    for (int i = 0; i < 255; i++) {
        CAddrInfo infoi = CAddrInfo(
            CAddress(ResolveService("250.1.1." + std::to_string(i)), NODE_NONE),
            ResolveIP("250.1.1." + std::to_string(i)));
        int bucket = infoi.GetTriedBucket(nKey1, asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the same /16 prefix should
    // never get more than 8 buckets with legacy grouping
    BOOST_CHECK_EQUAL(buckets.size(), 8U);

    buckets.clear();
    for (int j = 0; j < 255; j++) {
        CAddrInfo infoj = CAddrInfo(
            CAddress(ResolveService("250." + std::to_string(j) + ".1.1"), NODE_NONE),
            ResolveIP("250." + std::to_string(j) + ".1.1"));
        int bucket = infoj.GetTriedBucket(nKey1, asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the different /16 prefix should map to more than
    // 8 buckets with legacy grouping
    BOOST_CHECK_EQUAL(buckets.size(), 160U);
}

BOOST_AUTO_TEST_CASE(caddrinfo_get_new_bucket_legacy)
{
    CAddrManTest addrman;

    CAddress addr1 = CAddress(ResolveService("250.1.2.1", 8333), NODE_NONE);
    CAddress addr2 = CAddress(ResolveService("250.1.2.1", 9999), NODE_NONE);

    CNetAddr source1 = ResolveIP("250.1.2.1");

    CAddrInfo info1 = CAddrInfo(addr1, source1);

    uint256 nKey1 = (uint256)(CHashWriter(SER_GETHASH, 0) << 1).GetHash();
    uint256 nKey2 = (uint256)(CHashWriter(SER_GETHASH, 0) << 2).GetHash();

    std::vector<bool> asmap; // use /16

    // Test: Make sure the buckets are what we expect
    BOOST_CHECK_EQUAL(info1.GetNewBucket(nKey1, asmap), 786);
    BOOST_CHECK_EQUAL(info1.GetNewBucket(nKey1, source1, asmap), 786);

    // Test: Make sure key actually randomizes bucket placement. A fail on
    //  this test could be a security issue.
    BOOST_CHECK(info1.GetNewBucket(nKey1, asmap) != info1.GetNewBucket(nKey2, asmap));

    // Test: Ports should not affect bucket placement in the addr
    CAddrInfo info2 = CAddrInfo(addr2, source1);
    BOOST_CHECK(info1.GetKey() != info2.GetKey());
    BOOST_CHECK_EQUAL(info1.GetNewBucket(nKey1, asmap), info2.GetNewBucket(nKey1, asmap));

    std::set<int> buckets;
    for (int i = 0; i < 255; i++) {
        CAddrInfo infoi = CAddrInfo(
            CAddress(ResolveService("250.1.1." + std::to_string(i)), NODE_NONE),
            ResolveIP("250.1.1." + std::to_string(i)));
        int bucket = infoi.GetNewBucket(nKey1, asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the same group (\16 prefix for IPv4) should
    //  always map to the same bucket.
    BOOST_CHECK_EQUAL(buckets.size(), 1U);

    buckets.clear();
    for (int j = 0; j < 4 * 255; j++) {
        CAddrInfo infoj = CAddrInfo(CAddress(
                                        ResolveService(
                                            std::to_string(250 + (j / 255)) + "." + std::to_string(j % 256) + ".1.1"), NODE_NONE),
            ResolveIP("251.4.1.1"));
        int bucket = infoj.GetNewBucket(nKey1, asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the same source groups should map to NO MORE
    //  than 64 buckets.
    BOOST_CHECK(buckets.size() <= 64);

    buckets.clear();
    for (int p = 0; p < 255; p++) {
        CAddrInfo infoj = CAddrInfo(
            CAddress(ResolveService("250.1.1.1"), NODE_NONE),
            ResolveIP("250." + std::to_string(p) + ".1.1"));
        int bucket = infoj.GetNewBucket(nKey1, asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the different source groups should map to MORE
    //  than 64 buckets.
    BOOST_CHECK(buckets.size() > 64);
}

// The following three test cases use asmap.raw
// We use an artificial minimal mock mapping
// 250.0.0.0/8 AS1000
// 101.1.0.0/16 AS1
// 101.2.0.0/16 AS2
// 101.3.0.0/16 AS3
// 101.4.0.0/16 AS4
// 101.5.0.0/16 AS5
// 101.6.0.0/16 AS6
// 101.7.0.0/16 AS7
// 101.8.0.0/16 AS8
BOOST_AUTO_TEST_CASE(caddrinfo_get_tried_bucket)
{
    CAddrManTest addrman;

    CAddress addr1 = CAddress(ResolveService("250.1.1.1", 8333), NODE_NONE);
    CAddress addr2 = CAddress(ResolveService("250.1.1.1", 9999), NODE_NONE);

    CNetAddr source1 = ResolveIP("250.1.1.1");


    CAddrInfo info1 = CAddrInfo(addr1, source1);

    uint256 nKey1 = (uint256)(CHashWriter(SER_GETHASH, 0) << 1).GetHash();
    uint256 nKey2 = (uint256)(CHashWriter(SER_GETHASH, 0) << 2).GetHash();

    std::vector<bool> asmap = FromBytes(raw_tests::asmap, sizeof(raw_tests::asmap) * 8);

    BOOST_CHECK_EQUAL(info1.GetTriedBucket(nKey1, asmap), 236);

    // Test: Make sure key actually randomizes bucket placement. A fail on
    //  this test could be a security issue.
    BOOST_CHECK(info1.GetTriedBucket(nKey1, asmap) != info1.GetTriedBucket(nKey2, asmap));

    // Test: Two addresses with same IP but different ports can map to
    //  different buckets because they have different keys.
    CAddrInfo info2 = CAddrInfo(addr2, source1);

    BOOST_CHECK(info1.GetKey() != info2.GetKey());
    BOOST_CHECK(info1.GetTriedBucket(nKey1, asmap) != info2.GetTriedBucket(nKey1, asmap));

    std::set<int> buckets;
    for (int j = 0; j < 255; j++) {
        CAddrInfo infoj = CAddrInfo(
            CAddress(ResolveService("101." + std::to_string(j) + ".1.1"), NODE_NONE),
            ResolveIP("101." + std::to_string(j) + ".1.1"));
        int bucket = infoj.GetTriedBucket(nKey1, asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the different /16 prefix MAY map to more than
    // 8 buckets.
    BOOST_CHECK(buckets.size() > 8);

    buckets.clear();
    for (int j = 0; j < 255; j++) {
        CAddrInfo infoj = CAddrInfo(
            CAddress(ResolveService("250." + std::to_string(j) + ".1.1"), NODE_NONE),
            ResolveIP("250." + std::to_string(j) + ".1.1"));
        int bucket = infoj.GetTriedBucket(nKey1, asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the different /16 prefix MAY NOT map to more than
    // 8 buckets.
    BOOST_CHECK(buckets.size() == 8);
}

BOOST_AUTO_TEST_CASE(caddrinfo_get_new_bucket)
{
    CAddrManTest addrman;

    CAddress addr1 = CAddress(ResolveService("250.1.2.1", 8333), NODE_NONE);
    CAddress addr2 = CAddress(ResolveService("250.1.2.1", 9999), NODE_NONE);

    CNetAddr source1 = ResolveIP("250.1.2.1");

    CAddrInfo info1 = CAddrInfo(addr1, source1);

    uint256 nKey1 = (uint256)(CHashWriter(SER_GETHASH, 0) << 1).GetHash();
    uint256 nKey2 = (uint256)(CHashWriter(SER_GETHASH, 0) << 2).GetHash();

    std::vector<bool> asmap = FromBytes(raw_tests::asmap, sizeof(raw_tests::asmap) * 8);

    // Test: Make sure the buckets are what we expect
    BOOST_CHECK_EQUAL(info1.GetNewBucket(nKey1, asmap), 795);
    BOOST_CHECK_EQUAL(info1.GetNewBucket(nKey1, source1, asmap), 795);

    // Test: Make sure key actually randomizes bucket placement. A fail on
    //  this test could be a security issue.
    BOOST_CHECK(info1.GetNewBucket(nKey1, asmap) != info1.GetNewBucket(nKey2, asmap));

    // Test: Ports should not affect bucket placement in the addr
    CAddrInfo info2 = CAddrInfo(addr2, source1);
    BOOST_CHECK(info1.GetKey() != info2.GetKey());
    BOOST_CHECK_EQUAL(info1.GetNewBucket(nKey1, asmap), info2.GetNewBucket(nKey1, asmap));

    std::set<int> buckets;
    for (int i = 0; i < 255; i++) {
        CAddrInfo infoi = CAddrInfo(
            CAddress(ResolveService("250.1.1." + std::to_string(i)), NODE_NONE),
            ResolveIP("250.1.1." + std::to_string(i)));
        int bucket = infoi.GetNewBucket(nKey1, asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the same /16 prefix
    // usually map to the same bucket.
    BOOST_CHECK_EQUAL(buckets.size(), 1U);

    buckets.clear();
    for (int j = 0; j < 4 * 255; j++) {
        CAddrInfo infoj = CAddrInfo(CAddress(
                                        ResolveService(
                                            std::to_string(250 + (j / 255)) + "." + std::to_string(j % 256) + ".1.1"), NODE_NONE),
            ResolveIP("251.4.1.1"));
        int bucket = infoj.GetNewBucket(nKey1, asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the same source /16 prefix should not map to more
    // than 64 buckets.
    BOOST_CHECK(buckets.size() <= 64);

    buckets.clear();
    for (int p = 0; p < 255; p++) {
        CAddrInfo infoj = CAddrInfo(
            CAddress(ResolveService("250.1.1.1"), NODE_NONE),
            ResolveIP("101." + std::to_string(p) + ".1.1"));
        int bucket = infoj.GetNewBucket(nKey1, asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the different source /16 prefixes usually map to MORE
    // than 1 bucket.
    BOOST_CHECK(buckets.size() > 1);

    buckets.clear();
    for (int p = 0; p < 255; p++) {
        CAddrInfo infoj = CAddrInfo(
            CAddress(ResolveService("250.1.1.1"), NODE_NONE),
            ResolveIP("250." + std::to_string(p) + ".1.1"));
        int bucket = infoj.GetNewBucket(nKey1, asmap);
        buckets.insert(bucket);
    }
    // Test: IP addresses in the different source /16 prefixes sometimes map to NO MORE
    // than 1 bucket.
    BOOST_CHECK(buckets.size() == 1);

}

BOOST_AUTO_TEST_CASE(addrman_serialization)
{
    std::vector<bool> asmap1 = FromBytes(raw_tests::asmap, sizeof(raw_tests::asmap) * 8);

    CAddrManTest addrman_asmap1(true, asmap1);
    CAddrManTest addrman_asmap1_dup(true, asmap1);
    CAddrManTest addrman_noasmap;
    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);

    CAddress addr = CAddress(ResolveService("250.1.1.1"), NODE_NONE);
    CNetAddr default_source;


    addrman_asmap1.Add(addr, default_source);

    stream << addrman_asmap1;
    // serizalizing/deserializing addrman with the same asmap
    stream >> addrman_asmap1_dup;

    std::pair<int, int> bucketAndEntry_asmap1 = addrman_asmap1.GetBucketAndEntry(addr);
    std::pair<int, int> bucketAndEntry_asmap1_dup = addrman_asmap1_dup.GetBucketAndEntry(addr);
    BOOST_CHECK(bucketAndEntry_asmap1.second != -1);
    BOOST_CHECK(bucketAndEntry_asmap1_dup.second != -1);

    BOOST_CHECK(bucketAndEntry_asmap1.first == bucketAndEntry_asmap1_dup.first);
    BOOST_CHECK(bucketAndEntry_asmap1.second == bucketAndEntry_asmap1_dup.second);

    // deserializing asmaped peers.dat to non-asmaped addrman
    stream << addrman_asmap1;
    stream >> addrman_noasmap;
    std::pair<int, int> bucketAndEntry_noasmap = addrman_noasmap.GetBucketAndEntry(addr);
    BOOST_CHECK(bucketAndEntry_noasmap.second != -1);
    BOOST_CHECK(bucketAndEntry_asmap1.first != bucketAndEntry_noasmap.first);
    BOOST_CHECK(bucketAndEntry_asmap1.second != bucketAndEntry_noasmap.second);

    // deserializing non-asmaped peers.dat to asmaped addrman
    addrman_asmap1.Clear();
    addrman_noasmap.Clear();
    addrman_noasmap.Add(addr, default_source);
    stream << addrman_noasmap;
    stream >> addrman_asmap1;
    std::pair<int, int> bucketAndEntry_asmap1_deser = addrman_asmap1.GetBucketAndEntry(addr);
    BOOST_CHECK(bucketAndEntry_asmap1_deser.second != -1);
    BOOST_CHECK(bucketAndEntry_asmap1_deser.first != bucketAndEntry_noasmap.first);
    BOOST_CHECK(bucketAndEntry_asmap1_deser.first == bucketAndEntry_asmap1_dup.first);
    BOOST_CHECK(bucketAndEntry_asmap1_deser.second == bucketAndEntry_asmap1_dup.second);

    // used to map to different buckets, now maps to the same bucket.
    addrman_asmap1.Clear();
    addrman_noasmap.Clear();
    CAddress addr1 = CAddress(ResolveService("250.1.1.1"), NODE_NONE);
    CAddress addr2 = CAddress(ResolveService("250.2.1.1"), NODE_NONE);
    addrman_noasmap.Add(addr, default_source);
    addrman_noasmap.Add(addr2, default_source);
    std::pair<int, int> bucketAndEntry_noasmap_addr1 = addrman_noasmap.GetBucketAndEntry(addr1);
    std::pair<int, int> bucketAndEntry_noasmap_addr2 = addrman_noasmap.GetBucketAndEntry(addr2);
    BOOST_CHECK(bucketAndEntry_noasmap_addr1.first != bucketAndEntry_noasmap_addr2.first);
    BOOST_CHECK(bucketAndEntry_noasmap_addr1.second != bucketAndEntry_noasmap_addr2.second);
    stream << addrman_noasmap;
    stream >> addrman_asmap1;
    std::pair<int, int> bucketAndEntry_asmap1_deser_addr1 = addrman_asmap1.GetBucketAndEntry(addr1);
    std::pair<int, int> bucketAndEntry_asmap1_deser_addr2 = addrman_asmap1.GetBucketAndEntry(addr2);
    BOOST_CHECK(bucketAndEntry_asmap1_deser_addr1.first == bucketAndEntry_asmap1_deser_addr2.first);
    BOOST_CHECK(bucketAndEntry_asmap1_deser_addr1.second != bucketAndEntry_asmap1_deser_addr2.second);
}


BOOST_AUTO_TEST_CASE(addrman_selecttriedcollision)
{
    CAddrManTest addrman;

    BOOST_CHECK(addrman.size() == 0);

    // Empty addrman should return blank addrman info.
    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");

    // Add twenty two addresses.
    CNetAddr source = ResolveIP("252.2.2.2");
    for (unsigned int i = 1; i < 23; i++) {
        CService addr = ResolveService("250.1.1."+std::to_string(i));
        addrman.Add(CAddress(addr, NODE_NONE), source);
        addrman.Good(addr);

        // No collisions yet.
        BOOST_CHECK(addrman.size() == i);
        BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");
    }

    // Ensure Good handles duplicates well.
    for (unsigned int i = 1; i < 23; i++) {
        CService addr = ResolveService("250.1.1."+std::to_string(i));
        addrman.Good(addr);

        BOOST_CHECK(addrman.size() == 22);
        BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");
    }

}

BOOST_AUTO_TEST_CASE(addrman_noevict)
{
    CAddrManTest addrman;

    // Add twenty two addresses.
    CNetAddr source = ResolveIP("252.2.2.2");
    for (unsigned int i = 1; i < 23; i++) {
        CService addr = ResolveService("250.1.1."+std::to_string(i));
        addrman.Add(CAddress(addr, NODE_NONE), source);
        addrman.Good(addr);

        // No collision yet.
        BOOST_CHECK(addrman.size() == i);
        BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");
    }

    // Collision between 23 and 19.
    CService addr23 = ResolveService("250.1.1.23");
    addrman.Add(CAddress(addr23, NODE_NONE), source);
    addrman.Good(addr23);

    BOOST_CHECK(addrman.size() == 23);
    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "250.1.1.19:0");

    // 23 should be discarded and 19 not evicted.
    addrman.ResolveCollisions();
    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");

    // Lets create two collisions.
    for (unsigned int i = 24; i < 33; i++) {
        CService addr = ResolveService("250.1.1."+std::to_string(i));
        addrman.Add(CAddress(addr, NODE_NONE), source);
        addrman.Good(addr);

        BOOST_CHECK(addrman.size() == i);
        BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");
    }

    // Cause a collision.
    CService addr33 = ResolveService("250.1.1.33");
    addrman.Add(CAddress(addr33, NODE_NONE), source);
    addrman.Good(addr33);
    BOOST_CHECK(addrman.size() == 33);

    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "250.1.1.27:0");

    // Cause a second collision.
    addrman.Add(CAddress(addr23, NODE_NONE), source);
    addrman.Good(addr23);
    BOOST_CHECK(addrman.size() == 33);

    BOOST_CHECK(addrman.SelectTriedCollision().ToString() != "[::]:0");
    addrman.ResolveCollisions();
    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");
}

BOOST_AUTO_TEST_CASE(addrman_evictionworks)
{
    CAddrManTest addrman;

    BOOST_CHECK(addrman.size() == 0);

    // Empty addrman should return blank addrman info.
    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");

    // Add twenty two addresses.
    CNetAddr source = ResolveIP("252.2.2.2");
    for (unsigned int i = 1; i < 23; i++) {
        CService addr = ResolveService("250.1.1."+std::to_string(i));
        addrman.Add(CAddress(addr, NODE_NONE), source);
        addrman.Good(addr);

        // No collision yet.
        BOOST_CHECK(addrman.size() == i);
        BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");
    }

    // Collision between 23 and 19.
    CService addr = ResolveService("250.1.1.23");
    addrman.Add(CAddress(addr, NODE_NONE), source);
    addrman.Good(addr);

    BOOST_CHECK(addrman.size() == 23);
    CAddrInfo info = addrman.SelectTriedCollision();
    BOOST_CHECK(info.ToString() == "250.1.1.19:0");

    // Ensure test of address fails, so that it is evicted.
    addrman.SimConnFail(info);

    // Should swap 23 for 19.
    addrman.ResolveCollisions();
    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");

    // If 23 was swapped for 19, then this should cause no collisions.
    addrman.Add(CAddress(addr, NODE_NONE), source);
    addrman.Good(addr);

    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");

    // If we insert 19 is should collide with 23.
    CService addr19 = ResolveService("250.1.1.19");
    addrman.Add(CAddress(addr19, NODE_NONE), source);
    addrman.Good(addr19);

    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "250.1.1.23:0");

    addrman.ResolveCollisions();
    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");
}


BOOST_AUTO_TEST_SUITE_END()
