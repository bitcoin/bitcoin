// Copyright (c) 2014-2017 The Dash Core developers

#include "cachemultimap.h"

#include "test/test_dash.h"

#include <algorithm>
#include <iostream>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(cachemultimap_tests, BasicTestingSetup)

void DumpMap(const CacheMultiMap<int,int>& map)
{
    const CacheMultiMap<int,int>::list_t& listItems = map.GetItemList();
    for(CacheMultiMap<int,int>::list_cit it = listItems.begin(); it != listItems.end(); ++it) {
        const CacheItem<int,int>& item = *it;
        std::cout << item.key << " : " << item.value << std::endl;
    }
}

bool Compare(const CacheMultiMap<int,int>& map1, const CacheMultiMap<int,int>& map2 )
{
    if(map1.GetMaxSize() != map2.GetMaxSize()) {
        std::cout << "Compare returning false: max size mismatch" << std::endl;
        return false;
    }

    if(map1.GetSize() != map2.GetSize()) {
        std::cout << "Compare returning false: size mismatch" << std::endl;
        return false;
    }

    const CacheMultiMap<int,int>::list_t& items1 = map1.GetItemList();
    const CacheMultiMap<int,int>::list_t& items2 = map2.GetItemList();
    CacheMultiMap<int,int>::list_cit it2 = items2.begin();
    for(CacheMultiMap<int,int>::list_cit it1 = items1.begin(); it1 != items1.end(); ++it1) {
        const CacheItem<int,int>& item1 = *it1;
        const CacheItem<int,int>& item2 = *it2;
        if(item1.key != item2.key) {
            return false;
        }
        if(item1.value != item2.value) {
            return false;
        }
        ++it2;
    }

    return true;
}

bool CheckExpected(const CacheMultiMap<int,int>& map, int* expected, CacheMultiMap<int,int>::size_type nSize)
{
    if(map.GetSize() != nSize) {
        return false;
    }
    for(CacheMultiMap<int,int>::size_type i = 0; i < nSize; ++i) {
        int nVal = 0;
        int eVal = expected[i];
        if(!map.Get(eVal, nVal)) {
            return false;
        }
        if(nVal != eVal) {
            return false;
        }
    }
    return true;
}

BOOST_AUTO_TEST_CASE(cachemultimap_test)
{
    // create a CacheMultiMap limited to 10 items
    CacheMultiMap<int,int> mapTest1(10);

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
    BOOST_CHECK(CheckExpected(mapTest1, expected, 9 ) == true);

    // add multiple items for the same key
    mapTest1.Insert(5, 2);
    mapTest1.Insert(5, 1);
    mapTest1.Insert(5, 4);

    // check the size
    BOOST_CHECK(mapTest1.GetSize() == 10);

    // check that 2 keys have been removed
    BOOST_CHECK(mapTest1.HasKey(0) == false);
    BOOST_CHECK(mapTest1.HasKey(1) == false);
    BOOST_CHECK(mapTest1.HasKey(2) == true);

    // check multiple values
    std::vector<int> vecVals;
    BOOST_CHECK(mapTest1.GetAll(5, vecVals) == true);
    BOOST_CHECK(vecVals.size() == 3);
    BOOST_CHECK(vecVals[0] == 1);
    BOOST_CHECK(vecVals[1] == 2);
    BOOST_CHECK(vecVals[2] == 4);

//    std::cout << "mapTest1 dump:" << std::endl;
//    DumpMap(mapTest1);

    // test serialization
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << mapTest1;

    CacheMultiMap<int,int> mapTest2;
    ss >> mapTest2;

//    std::cout << "mapTest2 dump:" << std::endl;
//    DumpMap(mapTest2);

    // check multiple values
    std::vector<int> vecVals2;
    BOOST_CHECK(mapTest2.GetAll(5, vecVals2) == true);
    BOOST_CHECK(vecVals2.size() == 3);
    BOOST_CHECK(vecVals2[0] == 1);
    BOOST_CHECK(vecVals2[1] == 2);
    BOOST_CHECK(vecVals2[2] == 4);

    BOOST_CHECK(Compare(mapTest1, mapTest2));

    // test copy constructor
    CacheMultiMap<int,int> mapTest3(mapTest1);
    BOOST_CHECK(Compare(mapTest1, mapTest3));

    // test assignment operator
    CacheMultiMap<int,int> mapTest4;
    mapTest4 = mapTest1;
    BOOST_CHECK(Compare(mapTest1, mapTest4));
}

BOOST_AUTO_TEST_SUITE_END()
