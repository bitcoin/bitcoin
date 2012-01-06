//
// Unit tests for denial-of-service detection/prevention code
//
#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "main.h"
#include "wallet.h"
#include "net.h"
#include "util.h"

#include <stdint.h>

CService ip(uint32_t i)
{
    struct in_addr s;
    s.s_addr = i;
    return CService(CNetAddr(s), GetDefaultPort());
}

BOOST_AUTO_TEST_SUITE(DoS_tests)

BOOST_AUTO_TEST_CASE(DoS_banning)
{
    CNode::ClearBanned();
    CAddress addr1(ip(0xa0b0c001));
    CNode dummyNode1(INVALID_SOCKET, addr1, true);
    dummyNode1.Misbehaving(100); // Should get banned
    BOOST_CHECK(CNode::IsBanned(addr1));
    BOOST_CHECK(!CNode::IsBanned(ip(0xa0b0c001|0x0000ff00))); // Different ip, not banned

    CAddress addr2(ip(0xa0b0c002));
    CNode dummyNode2(INVALID_SOCKET, addr2, true);
    dummyNode2.Misbehaving(50);
    BOOST_CHECK(!CNode::IsBanned(addr2)); // 2 not banned yet...
    BOOST_CHECK(CNode::IsBanned(addr1));  // ... but 1 still should be
    dummyNode2.Misbehaving(50);
    BOOST_CHECK(CNode::IsBanned(addr2));
}    

BOOST_AUTO_TEST_CASE(DoS_banscore)
{
    CNode::ClearBanned();
    mapArgs["-banscore"] = "111"; // because 11 is my favorite number
    CAddress addr1(ip(0xa0b0c001));
    CNode dummyNode1(INVALID_SOCKET, addr1, true);
    dummyNode1.Misbehaving(100);
    BOOST_CHECK(!CNode::IsBanned(addr1));
    dummyNode1.Misbehaving(10);
    BOOST_CHECK(!CNode::IsBanned(addr1));
    dummyNode1.Misbehaving(1);
    BOOST_CHECK(CNode::IsBanned(addr1));
    mapArgs["-banscore"] = "100";
}

BOOST_AUTO_TEST_CASE(DoS_bantime)
{
    CNode::ClearBanned();
    int64 nStartTime = GetTime();
    SetMockTime(nStartTime); // Overrides future calls to GetTime()

    CAddress addr(ip(0xa0b0c001));
    CNode dummyNode(INVALID_SOCKET, addr, true);

    dummyNode.Misbehaving(100);
    BOOST_CHECK(CNode::IsBanned(addr));

    SetMockTime(nStartTime+60*60);
    BOOST_CHECK(CNode::IsBanned(addr));

    SetMockTime(nStartTime+60*60*24+1);
    BOOST_CHECK(!CNode::IsBanned(addr));
}

static bool CheckNBits(unsigned int nbits1, int64 time1, unsigned int nbits2, int64 time2)\
{
    if (time1 > time2)
        return CheckNBits(nbits2, time2, nbits1, time1);
    int64 deltaTime = time2-time1;

    CBigNum required;
    required.SetCompact(ComputeMinWork(nbits1, deltaTime));
    CBigNum have;
    have.SetCompact(nbits2);
    return (have <= required);
}

BOOST_AUTO_TEST_CASE(DoS_checknbits)
{
    using namespace boost::assign; // for 'map_list_of()'

    // Timestamps,nBits from the bitcoin blockchain.
    // These are the block-chain checkpoint blocks
    typedef std::map<int64, unsigned int> BlockData;
    BlockData chainData =
        map_list_of(1239852051,486604799)(1262749024,486594666)
        (1279305360,469854461)(1280200847,469830746)(1281678674,469809688)
        (1296207707,453179945)(1302624061,453036989)(1309640330,437004818)
        (1313172719,436789733);

    // Make sure CheckNBits considers every combination of block-chain-lock-in-points
    // "sane":
    BOOST_FOREACH(const BlockData::value_type& i, chainData)
    {
        BOOST_FOREACH(const BlockData::value_type& j, chainData)
        {
            BOOST_CHECK(CheckNBits(i.second, i.first, j.second, j.first));
        }
    }

    // Test a couple of insane combinations:
    BlockData::value_type firstcheck = *(chainData.begin());
    BlockData::value_type lastcheck = *(chainData.rbegin());

    // First checkpoint difficulty at or a while after the last checkpoint time should fail when
    // compared to last checkpoint
    BOOST_CHECK(!CheckNBits(firstcheck.second, lastcheck.first+60*10, lastcheck.second, lastcheck.first));
    BOOST_CHECK(!CheckNBits(firstcheck.second, lastcheck.first+60*60*24*14, lastcheck.second, lastcheck.first));

    // ... but OK if enough time passed for difficulty to adjust downward:
    BOOST_CHECK(CheckNBits(firstcheck.second, lastcheck.first+60*60*24*365*4, lastcheck.second, lastcheck.first));
    
}

BOOST_AUTO_TEST_SUITE_END()
