// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <netaddress.h>
#include <test/test_bitcoin.h>
#include <timedata.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(timedata_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(util_MedianFilter)
{
    CMedianFilter<int> filter(5, 15);

    BOOST_CHECK_EQUAL(filter.median(), 15);

    filter.input(20); // [15 20]
    BOOST_CHECK_EQUAL(filter.median(), 17);

    filter.input(30); // [15 20 30]
    BOOST_CHECK_EQUAL(filter.median(), 20);

    filter.input(3); // [3 15 20 30]
    BOOST_CHECK_EQUAL(filter.median(), 17);

    filter.input(7); // [3 7 15 20 30]
    BOOST_CHECK_EQUAL(filter.median(), 15);

    filter.input(18); // [3 7 18 20 30]
    BOOST_CHECK_EQUAL(filter.median(), 18);

    filter.input(0); // [0 3 7 18 30]
    BOOST_CHECK_EQUAL(filter.median(), 7);
}

BOOST_AUTO_TEST_CASE(util_MedianFilterShallNotGrowBeyondSize)
{
    CMedianFilter<int> filter(2, 15);

    BOOST_CHECK_EQUAL(filter.size(), 1); // 15

    filter.input(100); // 15 100
    BOOST_CHECK_EQUAL(filter.size(), 2);

    filter.input(10); // 100 10
    BOOST_CHECK_EQUAL(filter.size(), 2);
    BOOST_CHECK_EQUAL(filter.sorted()[0], 10);
    BOOST_CHECK_EQUAL(filter.sorted()[1], 100);
}

/* Utility function to build a CNetAddr object.

   Using memcpy to build sockaddr_in because compat.h does not support inet_pton with current #define _WIN32_WINNT 0x0501
*/
CNetAddr UtilBuildAddress(unsigned char ip1, unsigned char ip2, unsigned char ip3, unsigned char ip4)
{
    unsigned char ip[] = {ip1, ip2, ip3, ip4};

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sockaddr_in)); // initialize the memory block
    memcpy(&(sa.sin_addr), &ip, sizeof(ip));
    CNetAddr addr = CNetAddr(sa.sin_addr);
    return addr;
}

BOOST_AUTO_TEST_CASE(util_UtilBuildAddress)
{
    CNetAddr cn1 = UtilBuildAddress(0x001, 0x001, 0x001, 0x0D2); // 1.1.1.210

    CNetAddr cn2 = UtilBuildAddress(0x001, 0x001, 0x001, 0x0D2); // 1.1.1.210

    bool eq = (cn1 == cn2);

    BOOST_CHECK(eq);

    CNetAddr cn3 = UtilBuildAddress(0x001, 0x001, 0x001, 0x0D3); // 1.1.1.211

    bool neq = !(cn1 == cn3);

    BOOST_CHECK(neq);

    neq = !(cn2 == cn3);

    BOOST_CHECK(neq);
}

BOOST_AUTO_TEST_CASE(util_AddTimeDataAlgorithmComputeOffsetWhenNewSampleCountIsGreaterEqualFiveAndUneven)
{
    int limit = 20; //  can store up to 20 samples
    int64_t offset = 0;
    std::set<CNetAddr> knownSet;
    CMedianFilter<int64_t> offsetFilter(limit, 0); // max size : 20 , init sample: 0


    for (unsigned int sample = 1; sample < 10; sample++) { // precondition: at least 4 samples, all within bounds (including the init sample : 0
        CNetAddr addr = UtilBuildAddress(0x001, 0x001, 0x001, sample);
        AddTimeDataAlgorithm(addr, sample, knownSet, offsetFilter, offset); // 1.1.1.[1,2,3],  offsetSample = [1,2,3]
    }

    BOOST_CHECK(offset != 0); // offset has changed from the initial value

    assert(offsetFilter.size() == 10); // next sample will be the 11th (uneven) and will trigger a new computation of the offset


    int samples = offsetFilter.size();
    int64_t oldOffset = offset;
    CNetAddr addr2 = UtilBuildAddress(0x001, 0x001, 0x001, 0x0010);
    AddTimeDataAlgorithm(addr2, 111, knownSet, offsetFilter, offset); // 1.1.1.16 , offsetSample = 111

    BOOST_CHECK_EQUAL(offsetFilter.size(), samples + 1); // sample was added...
    BOOST_CHECK(oldOffset != offset);                    // ...and new offset was computed
}


