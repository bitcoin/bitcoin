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

CNetAddr UtilBuildAddress(const std::string& address)
{
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sockaddr_in));
    inet_pton(AF_INET, address.c_str(), &(sa.sin_addr));
    CNetAddr addr = CNetAddr(sa.sin_addr);
    return addr;
}

void UtilPreconditionIsAtLeastFiveEntriesRequired(const std::string& baseip, int basevalue)
{
    for (int i = CountOffsetSamples(); i < 5; i++) { // precondition 1: at least 5 entries required to compute any offset
        int val = basevalue + i;
        std::stringstream stream;
        stream << baseip << val;
        std::string ip = stream.str();
        AddTimeData(UtilBuildAddress(ip), val);
    }
}

BOOST_AUTO_TEST_CASE(util_AddTimeDataComputeOffsetWhenSampleCountIsUneven)
{
    UtilPreconditionIsAtLeastFiveEntriesRequired("1.1.1.", 200); // precondition 1: at least 5 entries required to compute any offset
    BOOST_CHECK(CountOffsetSamples() >= 5);


    if ((CountOffsetSamples() % 2) == 1) { // precondition 2: start with an even number of samples
        AddTimeData(UtilBuildAddress("1.1.1.210"), 210);
    }

    BOOST_CHECK(CountOffsetSamples() % 2 == 0);


    int64_t offset = GetTimeOffset();
    int samples = CountOffsetSamples();
    AddTimeData(UtilBuildAddress("1.1.1.211"), 211);

    BOOST_CHECK_EQUAL(CountOffsetSamples(), samples + 1); // sample was added
    BOOST_CHECK(GetTimeOffset() != offset);               // and new offset was computed
}

BOOST_AUTO_TEST_CASE(util_AddTimeDataDoNotComputeOffsetWhenSampleCountIsEven)
{
    UtilPreconditionIsAtLeastFiveEntriesRequired("1.1.1.", 100); // precondition 1: at least 5 entries required to compute any offset

    BOOST_CHECK(CountOffsetSamples() >= 5);

    if (CountOffsetSamples() % 2 == 0) { // precondition 2: start with an uneven number of samples
        AddTimeData(UtilBuildAddress("1.1.1.110"), 110);
    }

    BOOST_CHECK(CountOffsetSamples() % 2 == 1);


    int64_t offset = GetTimeOffset();
    int samples = CountOffsetSamples();
    AddTimeData(UtilBuildAddress("1.1.1.111"), 111);

    BOOST_CHECK_EQUAL(CountOffsetSamples(), samples + 1); // sample was added
    BOOST_CHECK_EQUAL(GetTimeOffset(), offset);           //new offset was not computed
}

BOOST_AUTO_TEST_CASE(util_AddTimeDataIgnoreSampleWithDuplicateIP)
{
    UtilPreconditionIsAtLeastFiveEntriesRequired("1.1.3.", 50); // precondition 1: at least 5 entries required to compute any offset
    BOOST_CHECK(CountOffsetSamples() >= 5);


    AddTimeData(UtilBuildAddress("1.1.3.60"), 60);

    int64_t offset = GetTimeOffset();
    int samples = CountOffsetSamples();
    AddTimeData(UtilBuildAddress("1.1.3.60"), 61);

    BOOST_CHECK_EQUAL(CountOffsetSamples(), samples); // sample was ignored
    BOOST_CHECK_EQUAL(GetTimeOffset(), offset);       //new offset was not computed
}

BOOST_AUTO_TEST_CASE(util_AddTimeDataAlgorithmMedianIsWithinBounds)
{
    int limit = 10;
    int64_t offset = 0;
    std::set<CNetAddr> knownSet;
    CMedianFilter<int64_t> offsetFilter(limit, 0); // max size : 10 , init value: 0


    for (int sample = 1; sample < 4; sample++) { // precondition: 4 samples, all within bounds
        std::stringstream stream;
        stream << "1.1.1." << sample;
        std::string ip = stream.str();
        AddTimeDataAlgorithm(UtilBuildAddress(ip), sample, knownSet, offsetFilter, offset);
    }

    BOOST_CHECK_EQUAL(offset, 0);
    BOOST_CHECK_EQUAL(offsetFilter.size(), 4);

    AddTimeDataAlgorithm(UtilBuildAddress("1.1.1.200"), 200, knownSet, offsetFilter, offset);

    BOOST_CHECK_EQUAL(offset, offsetFilter.median());
    BOOST_CHECK_EQUAL(offsetFilter.size(), 5);
}

