// Copyright (c) 2014-2017 The Dash Core developers

#include "cachemap.h"

#include "test/test_dash.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(cachemap_tests, BasicTestingSetup)

bool Compare(const CacheMap<int,int>& map1, const CacheMap<int,int>& map2 )
{
    if(map1.GetMaxSize() != map2.GetMaxSize()) {
        return false;
    }

    if(map1.GetSize() != map2.GetSize()) {
        return false;
    }

    const CacheMap<int,int>::list_t& items1 = map1.GetItemList();
    for(CacheMap<int,int>::list_cit it = items1.begin(); it != items1.end(); ++it) {
        if(!map2.HasKey(it->key)) {
            return false;
        }
        int val = 0;
        if(!map2.Get(it->key, val)) {
            return false;
        }
        if(it->value != val) {
            return false;
        }
    }

    const CacheMap<int,int>::list_t& items2 = map2.GetItemList();
    for(CacheMap<int,int>::list_cit it = items2.begin(); it != items2.end(); ++it) {
        if(!map1.HasKey(it->key)) {
            return false;
        }
        int val = 0;
        if(!map1.Get(it->key, val)) {
            return false;
        }
        if(it->value != val) {
            return false;
        }
    }

    return true;
}

BOOST_AUTO_TEST_CASE(cachemap_test)
{
    // create a CacheMap limited to 10 items
    CacheMap<int,int> mapTest1(10);

    // check that the max size is 10
    BOOST_CHECK(mapTest1.GetMaxSize() == 10);

    // check that the size is 0
    BOOST_CHECK(mapTest1.GetSize() == 0);

    // insert (-1, -1)
    mapTest1.Insert(-1, -1);

    // make sure that the size is updated
    BOOST_CHECK(mapTest1.GetSize() == 1);

    // make sure the map contains the key
    BOOST_CHECK(mapTest1.HasKey(-1) == true);

    // add 10 items
    for(int i = 0; i < 10; ++i) {
        mapTest1.Insert(i, i);
    }

    // check that the size is 10
    BOOST_CHECK(mapTest1.GetSize() == 10);

    // check that the map contains the expected items
    for(int i = 0; i < 10; ++i) {
        int nVal = 0;
        BOOST_CHECK(mapTest1.Get(i, nVal) == true);
        BOOST_CHECK(nVal == i);
    }

    // check that the map no longer contains the first item
    BOOST_CHECK(mapTest1.HasKey(-1) == false);

    // erase an item
    mapTest1.Erase(5);

    // check the size
    BOOST_CHECK(mapTest1.GetSize() == 9);

    // check that the map no longer contains the item
    BOOST_CHECK(mapTest1.HasKey(5) == false);

    // check that the map contains the expected items
    int expected[] = { 0, 1, 2, 3, 4, 6, 7, 8, 9 };
    for(size_t i = 0; i < 9; ++i) {
        int nVal = 0;
        int eVal = expected[i];
        BOOST_CHECK(mapTest1.Get(eVal, nVal) == true);
        BOOST_CHECK(nVal == eVal);
    }

    // test serialization
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << mapTest1;

    CacheMap<int,int> mapTest2;
    ss >> mapTest2;

    BOOST_CHECK(Compare(mapTest1, mapTest2));

    // test copy constructor
    CacheMap<int,int> mapTest3(mapTest1);
    BOOST_CHECK(Compare(mapTest1, mapTest3));

    // test assignment operator
    CacheMap<int,int> mapTest4;
    mapTest4 = mapTest1;
    BOOST_CHECK(Compare(mapTest1, mapTest4));
}

BOOST_AUTO_TEST_SUITE_END()
