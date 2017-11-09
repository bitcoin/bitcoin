// Copyright (c) 2016-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "stat.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(stat_tests, BasicTestingSetup)
BOOST_AUTO_TEST_CASE(stat_testvectors)
{
    // const int numMetrics = 5;
    CStatHistory<int> *s1 = new CStatHistory<int>("s1");
    BOOST_CHECK((*s1)() == 0);
    CStatHistory<unsigned int> *s2 = new CStatHistory<unsigned int>("s2", STAT_OP_AVE);
    BOOST_CHECK((*s2)() == 0);
    CStatHistory<uint64_t> *s3 = new CStatHistory<uint64_t>("s3", STAT_OP_MAX);
    BOOST_CHECK((*s3)() == 0);
    CStatHistory<double> *s4 = new CStatHistory<double>("s4", STAT_OP_MIN);
    BOOST_CHECK((*s4)() == 0.0);

    CStatHistory<float, MinValMax<float> > *s5 = new CStatHistory<float, MinValMax<float> >("s5");

    (*s5) += 4.5f;
    BOOST_CHECK((*s5)().min == 4.5f);
    BOOST_CHECK((*s5)().max == 4.5f);
    BOOST_CHECK((*s5)().val == 4.5f);
    (*s5) << 1.5f;
    BOOST_CHECK((*s5)().min == 4.5f);
    BOOST_CHECK((*s5)().max == 6.0f);
    BOOST_CHECK((*s5)().val == 6.0f);

    // check the + over various data types
    (*s1) += 5;
    BOOST_CHECK((*s1)() == 5);
    (*s2) += 10;
    BOOST_CHECK((*s2)() == 10);
    (*s3) += 10000;
    BOOST_CHECK((*s3)() == 10000);
    (*s4) += 3.3;
    BOOST_CHECK((*s4)() == 3.3);

    (*s1) += 15;
    BOOST_CHECK((*s1)() == 20);
    (*s2) += 110;
    BOOST_CHECK((*s2)() == 120);
    (*s3) += 110000;
    BOOST_CHECK((*s3)() == 120000);
    (*s4) += 4.3;
    BOOST_CHECK((*s4)() == 7.6);

    // Make the boost callback huge so it never happens.  I call timeout myself
    statMinInterval = std::chrono::milliseconds(5000);

    s1->timeout(boost::system::error_code());
    s2->timeout(boost::system::error_code());
    s3->timeout(boost::system::error_code());
    s4->timeout(boost::system::error_code());
    s5->timeout(boost::system::error_code());

    BOOST_CHECK((*s1)() == 0);
    for (int i = 0; i < 30; i++)
    {
        (*s1) += 5;
        (*s2) += 10;
        (*s3) += 10000;
        (*s4) += 3.3;
        (*s5) << 2.4;
        s1->timeout(boost::system::error_code());
        s2->timeout(boost::system::error_code());
        s3->timeout(boost::system::error_code());
        s4->timeout(boost::system::error_code());
        s5->timeout(boost::system::error_code());
    }
    BOOST_CHECK(s1->History(1, 0) == 29 * 5 + 20);
    BOOST_CHECK(s2->History(1, 0) == (29 * 10 + 120) / 30);
    BOOST_CHECK(s3->History(1, 0) == 120000);
    BOOST_CHECK(s4->History(1, 0) == 3.3);

    // statMinInterval=std::chrono::milliseconds(30); // Speed things up
    for (int i = 0; i < 12; i++)
        for (int j = 0; j < 30; j++)
        {
            (*s1) += 5;
            (*s2) += 10;
            (*s3) += 10000;
            (*s4) += 3.3;
            (*s5) << 2.4;
            s1->timeout(boost::system::error_code());
            s2->timeout(boost::system::error_code());
            s3->timeout(boost::system::error_code());
            s4->timeout(boost::system::error_code());
            s5->timeout(boost::system::error_code());
        }
    BOOST_CHECK(s1->History(2, 0) > 0);
    BOOST_CHECK(s2->History(2, 0) > 0);
    BOOST_CHECK(s3->History(2, 0) == 120000);
    BOOST_CHECK(s4->History(2, 0) == 3.3);

    int array[15];

    BOOST_CHECK(s1->Series(0, array, 15) == 15);
    for (int i = 0; i < 15; i++)
        BOOST_CHECK(array[i] == 5);

    delete s1;
    delete s2;
    delete s3;
    delete s4;
    delete s5;
}

BOOST_AUTO_TEST_CASE(stat_empty_construct)
{
    {
        /*! Create non-zero CStatHistory object and destroy it again.
          This hopefully primes the same memory for the test below to be
          non-zero, so the (formerly) missing initialization can be caught.
        */
        CStatHistory<uint64_t> *stats = new CStatHistory<uint64_t>("name");
        (*stats) += 0x5555555555555555UL;
        BOOST_CHECK((*stats)() == 0x5555555555555555UL);
        delete stats;
    }
    {
        CStatHistory<uint64_t> *stats = new CStatHistory<uint64_t>();
        // check that default constructor zeroes as well
        BOOST_CHECK((*stats)() == 0UL);
        delete stats;
    }
}

BOOST_AUTO_TEST_SUITE_END()