BOOST_AUTO_TEST_CASE(util_AddTimeDataAlgorithmMedianIsOutsideBounds)
{
    int limit = 10;
    int64_t offset = 0;
    std::set<CNetAddr> knownSet;
    CMedianFilter<int64_t> offsetFilter(limit, 0); // max size : limit , init value: 0

    for (int sample = 1; sample < 4; sample++) { // precondition: 4 samples, all outside bounds
        std::stringstream stream;
        stream << "1.1.1." << 1 + sample;
        std::string ip = stream.str();
        AddTimeDataAlgorithm(UtilBuildAddress(ip), 2 * DEFAULT_MAX_TIME_ADJUSTMENT, knownSet, offsetFilter, offset);
    } // sorted filter: 0 x x x  -- x is outside the boundaries

    BOOST_CHECK_EQUAL(offset, 0);
    BOOST_CHECK_EQUAL(offsetFilter.size(), 4);

    // offset is computed only when number of entries is uneven
    AddTimeDataAlgorithm(UtilBuildAddress("1.1.1.5"), 1, knownSet, offsetFilter, offset); // sorted filter : 0 1 (x) x x  -- median (x)
    BOOST_CHECK_EQUAL(offset, 0);
    AddTimeDataAlgorithm(UtilBuildAddress("1.1.1.6"), 1, knownSet, offsetFilter, offset); // sorted filter : 0 1 1 x x x
    BOOST_CHECK_EQUAL(offset, 0);
    AddTimeDataAlgorithm(UtilBuildAddress("1.1.1.7"), 1, knownSet, offsetFilter, offset);                               // sorted filter : 0 1 1 (1) x x x x -- median (1)
    BOOST_CHECK_EQUAL(offset, 1);                                                                                       // flip to 1
    AddTimeDataAlgorithm(UtilBuildAddress("1.1.1.8"), 2 * DEFAULT_MAX_TIME_ADJUSTMENT, knownSet, offsetFilter, offset); // sorted filter : 0 1 1 1 x x x x x
    BOOST_CHECK_EQUAL(offset, 1);
    AddTimeDataAlgorithm(UtilBuildAddress("1.1.1.9"), 2 * DEFAULT_MAX_TIME_ADJUSTMENT, knownSet, offsetFilter, offset); // sorted filter : 0 1 1 1 (x) x x x x x -- median (x)
    BOOST_CHECK_EQUAL(offset, 0);                                                                                       // flip back to zero

    BOOST_CHECK_EQUAL(offsetFilter.size(), 9);
}

BOOST_AUTO_TEST_CASE(util_AddTimeDataAlgorithmAtLeastFiveSamplesToComputeOffset)
{
    int limit = 10;
    int64_t offset = 0;
    std::set<CNetAddr> knownSet;
    CMedianFilter<int64_t> offsetFilter(limit, 0); // max size : limit , init value: 0

    AddTimeDataAlgorithm(UtilBuildAddress("1.1.1.1"), 1, knownSet, offsetFilter, offset);
    BOOST_CHECK_EQUAL(offset, 0);

    AddTimeDataAlgorithm(UtilBuildAddress("1.1.1.2"), 2, knownSet, offsetFilter, offset);
    BOOST_CHECK_EQUAL(offset, 0);

    AddTimeDataAlgorithm(UtilBuildAddress("1.1.1.3"), 3, knownSet, offsetFilter, offset);
    BOOST_CHECK_EQUAL(offset, 0);

    AddTimeDataAlgorithm(UtilBuildAddress("1.1.1.4"), 4, knownSet, offsetFilter, offset); // this is the fifth entry
    BOOST_CHECK_EQUAL(offset, 2);
}

BOOST_AUTO_TEST_CASE(util_AddTimeDataAlgorithmIgnoresSamplesBeyondInternalLimit)
{
    int limit = 10;
    int64_t offset = 0;
    std::set<CNetAddr> knownSet;
    CMedianFilter<int64_t> offsetFilter(limit, 0); // max size : limit , init value: 0


    for (int sample = 1; sample < limit; sample++) { // precondition: limit samples
        std::stringstream stream;
        stream << "1.1.1." << sample;
        std::string ip = stream.str();
        AddTimeDataAlgorithm(UtilBuildAddress(ip), sample, knownSet, offsetFilter, offset);
    }

    BOOST_CHECK_EQUAL(offsetFilter.size(), limit);

    int64_t pre = offset;
    int size = offsetFilter.size();

    AddTimeDataAlgorithm(UtilBuildAddress("1.1.1.200"), 200, knownSet, offsetFilter, offset);

    BOOST_CHECK_EQUAL(offset, pre);
    BOOST_CHECK_EQUAL(offsetFilter.size(), size);
}

BOOST_AUTO_TEST_SUITE_END()