BOOST_AUTO_TEST_CASE(util_AddTimeDataAlgorithmDoNotComputeOffsetWhenNewSampleCountIsGreaterEqualFiveButEven)
{
    int limit = 20; //  can store up to 20 samples
    int64_t offset = 0;
    std::set<CNetAddr> knownSet;
    CMedianFilter<int64_t> offsetFilter(limit, 0); // max size : 20 , init sample: 0


    for (unsigned int sample = 1; sample < 9; sample++) { // precondition: at least 4 samples, all within bounds (including the init sample : 0
        CNetAddr addr = UtilBuildAddress(0x001, 0x001, 0x001, sample);
        AddTimeDataAlgorithm(addr, sample, knownSet, offsetFilter, offset); // 1.1.1.[1,2,3],  offsetSample = [1,2,3]
    }

    BOOST_CHECK(offset != 0); // offset has changed from the initial value

    assert(offsetFilter.size() == 9); // next sample will be the 10th (even) and will *not* trigger a new computation of the offset


    int samples = offsetFilter.size();
    int64_t oldOffset = offset;
    CNetAddr addr2 = UtilBuildAddress(0x001, 0x001, 0x001, 0x0010);
    AddTimeDataAlgorithm(addr2, 111, knownSet, offsetFilter, offset); // 1.1.1.16 , offsetSample = 111

    BOOST_CHECK_EQUAL(offsetFilter.size(), samples + 1); // sample was added...
    BOOST_CHECK(oldOffset == offset);                    // ...but new offset was not computed
}


BOOST_AUTO_TEST_CASE(util_AddTimeDataAlgorithmMedianIsWithinBounds)
{
    int limit = 10;
    int64_t offset = 0;
    std::set<CNetAddr> knownSet;
    CMedianFilter<int64_t> offsetFilter(limit, 0); // max size : 10 , init value: 0


    for (unsigned int sample = 1; sample < 4; sample++) { // precondition: 4 samples, all within bounds
        CNetAddr addr = UtilBuildAddress(0x001, 0x001, 0x001, sample);
        AddTimeDataAlgorithm(addr, sample, knownSet, offsetFilter, offset); // 1.1.1.[1,2,3],  offsetSample = [1,2,3]
    }

    BOOST_CHECK_EQUAL(offset, 0);
    BOOST_CHECK_EQUAL(offsetFilter.size(), 4);

    AddTimeDataAlgorithm(UtilBuildAddress(0x001, 0x001, 0x001, 0x0C8), 200, knownSet, offsetFilter, offset); // 1.1.1.200 , offsetSample = 200

    BOOST_CHECK_EQUAL(offset, offsetFilter.median());
    BOOST_CHECK_EQUAL(offsetFilter.size(), 5);
}

BOOST_AUTO_TEST_CASE(util_AddTimeDataAlgorithmMedianIsOutsideBounds)
{
    int limit = 10;
    int64_t offset = 0;
    std::set<CNetAddr> knownSet;
    CMedianFilter<int64_t> offsetFilter(limit, 0); // max size : limit , init value: 0

    for (int sample = 1; sample < 4; sample++) {                                                     // precondition: 4 samples, all outside bounds
        CNetAddr addr = UtilBuildAddress(0x001, 0x001, 0x001, sample);                               // 1.1.1.[1,2,3]
        AddTimeDataAlgorithm(addr, 2 * DEFAULT_MAX_TIME_ADJUSTMENT, knownSet, offsetFilter, offset); // offsetSample  = 2 * DEFAULT_MAX_TIME_ADJUSTMENT (out of bounds)

    } // sorted filter: 0 x x x  -- x is outside the boundaries

    BOOST_CHECK_EQUAL(offset, 0);
    BOOST_CHECK_EQUAL(offsetFilter.size(), 4);

    // offset is computed only when number of entries is uneven
    AddTimeDataAlgorithm(UtilBuildAddress(0x001, 0x001, 0x001, 0x005), 1, knownSet, offsetFilter, offset); // sorted filter : 0 1 (x) x x  -- median (x)
    BOOST_CHECK_EQUAL(offset, 0);
    AddTimeDataAlgorithm(UtilBuildAddress(0x001, 0x001, 0x001, 0x006), 1, knownSet, offsetFilter, offset); // sorted filter : 0 1 1 x x x
    BOOST_CHECK_EQUAL(offset, 0);
    AddTimeDataAlgorithm(UtilBuildAddress(0x001, 0x001, 0x001, 0x007), 1, knownSet, offsetFilter, offset);                               // sorted filter : 0 1 1 (1) x x x x -- median (1)
    BOOST_CHECK_EQUAL(offset, 1);                                                                                                        // flip to 1
    AddTimeDataAlgorithm(UtilBuildAddress(0x001, 0x001, 0x001, 0x008), 2 * DEFAULT_MAX_TIME_ADJUSTMENT, knownSet, offsetFilter, offset); // sorted filter : 0 1 1 1 x x x x x
    BOOST_CHECK_EQUAL(offset, 1);
    AddTimeDataAlgorithm(UtilBuildAddress(0x001, 0x001, 0x001, 0x009), 2 * DEFAULT_MAX_TIME_ADJUSTMENT, knownSet, offsetFilter, offset); // sorted filter : 0 1 1 1 (x) x x x x x -- median (x)
    BOOST_CHECK_EQUAL(offset, 0);                                                                                                        // flip back to zero

    BOOST_CHECK_EQUAL(offsetFilter.size(), 9);
}

