// Copyright (c) 2013-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "thinblock.h"
#include "net.h"
#include "test/test_bitcoin.h"
#include <assert.h>
#include <boost/range/irange.hpp>
#include <boost/test/unit_test.hpp>
#include <limits>
#include <string>
#include <vector>

using namespace std;

CService ipaddress3(uint32_t i, uint32_t port)
{
    struct in_addr s;
    s.s_addr = i;
    return CService(CNetAddr(s), port);
}

class TestTBD : public CThinBlockData
{
protected:
    virtual int64_t getTimeForStats() { return times[min(times_idx++, times.size() - 1)]; }
    vector<int64_t> times;
    size_t times_idx;

public:
    TestTBD(const vector<int64_t> &_times)
    {
        assert(_times.size() > 0);
        times = _times;
        times_idx = 0;
    }
    void resetTimeIdx() { times_idx = 0; }
};


BOOST_FIXTURE_TEST_SUITE(thinblock_data_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_thinblock_byte_tracking)
{
    CThinBlockData thindata;

    /**
     * Do calcuations for single peer building a thinblock
     */

    CAddress addr1(ipaddress3(0xa0b0c001, 10000));
    CNode dummyNode1(INVALID_SOCKET, addr1, "", true);

    thindata.ResetThinBlockBytes();
    BOOST_CHECK(0 == thindata.GetThinBlockBytes());
    BOOST_CHECK(0 == dummyNode1.nLocalThinBlockBytes);

    thindata.AddThinBlockBytes(0, &dummyNode1);
    BOOST_CHECK(0 == thindata.GetThinBlockBytes());
    BOOST_CHECK(0 == dummyNode1.nLocalThinBlockBytes);

    thindata.AddThinBlockBytes(1000, &dummyNode1);
    BOOST_CHECK(1000 == thindata.GetThinBlockBytes());
    BOOST_CHECK(1000 == dummyNode1.nLocalThinBlockBytes);

    thindata.AddThinBlockBytes(449932, &dummyNode1);
    BOOST_CHECK(450932 == thindata.GetThinBlockBytes());
    BOOST_CHECK(450932 == dummyNode1.nLocalThinBlockBytes);

    thindata.DeleteThinBlockBytes(0, &dummyNode1);
    BOOST_CHECK(450932 == thindata.GetThinBlockBytes());
    BOOST_CHECK(450932 == dummyNode1.nLocalThinBlockBytes);

    thindata.DeleteThinBlockBytes(1, &dummyNode1);
    BOOST_CHECK(450931 == thindata.GetThinBlockBytes());
    BOOST_CHECK(450931 == dummyNode1.nLocalThinBlockBytes);

    thindata.DeleteThinBlockBytes(13939, &dummyNode1);
    BOOST_CHECK(436992 == thindata.GetThinBlockBytes());
    BOOST_CHECK(436992 == dummyNode1.nLocalThinBlockBytes);

    // Try to delete more bytes than we already have tracked.  This should not be possible...we don't allow this
    // to happen in the event that we get an incorrect or invalid value returned for the dynamic memory usage of
    // a transaction.  This could then be used in a theoretical attack by resetting total byte usage to zero while
    // continuing to build more thinblocks.
    thindata.DeleteThinBlockBytes(436993, &dummyNode1);
    BOOST_CHECK_MESSAGE(436992 == thindata.GetThinBlockBytes(), "nThinBlockBytes is " << thindata.GetThinBlockBytes());
    BOOST_CHECK_MESSAGE(
        436992 == dummyNode1.nLocalThinBlockBytes, "nLocalThinBlockBytes is " << dummyNode1.nLocalThinBlockBytes);


    /**
     * Add a second peer and do more calcuations for building a second thinblock
     */

    CAddress addr2(ipaddress3(0xa0b0c002, 10000));
    CNode dummyNode2(INVALID_SOCKET, addr2, "", true);

    thindata.AddThinBlockBytes(1000, &dummyNode2);
    BOOST_CHECK(437992 == thindata.GetThinBlockBytes());
    BOOST_CHECK(436992 == dummyNode1.nLocalThinBlockBytes);
    BOOST_CHECK(1000 == dummyNode2.nLocalThinBlockBytes);

    thindata.DeleteThinBlockBytes(0, &dummyNode2);
    BOOST_CHECK(437992 == thindata.GetThinBlockBytes());
    BOOST_CHECK(1000 == dummyNode2.nLocalThinBlockBytes);

    thindata.DeleteThinBlockBytes(1, &dummyNode2);
    BOOST_CHECK(437991 == thindata.GetThinBlockBytes());
    BOOST_CHECK(436992 == dummyNode1.nLocalThinBlockBytes);
    BOOST_CHECK(999 == dummyNode2.nLocalThinBlockBytes);

    thindata.DeleteThinBlockBytes(999, &dummyNode2);
    BOOST_CHECK(436992 == thindata.GetThinBlockBytes());
    BOOST_CHECK(436992 == dummyNode1.nLocalThinBlockBytes);
    BOOST_CHECK(0 == dummyNode2.nLocalThinBlockBytes);

    // now finally reset everything
    thindata.ResetThinBlockBytes();
    BOOST_CHECK_MESSAGE(0 == thindata.GetThinBlockBytes(), "nThinBlockBytes is " << thindata.GetThinBlockBytes());
}

