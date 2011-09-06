//
// Unit tests for denial-of-service detection/prevention code
//
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "../main.h"
#include "../net.h"
#include "../util.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(DoS_tests)

BOOST_AUTO_TEST_CASE(DoS_banning)
{
    CNode::ClearBanned();
    CAddress addr1(0xa0b0c001);
    CNode dummyNode1(INVALID_SOCKET, addr1, true);
    dummyNode1.Misbehaving(100); // Should get banned
    BOOST_CHECK(CNode::IsBanned(addr1.ip));
    BOOST_CHECK(!CNode::IsBanned(addr1.ip|0x0000ff00)); // Different ip, not banned

    CAddress addr2(0xa0b0c002);
    CNode dummyNode2(INVALID_SOCKET, addr2, true);
    dummyNode2.Misbehaving(50);
    BOOST_CHECK(!CNode::IsBanned(addr2.ip)); // 2 not banned yet...
    BOOST_CHECK(CNode::IsBanned(addr1.ip));  // ... but 1 still should be
    dummyNode2.Misbehaving(50);
    BOOST_CHECK(CNode::IsBanned(addr2.ip));
}    

BOOST_AUTO_TEST_CASE(DoS_banscore)
{
    CNode::ClearBanned();
    mapArgs["-banscore"] = "111"; // because 11 is my favorite number
    CAddress addr1(0xa0b0c001);
    CNode dummyNode1(INVALID_SOCKET, addr1, true);
    dummyNode1.Misbehaving(100);
    BOOST_CHECK(!CNode::IsBanned(addr1.ip));
    dummyNode1.Misbehaving(10);
    BOOST_CHECK(!CNode::IsBanned(addr1.ip));
    dummyNode1.Misbehaving(1);
    BOOST_CHECK(CNode::IsBanned(addr1.ip));
    mapArgs["-banscore"] = "100";
}

BOOST_AUTO_TEST_CASE(DoS_bantime)
{
    CNode::ClearBanned();
    int64 nStartTime = GetTime();
    SetMockTime(nStartTime); // Overrides future calls to GetTime()

    CAddress addr(0xa0b0c001);
    CNode dummyNode(INVALID_SOCKET, addr, true);

    dummyNode.Misbehaving(100);
    BOOST_CHECK(CNode::IsBanned(addr.ip));

    SetMockTime(nStartTime+60*60);
    BOOST_CHECK(CNode::IsBanned(addr.ip));

    SetMockTime(nStartTime+60*60*24+1);
    BOOST_CHECK(!CNode::IsBanned(addr.ip));
}    


BOOST_AUTO_TEST_SUITE_END()
