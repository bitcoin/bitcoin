// Copyright (c) 2012-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define __ADDRMAN_TEST__

#include "addrman.h"
#include "test/test_bitcoin.h"
#include <string>
#include <boost/test/unit_test.hpp>

#include "random.h"

using namespace std;

class CAddrManTest : public CAddrMan
{
    public:
        // Simulates a conneciton failure.
        void SimConnFail(CService& addr){
            int64_t nTime = 1; 
            Good_(addr, true, nTime); // Set last good connection in the deep past.
            Attempt(addr); 
        }
};


BOOST_FIXTURE_TEST_SUITE(addrman_tests, BasicTestingSetup)


BOOST_AUTO_TEST_CASE(addrman_selecttriedcollision)
{
    CAddrManTest addrman;

    // Set addrman addr placement to be deterministic.
    addrman.MakeDeterministic();

    BOOST_CHECK(addrman.size() == 0);

    // Empty addrman should return blank addrman info.
    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");

    // Add twenty two addresses.
    CNetAddr source = CNetAddr("252.2.2.2");
    for (unsigned int i = 1; i < 23; i++){
        CService addr = CService("250.1.1."+boost::to_string(i));
        addrman.Add(CAddress(addr), source);
        addrman.Good(addr);

        // No collisions yet.
        BOOST_CHECK(addrman.size() == i);
        BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");
    }

    // Ensure Good handles duplicates well.
    for (unsigned int i = 1; i < 23; i++){
        CService addr = CService("250.1.1."+boost::to_string(i));
        addrman.Good(addr);

        BOOST_CHECK(addrman.size() == 22);
        BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");
    }

}

BOOST_AUTO_TEST_CASE(addrman_noevict)
{
    CAddrManTest addrman;

    // Set addrman addr placement to be deterministic.
    addrman.MakeDeterministic();

    // Add twenty two addresses.
    CNetAddr source = CNetAddr("252.2.2.2");
    for (unsigned int i = 1; i < 23; i++){
        CService addr = CService("250.1.1."+boost::to_string(i));
        addrman.Add(CAddress(addr), source);
        addrman.Good(addr);

        // No collision yet.
        BOOST_CHECK(addrman.size() == i);
        BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");
    }

    // Collision between 23 and 19.
    CService addr23 = CService("250.1.1.23");
    addrman.Add(CAddress(addr23), source);
    addrman.Good(addr23);

    BOOST_CHECK(addrman.size() == 23);
    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "250.1.1.19:0");

    // 23 should be discarded and 19 not evicted.
    addrman.ResolveCollisions();
    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");

    // Lets create two collisions.
    for (unsigned int i = 24; i < 33; i++){
        CService addr = CService("250.1.1."+boost::to_string(i));
        addrman.Add(CAddress(addr), source);
        addrman.Good(addr);

        BOOST_CHECK(addrman.size() == i);
        BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");
    }

    // Cause a collision.
    CService addr33 = CService("250.1.1.33");
    addrman.Add(CAddress(addr33), source);
    addrman.Good(addr33);
    BOOST_CHECK(addrman.size() == 33);

    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "250.1.1.27:0");

    // Cause a second collision.
    addrman.Add(CAddress(addr23), source);
    addrman.Good(addr23);
    BOOST_CHECK(addrman.size() == 33);

    BOOST_CHECK(addrman.SelectTriedCollision().ToString() != "[::]:0");
    addrman.ResolveCollisions();
    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");
}

BOOST_AUTO_TEST_CASE(addrman_evictionworks)
{
    CAddrManTest addrman;

    // Set addrman addr placement to be deterministic.
    addrman.MakeDeterministic();

    BOOST_CHECK(addrman.size() == 0);

    // Empty addrman should return blank addrman info.
    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");

    // Add twenty two addresses.
    CNetAddr source = CNetAddr("252.2.2.2");
    for (unsigned int i = 1; i < 23; i++){
        CService addr = CService("250.1.1."+boost::to_string(i));
        addrman.Add(CAddress(addr), source);
        addrman.Good(addr);

        // No collision yet.
        BOOST_CHECK(addrman.size() == i);
        BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");
    }

    // Collision between 23 and 19.
    CService addr = CService("250.1.1.23");
    addrman.Add(CAddress(addr), source);
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
    addrman.Add(CAddress(addr), source);
    addrman.Good(addr);

    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");

    // If we insert 19 is should collide with 23.
    CService addr19 = CService("250.1.1.19");
    addrman.Add(CAddress(addr19), source);
    addrman.Good(addr19);

    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "250.1.1.23:0");

    addrman.ResolveCollisions();
    BOOST_CHECK(addrman.SelectTriedCollision().ToString() == "[::]:0");

}


BOOST_AUTO_TEST_SUITE_END()