BOOST_AUTO_TEST_CASE(test_thinblockdata_stats1)
{
    vector<int64_t> times1(1000); // minutes

    for (uint32_t i = 0; i < times1.size(); i++)
    {
        times1[i] = 1000 * 60 * i;
    }

    {
        TestTBD tbd(times1);
        // exercise summary methods on empty arrays to make sure they don't fail
        // in weird ways
        tbd.ToString();
        tbd.InBoundPercentToString();
        tbd.OutBoundPercentToString();
        tbd.InBoundBloomFiltersToString();
        tbd.OutBoundBloomFiltersToString();
        tbd.ResponseTimeToString();
        tbd.ValidationTimeToString();
        tbd.ReRequestedTxToString();
        tbd.MempoolLimiterBytesSavedToString();
        tbd.GetThinBlockBytes();
    }

    {
        TestTBD tbd(times1);

        for (int64_t i : boost::irange(0, 100))
            tbd.UpdateInBound(i, 3 * i);

        string res = tbd.InBoundPercentToString();

        BOOST_CHECK_MESSAGE(res.find("66.7%") != string::npos, "InBoundPercentToString() is " << res);
    }

    {
        TestTBD tbd(times1);

        for (int64_t i : boost::irange(0, 100))
            tbd.UpdateOutBound(i, 3 * i);

        string res = tbd.OutBoundPercentToString();

        BOOST_CHECK_MESSAGE(res.find("66.7%") != string::npos, "OutBoundPercentToString() is " << res);
    }

    {
        TestTBD tbd(times1);

        for (int64_t i : boost::irange(0, 100))
            tbd.UpdateInBoundBloomFilter(1000 * i);

        string res = tbd.InBoundBloomFiltersToString();

        BOOST_CHECK_MESSAGE(res.find("49.50KB") != string::npos, "InBoundBloomFiltersToString() is " << res);
    }

    {
        TestTBD tbd(times1);

        for (int64_t i : boost::irange(0, 100))
            tbd.UpdateOutBoundBloomFilter(1000 * i);

        string res = tbd.OutBoundBloomFiltersToString();
        BOOST_CHECK_MESSAGE(res.find("49.50KB") != string::npos, "OutBoundBloomFiltersToString() is " << res);
    }
    // FIXME: check others somehow that depend on chain sync state

    {
        TestTBD tbd(times1);

        for (int64_t i : boost::irange(0, 100))
            tbd.UpdateInBoundReRequestedTx(1000 * i);

        string res = tbd.ReRequestedTxToString();
        BOOST_CHECK_MESSAGE(res.find(":100") != string::npos, "ReRequestedTxToString() is " << res);
    }

    {
        TestTBD tbd(times1);

        for (int64_t i : boost::irange(0, 100))
            tbd.UpdateMempoolLimiterBytesSaved(1000 * i);

        string res = tbd.MempoolLimiterBytesSavedToString();
        BOOST_CHECK_MESSAGE(res.find("4.95MB") != string::npos, "MempoolLimiterBytesSavedToString() is " << res);
    }
}

BOOST_AUTO_TEST_SUITE_END()
