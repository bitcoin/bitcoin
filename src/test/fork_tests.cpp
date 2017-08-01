// Copyright (c) 2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "addrman.h"
#include "test/test_bitcoin.h"
#include <string>
#include <boost/test/unit_test.hpp>
#include "hash.h"
#include "serialize.h"
#include "streams.h"
#include "net.h"
#include "chainparams.h"


BOOST_FIXTURE_TEST_SUITE(fork_test, TestingSetup)

// must be >= 12
#define SZ 20

BOOST_AUTO_TEST_CASE(fork_trigger)
{
    CChain test;

    CBlockIndex bi[SZ];

    for (int i=0;i<SZ;i++)
    {
        if (i) bi[i].pprev = &bi[i-1];
        bi[i].nHeight = i;
        bi[i].nTime = i;
    }

    for(int i=0;i<SZ;i++)
    {
        BOOST_CHECK(bi[i].forkAtNextBlock(SZ+1)==false);
        BOOST_CHECK(bi[i].forkActivated(SZ+1)==false);
        BOOST_CHECK(bi[i].forkActivateNow(SZ+1)==false);
    }

    for(int i=0;i<SZ;i++)
    {
        BOOST_CHECK(bi[i].forkAtNextBlock(0)==false);
        BOOST_CHECK(bi[i].forkActivated(0)==false);
        BOOST_CHECK(bi[i].forkActivateNow(0)==false);
    }

    BOOST_CHECK(bi[SZ-1].forkActivated(1) == true);
    BOOST_CHECK(bi[SZ-1].forkAtNextBlock(1) == false);

    BOOST_CHECK(bi[SZ-1].forkAtNextBlock(SZ-6) == true);
    BOOST_CHECK(bi[SZ-1].forkAtNextBlock(SZ-5) == false);

}

BOOST_AUTO_TEST_SUITE_END()