BOOST_AUTO_TEST_CASE(util_AddTimeDataAlgorithmAtLeastFiveSamplesToComputeOffset)
{
    int limit = 10;
    int64_t offset = 0;
    std::set<CNetAddr> knownSet;
    CMedianFilter<int64_t> offsetFilter(limit, 0); // max size : limit , init value: 0

    AddTimeDataAlgorithm(UtilBuildAddress(0x001, 0x001, 0x001, 0x001), 1, knownSet, offsetFilter, offset);
    BOOST_CHECK_EQUAL(offset, 0);

    AddTimeDataAlgorithm(UtilBuildAddress(0x001, 0x001, 0x001, 0x002), 2, knownSet, offsetFilter, offset);
    BOOST_CHECK_EQUAL(offset, 0);

    AddTimeDataAlgorithm(UtilBuildAddress(0x001, 0x001, 0x001, 0x003), 3, knownSet, offsetFilter, offset);
    BOOST_CHECK_EQUAL(offset, 0);

    AddTimeDataAlgorithm(UtilBuildAddress(0x001, 0x001, 0x001, 0x004), 4, knownSet, offsetFilter, offset); // this is the fifth entry
    BOOST_CHECK_EQUAL(offset, 2);
}

BOOST_AUTO_TEST_CASE(util_AddTimeDataAlgorithmIgnoreSamplesBeyondInternalCapacity)
{
    int capacity = 10; // internal capacity
    int64_t offset = 0;
    std::set<CNetAddr> knownSet;
    CMedianFilter<int64_t> offsetFilter(capacity, 0); // max size : capacity , initial offset: 0


    for (int sample = 1; sample < capacity; sample++) { // precondition: fill filter up to the capacity
        CNetAddr addr = UtilBuildAddress(0x001, 0x001, 0x001, sample);
        AddTimeDataAlgorithm(addr, sample, knownSet, offsetFilter, offset);
    }

    BOOST_CHECK_EQUAL(offsetFilter.size(), capacity);

    int64_t pre = offset;
    int size = offsetFilter.size();

    AddTimeDataAlgorithm(UtilBuildAddress(0x001, 0x001, 0x001, 0x0C8), 200, knownSet, offsetFilter, offset); // 1.1.1.200, offsetSample = 200

    BOOST_CHECK_EQUAL(offset, pre);
    BOOST_CHECK_EQUAL(offsetFilter.size(), size);
}

BOOST_AUTO_TEST_CASE(util_AddTimeDataAlgorithmIgnoreSampleWithDuplicateIp)
{
    int capacity = 10; // internal capacity
    int64_t offset = 0;
    std::set<CNetAddr> knownSet;
    CMedianFilter<int64_t> offsetFilter(capacity, 0); // max size : capacity , initial offset: 0

    int samples = offsetFilter.size();

    AddTimeDataAlgorithm(UtilBuildAddress(0x001, 0x001, 0x001, 0x0C8), 200, knownSet, offsetFilter, offset); // 1.1.1.200, offsetSample = 200
    BOOST_CHECK_EQUAL(offsetFilter.size(), samples + 1);                                                     // new sample was accepted

    AddTimeDataAlgorithm(UtilBuildAddress(0x001, 0x001, 0x001, 0x0C8), 201, knownSet, offsetFilter, offset); // 1.1.1.200, offsetSample = 201
    BOOST_CHECK_EQUAL(offsetFilter.size(), samples + 1);                                                     // sample with duplicate ip was ignored
}


BOOST_AUTO_TEST_SUITE_END()
