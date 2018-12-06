// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <timedata.h>
#include <netaddress.h>
#include <test/test_bitcoin.h>

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


CNetAddr utilBuildAddress(std::string address);

void utilPreconditionIsAtLeastFiveEntriesRequired(std::string baseip, int basevalue);


BOOST_AUTO_TEST_CASE(util_AddTimeDataComputeOffsetWhenSampleCountIsUneven)
{
    utilPreconditionIsAtLeastFiveEntriesRequired("1.1.1.", 200); // precondition 1: at least 5 entries required to compute any offset
    BOOST_CHECK(CountOffsetSamples() >= 5);


    if ((CountOffsetSamples() % 2) == 1) { // precondition 2: start with an even number of samples
        AddTimeData(utilBuildAddress("1.1.1.210"), 210);
    }

    BOOST_CHECK(CountOffsetSamples() % 2 == 0);


    int64_t offset = GetTimeOffset();
    int samples = CountOffsetSamples();
    AddTimeData(utilBuildAddress("1.1.1.211"), 211);

    BOOST_CHECK_EQUAL(CountOffsetSamples(), samples + 1); // sample was added
    BOOST_CHECK(GetTimeOffset() != offset);               // and new offset was computed
}


BOOST_AUTO_TEST_CASE(util_AddTimeDataDoNotComputeOffsetWhenSampleCountIsEven)
{
    utilPreconditionIsAtLeastFiveEntriesRequired("1.1.1.", 100); // precondition 1: at least 5 entries required to compute any offset

    BOOST_CHECK(CountOffsetSamples() >= 5);

    if (CountOffsetSamples() % 2 == 0) { // precondition 2: start with an uneven number of samples
        AddTimeData(utilBuildAddress("1.1.1.110"), 110);
    }

    BOOST_CHECK(CountOffsetSamples() % 2 == 1);


    int64_t offset = GetTimeOffset();
    int samples = CountOffsetSamples();
    AddTimeData(utilBuildAddress("1.1.1.111"), 111);

    BOOST_CHECK_EQUAL(CountOffsetSamples(), samples + 1); // sample was added
    BOOST_CHECK_EQUAL(GetTimeOffset(), offset);           //new offset was not computed
}


BOOST_AUTO_TEST_CASE(util_AddTimeDataIgnoreSampleWithDuplicateIP)
{
    utilPreconditionIsAtLeastFiveEntriesRequired("1.1.1.", 300); // precondition 1: at least 5 entries required to compute any offset
    BOOST_CHECK(CountOffsetSamples() >= 5);


    AddTimeData(utilBuildAddress("1.1.1.310"), 310);

    int64_t offset = GetTimeOffset();
    int samples = CountOffsetSamples();
    AddTimeData(utilBuildAddress("1.1.1.310"), 311);

    BOOST_CHECK_EQUAL(CountOffsetSamples(), samples); // sample was ignored
    BOOST_CHECK_EQUAL(GetTimeOffset(), offset);       //new offset was not computed
}


CNetAddr utilBuildAddress(std::string address)
{
    struct sockaddr_in sa;
    inet_pton(AF_INET, address.c_str(), &(sa.sin_addr));
    CNetAddr addr = CNetAddr(sa.sin_addr);
    return addr;
}


void utilPreconditionIsAtLeastFiveEntriesRequired(std::string baseip, int basevalue)
{
    for (int i = CountOffsetSamples(); i < 5; i++) { // precondition 1: at least 5 entries required to compute any offset
        int val = basevalue + i;
        std::stringstream stream;
        stream << baseip << val;
        std::string ip = stream.str();
        AddTimeData(utilBuildAddress(ip.c_str()), val);
    }
}

BOOST_AUTO_TEST_SUITE_END()
