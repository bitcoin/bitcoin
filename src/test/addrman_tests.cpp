// Copyright (c) 2012-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "addrman.h"
#include "test/test_bitcoin.h"
#include <string>
#include <boost/test/unit_test.hpp>

#include "random.h"

using namespace std;

class CAddrManTest : public CAddrMan{};

BOOST_FIXTURE_TEST_SUITE(addrman_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(addrman_simple)
{
    CAddrManTest addrman;

    // Set addrman addr placement to be deterministic.
    addrman.MakeDeterministic();

    CNetAddr source = CNetAddr("252.2.2.2:8333");

    // Test 1: Does Addrman respond correctly when empty.
    BOOST_CHECK(addrman.size() == 0);
    CAddrInfo addr_null = addrman.Select();
    BOOST_CHECK(addr_null.ToString() == "[::]:0");

    // Test 2: Does Addrman::Add work as expected.
    CService addr1 = CService("250.1.1.1:8333");
    addrman.Add(CAddress(addr1), source);
    BOOST_CHECK(addrman.size() == 1);
    CAddrInfo addr_ret1 = addrman.Select();
    BOOST_CHECK(addr_ret1.ToString() == "250.1.1.1:8333");

    // Test 3: Does IP address deduplication work correctly. 
    //  Expected dup IP should not be added.
    CService addr1_dup = CService("250.1.1.1:8333");
    addrman.Add(CAddress(addr1_dup), source);
    BOOST_CHECK(addrman.size() == 1);


    // Test 5: New table has one addr and we add a diff addr we should
    //  have two addrs.
    CService addr2 = CService("250.1.1.2:8333");
    addrman.Add(CAddress(addr2), source);
    BOOST_CHECK(addrman.size() == 2);

    // Test 6: AddrMan::Clear() should empty the new table. 
    addrman.Clear();
    BOOST_CHECK(addrman.size() == 0);
    CAddrInfo addr_null2 = addrman.Select();
    BOOST_CHECK(addr_null2.ToString() == "[::]:0");
}

BOOST_AUTO_TEST_CASE(addrman_ports)
{
    CAddrManTest addrman;

    // Set addrman addr placement to be deterministic.
    addrman.MakeDeterministic();

    CNetAddr source = CNetAddr("252.2.2.2:8333");

    BOOST_CHECK(addrman.size() == 0);

    // Test 7; Addr with same IP but diff port does not replace existing addr.
    CService addr1 = CService("250.1.1.1:8333");
    addrman.Add(CAddress(addr1), source);
    BOOST_CHECK(addrman.size() == 1);

    CService addr1_port = CService("250.1.1.1:8334");
    addrman.Add(CAddress(addr1_port), source);
    BOOST_CHECK(addrman.size() == 1);
    CAddrInfo addr_ret2 = addrman.Select();
    BOOST_CHECK(addr_ret2.ToString() == "250.1.1.1:8333");

    // Test 8: Add same IP but diff port to tried table, it doesn't get added.
    //  Perhaps this is not ideal behavior but it is the current behavior.
    addrman.Good(CAddress(addr1_port));
    BOOST_CHECK(addrman.size() == 1);
    bool newOnly = true;
    CAddrInfo addr_ret3 = addrman.Select(newOnly);
    BOOST_CHECK(addr_ret3.ToString() == "250.1.1.1:8333");
}


BOOST_AUTO_TEST_CASE(addrman_select)
{
    CAddrManTest addrman;

    // Set addrman addr placement to be deterministic.
    addrman.MakeDeterministic();

    CNetAddr source = CNetAddr("252.2.2.2:8333");

    // Test 9: Select from new with 1 addr in new.
    CService addr1 = CService("250.1.1.1:8333");
    addrman.Add(CAddress(addr1), source);
    BOOST_CHECK(addrman.size() == 1);

    bool newOnly = true;
    CAddrInfo addr_ret1 = addrman.Select(newOnly);
    BOOST_CHECK(addr_ret1.ToString() == "250.1.1.1:8333");


    // Test 10: move addr to tried, select from new expected nothing returned.
    addrman.Good(CAddress(addr1));
    BOOST_CHECK(addrman.size() == 1);
    CAddrInfo addr_ret2 = addrman.Select(newOnly);
    BOOST_CHECK(addr_ret2.ToString() == "[::]:0");

    CAddrInfo addr_ret3 = addrman.Select();
    BOOST_CHECK(addr_ret3.ToString() == "250.1.1.1:8333");
}

BOOST_AUTO_TEST_CASE(addrman_new_collisions)
{
    CAddrManTest addrman;

    // Set addrman addr placement to be deterministic.
    addrman.MakeDeterministic();

    CNetAddr source = CNetAddr("252.2.2.2:8333");

    BOOST_CHECK(addrman.size() == 0);

    for (unsigned int i = 1; i < 4; i++){
        CService addr = CService("250.1.1."+boost::to_string(i));
        addrman.Add(CAddress(addr), source);

        //Test 11: No collision in new table yet.
        BOOST_CHECK(addrman.size() == i);
    }

    //Test 12: new table collision!
    CService addr1 = CService("250.1.1.4");
    addrman.Add(CAddress(addr1), source);
    BOOST_CHECK(addrman.size() == 3);

    CService addr2 = CService("250.1.1.5");
    addrman.Add(CAddress(addr2), source);
    BOOST_CHECK(addrman.size() == 4);
}

BOOST_AUTO_TEST_CASE(addrman_tried_collisions)
{
    CAddrManTest addrman;

    // Set addrman addr placement to be deterministic.
    addrman.MakeDeterministic();

    CNetAddr source = CNetAddr("252.2.2.2:8333");

    BOOST_CHECK(addrman.size() == 0);

    for (unsigned int i = 1; i < 75; i++){
        CService addr = CService("250.1.1."+boost::to_string(i));
        addrman.Add(CAddress(addr), source);
        addrman.Good(CAddress(addr));

        //Test 13: No collision in tried table yet.
        BOOST_TEST_MESSAGE(addrman.size());
        BOOST_CHECK(addrman.size() == i);
    }

    //Test 14: tried table collision!
    CService addr1 = CService("250.1.1.76");
    addrman.Add(CAddress(addr1), source);
    BOOST_CHECK(addrman.size() == 74);

    CService addr2 = CService("250.1.1.77");
    addrman.Add(CAddress(addr2), source);
    BOOST_CHECK(addrman.size() == 75);
}


BOOST_AUTO_TEST_SUITE_END()