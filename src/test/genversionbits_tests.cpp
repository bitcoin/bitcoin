// Copyright (c) 2017 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chain.h"
#include "random.h"
#include "versionbits.h"
#include "test/test_bitcoin.h"
#include "test/test_random.h"
#include "chainparams.h"
#include "chainparamsbase.h"
//#include "main.h"  // bip135 obsolete
#include "validation.h"
#include "consensus/params.h"

#include <boost/test/unit_test.hpp>

/* Define a virtual block time, one block per 10 minutes after Nov 14 2014, 0:55:36am */
int32_t GenVBTestTime(int nHeight) { return 1415926536 + 600 * nHeight; }

#define TEST_BIT 8

#define SELECTED_CHAIN CBaseChainParams::MAIN


// a checker which enforced some minlockedblocks
class TestConditionCheckerMinBlocks : public AbstractThresholdConditionChecker
{
private:
    mutable ThresholdConditionCache cache;

protected:
    int64_t BeginTime(const Consensus::Params& params) const { return GenVBTestTime(10000); }
    int64_t EndTime(const Consensus::Params& params) const { return GenVBTestTime(20000); }
    int Period(const Consensus::Params& params) const { return 1000; }
    int Threshold(const Consensus::Params& params) const { return 900; }
    int MinLockedBlocks(const Consensus::Params& params) const { return 1001; }
    int64_t MinLockedTime(const Consensus::Params& params) const { return 0; }

    // test for fork bit
    bool Condition(const CBlockIndex* pindex, const Consensus::Params& params) const { return (pindex->nVersion & (1 << TEST_BIT)); }

public:
    ThresholdState GetState(const CBlockIndex* pindexPrev) const {
        return AbstractThresholdConditionChecker::GetStateFor(pindexPrev, ModifiableParams().GetModifiableConsensus(), cache);
    }
};

// a checker which enforced some minlockedtime
class TestConditionCheckerMinTime : public AbstractThresholdConditionChecker
{
private:
    mutable ThresholdConditionCache cache;

protected:
    int64_t BeginTime(const Consensus::Params& params) const { return GenVBTestTime(10000); }
    int64_t EndTime(const Consensus::Params& params) const { return GenVBTestTime(20000); }
    int Period(const Consensus::Params& params) const { return 1000; }
    int Threshold(const Consensus::Params& params) const { return 900; }
    int MinLockedBlocks(const Consensus::Params& params) const { return 0; }
    int64_t MinLockedTime(const Consensus::Params& params) const { return 5000 * 600; }  // this is a minimum *duration* after lock-in time

    // test for fork bit
    bool Condition(const CBlockIndex* pindex, const Consensus::Params& params) const { return (pindex->nVersion & (1 << TEST_BIT)); }

public:
    ThresholdState GetState(const CBlockIndex* pindexPrev) const {
        return AbstractThresholdConditionChecker::GetStateFor(pindexPrev, ModifiableParams().GetModifiableConsensus(), cache);
    }
};


#define CHECKERS 6

class GenVersionBitsTesterMinBlocks
{
public:
    // A fake blockchain
    std::vector<CBlockIndex*> vpblock;

    // 6 independent checkers for the same bit.
    // The first one performs all checks, the second only 50%, the third only 25%, etc...
    // This is to test whether lack of cached information leads to the same results.
    TestConditionCheckerMinBlocks checker[CHECKERS];

    // Test counter (to identify failures)
    int num;

    GenVersionBitsTesterMinBlocks() : num(0) {}

    GenVersionBitsTesterMinBlocks& Reset() {
        for (unsigned int i = 0; i < vpblock.size(); i++) {
            delete vpblock[i];
        }
        for (unsigned int i = 0; i < CHECKERS; i++) {
            checker[i] = TestConditionCheckerMinBlocks();
        }
        vpblock.clear();
        return *this;
    }

    ~GenVersionBitsTesterMinBlocks() {
         Reset();
    }

    /**
     * Create fake blocks in the test chain
     * @param height up to which to mine
     * @param nTime this time to enter in mined block
     * @param nVersion block versions are set to this
     */
    GenVersionBitsTesterMinBlocks& Mine(unsigned int height, int32_t nTime, int32_t nVersion) {
        while (vpblock.size() < height) {
            CBlockIndex* pindex = new CBlockIndex();
            pindex->nHeight = vpblock.size();
            pindex->pprev = vpblock.size() > 0 ? vpblock.back() : NULL;
            pindex->nTime = nTime;
            pindex->nVersion = nVersion;
            pindex->BuildSkip();
            vpblock.push_back(pindex);
        }
        return *this;
    }

    GenVersionBitsTesterMinBlocks& TestDefined() {
        for (int i = 0; i < CHECKERS; i++) {
            if ((insecure_rand() & ((1 << i) - 1)) == 0) {
                BOOST_CHECK_MESSAGE(checker[i].GetState(vpblock.empty() ? NULL : vpblock.back()) == THRESHOLD_DEFINED, strprintf("Test %i for DEFINED", num));
            }
        }
        num++;
        return *this;
    }

    GenVersionBitsTesterMinBlocks& TestStarted() {
        for (int i = 0; i < CHECKERS; i++) {
            if ((insecure_rand() & ((1 << i) - 1)) == 0) {
                BOOST_CHECK_MESSAGE(checker[i].GetState(vpblock.empty() ? NULL : vpblock.back()) == THRESHOLD_STARTED, strprintf("Test %i for STARTED", num));
            }
        }
        num++;
        return *this;
    }

    GenVersionBitsTesterMinBlocks& TestLockedIn() {
        for (int i = 0; i < CHECKERS; i++) {
            if ((insecure_rand() & ((1 << i) - 1)) == 0) {
                BOOST_CHECK_MESSAGE(checker[i].GetState(vpblock.empty() ? NULL : vpblock.back()) == THRESHOLD_LOCKED_IN, strprintf("Test %i for LOCKED_IN", num));
            }
        }
        num++;
        return *this;
    }

    GenVersionBitsTesterMinBlocks& TestActive() {
        for (int i = 0; i < CHECKERS; i++) {
            if ((insecure_rand() & ((1 << i) - 1)) == 0) {
                BOOST_CHECK_MESSAGE(checker[i].GetState(vpblock.empty() ? NULL : vpblock.back()) == THRESHOLD_ACTIVE, strprintf("Test %i for ACTIVE", num));
            }
        }
        num++;
        return *this;
    }
    GenVersionBitsTesterMinBlocks& TestFailed() {
        for (int i = 0; i < CHECKERS; i++) {
            if ((insecure_rand() & ((1 << i) - 1)) == 0) {
                BOOST_CHECK_MESSAGE(checker[i].GetState(vpblock.empty() ? NULL : vpblock.back()) == THRESHOLD_FAILED, strprintf("Test %i for FAILED", num));
            }
        }
        num++;
        return *this;
    }

    CBlockIndex * Tip() { return vpblock.size() ? vpblock.back() : NULL; }
};


class GenVersionBitsTesterMinTime
{
public:
    // A fake blockchain
    std::vector<CBlockIndex*> vpblock;

    // 6 independent checkers for the same bit.
    // The first one performs all checks, the second only 50%, the third only 25%, etc...
    // This is to test whether lack of cached information leads to the same results.
    TestConditionCheckerMinTime checker[CHECKERS];

    // Test counter (to identify failures)
    int num;

    GenVersionBitsTesterMinTime() : num(0) {}

    GenVersionBitsTesterMinTime& Reset() {
        for (unsigned int i = 0; i < vpblock.size(); i++) {
            delete vpblock[i];
        }
        for (unsigned int i = 0; i < CHECKERS; i++) {
            checker[i] = TestConditionCheckerMinTime();
        }
        vpblock.clear();
        return *this;
    }

    ~GenVersionBitsTesterMinTime() {
         Reset();
    }

    /**
     * Create fake blocks in the test chain
     * @param height up to which to mine
     * @param nTime this time to enter in mined block
     * @param nVersion block versions are set to this
     */
    GenVersionBitsTesterMinTime& Mine(unsigned int height, int32_t nTime, int32_t nVersion) {
        while (vpblock.size() < height) {
            CBlockIndex* pindex = new CBlockIndex();
            pindex->nHeight = vpblock.size();
            pindex->pprev = vpblock.size() > 0 ? vpblock.back() : NULL;
            pindex->nTime = nTime;
            pindex->nVersion = nVersion;
            pindex->BuildSkip();
            vpblock.push_back(pindex);
        }
        return *this;
    }

    GenVersionBitsTesterMinTime& TestDefined() {
        for (int i = 0; i < CHECKERS; i++) {
            if ((insecure_rand() & ((1 << i) - 1)) == 0) {
                BOOST_CHECK_MESSAGE(checker[i].GetState(vpblock.empty() ? NULL : vpblock.back()) == THRESHOLD_DEFINED, strprintf("Test %i for DEFINED", num));
            }
        }
        num++;
        return *this;
    }

    GenVersionBitsTesterMinTime& TestStarted() {
        for (int i = 0; i < CHECKERS; i++) {
            if ((insecure_rand() & ((1 << i) - 1)) == 0) {
                BOOST_CHECK_MESSAGE(checker[i].GetState(vpblock.empty() ? NULL : vpblock.back()) == THRESHOLD_STARTED, strprintf("Test %i for STARTED", num));
            }
        }
        num++;
        return *this;
    }

    GenVersionBitsTesterMinTime& TestLockedIn() {
        for (int i = 0; i < CHECKERS; i++) {
            if ((insecure_rand() & ((1 << i) - 1)) == 0) {
                BOOST_CHECK_MESSAGE(checker[i].GetState(vpblock.empty() ? NULL : vpblock.back()) == THRESHOLD_LOCKED_IN, strprintf("Test %i for LOCKED_IN", num));
            }
        }
        num++;
        return *this;
    }

    GenVersionBitsTesterMinTime& TestActive() {
        for (int i = 0; i < CHECKERS; i++) {
            if ((insecure_rand() & ((1 << i) - 1)) == 0) {
                BOOST_CHECK_MESSAGE(checker[i].GetState(vpblock.empty() ? NULL : vpblock.back()) == THRESHOLD_ACTIVE, strprintf("Test %i for ACTIVE", num));
            }
        }
        num++;
        return *this;
    }
    GenVersionBitsTesterMinTime& TestFailed() {
        for (int i = 0; i < CHECKERS; i++) {
            if ((insecure_rand() & ((1 << i) - 1)) == 0) {
                BOOST_CHECK_MESSAGE(checker[i].GetState(vpblock.empty() ? NULL : vpblock.back()) == THRESHOLD_FAILED, strprintf("Test %i for FAILED", num));
            }
        }
        num++;
        return *this;
    }

    CBlockIndex * Tip() { return vpblock.size() ? vpblock.back() : NULL; }
};


BOOST_FIXTURE_TEST_SUITE(genversionbits_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(genversionbits_minblocks_test)
{
    // NOTE: the index i is not used in this loop. The high number of iterations
    // is probably because the testing is probabilistic in terms of the checkers
    // finding an error, and they wanted to cover the bases.
    for (int i = 0; i < 64; i++) {
        // DEFINED -> FAILED
        GenVersionBitsTesterMinBlocks().TestDefined()
                           .Mine(1, GenVBTestTime(1), 0x100).TestDefined()
                           .Mine(11, GenVBTestTime(11), 0x100).TestDefined()
                           .Mine(989, GenVBTestTime(989), 0x100).TestDefined()
                           .Mine(999, GenVBTestTime(20000), 0x100).TestDefined()
                           .Mine(1000, GenVBTestTime(20000), 0x100).TestFailed()
                           .Mine(1999, GenVBTestTime(30001), 0x100).TestFailed()
                           .Mine(2000, GenVBTestTime(30002), 0x100).TestFailed()
                           .Mine(2001, GenVBTestTime(30003), 0x100).TestFailed()
                           .Mine(2999, GenVBTestTime(30004), 0x100).TestFailed()
                           .Mine(3000, GenVBTestTime(30005), 0x100).TestFailed()

        // DEFINED -> STARTED -> FAILED
                           .Reset().TestDefined()
                           .Mine(1, GenVBTestTime(1), 0).TestDefined()
                           .Mine(1000, GenVBTestTime(10000) - 1, 0x100).TestDefined() // One second more and it would be defined
                           .Mine(2000, GenVBTestTime(10000), 0x100).TestStarted() // So that's what happens the next period
                           .Mine(2051, GenVBTestTime(10010), 0).TestStarted() // 51 old blocks
                           .Mine(2950, GenVBTestTime(10020), 0x100).TestStarted() // 899 new blocks
                           .Mine(3000, GenVBTestTime(20000), 0).TestFailed() // 50 old blocks (so 899 out of the past 1000)
                           .Mine(4000, GenVBTestTime(20010), 0x100).TestFailed()

        // DEFINED -> STARTED -> FAILED while threshold reached
                           .Reset().TestDefined()
                           .Mine(1, GenVBTestTime(1), 0).TestDefined()
                           .Mine(1000, GenVBTestTime(10000) - 1, 0x101).TestDefined() // One second more and it would be defined
                           .Mine(2000, GenVBTestTime(10000), 0x101).TestStarted() // So that's what happens the next period
                           .Mine(2999, GenVBTestTime(30000), 0x100).TestStarted() // 999 new blocks
                           .Mine(3000, GenVBTestTime(30000), 0x100).TestFailed() // 1 new block (so 1000 out of the past 1000 are new)
                           .Mine(3999, GenVBTestTime(30001), 0).TestFailed()
                           .Mine(4000, GenVBTestTime(30002), 0).TestFailed()
                           .Mine(14333, GenVBTestTime(30003), 0).TestFailed()
                           .Mine(24000, GenVBTestTime(40000), 0).TestFailed()

        // DEFINED -> STARTED -> LOCKEDIN at the last minute -> ACTIVE
                           .Reset().TestDefined()
                           .Mine(1, GenVBTestTime(1), 0).TestDefined()
                           .Mine(1000, GenVBTestTime(10000) - 1, 0x101).TestDefined() // One second more and it would be defined
                           .Mine(2000, GenVBTestTime(10000), 0x101).TestStarted() // So that's what happens the next period
                           .Mine(2050, GenVBTestTime(10010), 0x200).TestStarted() // 50 old blocks
                           .Mine(2950, GenVBTestTime(10020), 0x100).TestStarted() // 900 new blocks
                           .Mine(2999, GenVBTestTime(19999), 0x200).TestStarted() // 49 old blocks
                           .Mine(3000, GenVBTestTime(20000), 0x200).TestLockedIn() // 1 old block (so 900 out of the past 1000)
                           .Mine(3001, GenVBTestTime(20001), 0).TestLockedIn() // still locked in
                           .Mine(3999, GenVBTestTime(30000), 0).TestLockedIn() // wait at least minlockedblocks = 1001, so not active at next
                           .Mine(4000, GenVBTestTime(30001), 0).TestLockedIn() // need to remain locked in until next sync at 5000
                           .Mine(4999, GenVBTestTime(30002), 0).TestLockedIn() // last locked in block
                           .Mine(5000, GenVBTestTime(30002), 0).TestActive()
                           .Mine(24000, GenVBTestTime(40000), 0).TestActive();
    }
}


BOOST_AUTO_TEST_CASE(genversionbits_mintime_test)
{
    for (int i = 0; i < 64; i++) {
        // DEFINED -> FAILED
        GenVersionBitsTesterMinTime().TestDefined()
                           .Mine(1, GenVBTestTime(1), 0x100).TestDefined()
                           .Mine(11, GenVBTestTime(11), 0x100).TestDefined()
                           .Mine(989, GenVBTestTime(989), 0x100).TestDefined()
                           .Mine(999, GenVBTestTime(20000), 0x100).TestDefined()
                           .Mine(1000, GenVBTestTime(20000), 0x100).TestFailed()
                           .Mine(1999, GenVBTestTime(30001), 0x100).TestFailed()
                           .Mine(2000, GenVBTestTime(30002), 0x100).TestFailed()
                           .Mine(2001, GenVBTestTime(30003), 0x100).TestFailed()
                           .Mine(2999, GenVBTestTime(30004), 0x100).TestFailed()
                           .Mine(3000, GenVBTestTime(30005), 0x100).TestFailed()

        // DEFINED -> STARTED -> FAILED
                           .Reset().TestDefined()
                           .Mine(1, GenVBTestTime(1), 0).TestDefined()
                           .Mine(1000, GenVBTestTime(10000) - 1, 0x100).TestDefined() // One second more and it would be defined
                           .Mine(2000, GenVBTestTime(10000), 0x100).TestStarted() // So that's what happens the next period
                           .Mine(2051, GenVBTestTime(10010), 0).TestStarted() // 51 old blocks
                           .Mine(2950, GenVBTestTime(10020), 0x100).TestStarted() // 899 new blocks
                           .Mine(3000, GenVBTestTime(20000), 0).TestFailed() // 50 old blocks (so 899 out of the past 1000)
                           .Mine(4000, GenVBTestTime(20010), 0x100).TestFailed()

        // DEFINED -> STARTED -> FAILED while threshold reached
                           .Reset().TestDefined()
                           .Mine(1, GenVBTestTime(1), 0).TestDefined()
                           .Mine(1000, GenVBTestTime(10000) - 1, 0x101).TestDefined() // One second more and it would be defined
                           .Mine(2000, GenVBTestTime(10000), 0x101).TestStarted() // So that's what happens the next period
                           .Mine(2999, GenVBTestTime(30000), 0x100).TestStarted() // 999 new blocks
                           .Mine(3000, GenVBTestTime(30000), 0x100).TestFailed() // 1 new block (so 1000 out of the past 1000 are new)
                           .Mine(3999, GenVBTestTime(30001), 0).TestFailed()
                           .Mine(4000, GenVBTestTime(30002), 0).TestFailed()
                           .Mine(14333, GenVBTestTime(30003), 0).TestFailed()
                           .Mine(24000, GenVBTestTime(40000), 0).TestFailed()

        // DEFINED -> STARTED -> LOCKEDIN at the last minute -> ACTIVE
                           .Reset().TestDefined()
                           .Mine(1, GenVBTestTime(1), 0).TestDefined()
                           .Mine(1000, GenVBTestTime(10000) - 1, 0x101).TestDefined() // One second more and it would be defined
                           .Mine(2000, GenVBTestTime(10000), 0x101).TestStarted() // So that's what happens the next period
                           .Mine(2050, GenVBTestTime(10010), 0x200).TestStarted() // 50 old blocks
                           .Mine(2950, GenVBTestTime(10020), 0x100).TestStarted() // 900 new blocks
                           .Mine(2999, GenVBTestTime(19999), 0x200).TestStarted() // 49 old blocks. Actual lock-in time will be 19999
                           .Mine(3000, GenVBTestTime(20000), 0x200).TestLockedIn() // 1 old block (so 900 out of the past 1000)
                           .Mine(3001, GenVBTestTime(20001), 0).TestLockedIn() // still locked in
                           .Mine(4000, GenVBTestTime(21000), 0).TestLockedIn() // need to remain locked at least 5000s after lock-in time @ 20000s
                           .Mine(8000, GenVBTestTime(22002), 0).TestLockedIn() // still locked in more than 5000 blocks later if time not passed
                           .Mine(9000, GenVBTestTime(24998), 0).TestLockedIn() // 1 sec prior to minimum lock-in duration - still remain locked in
                           .Mine(9001, GenVBTestTime(24999), 0).TestLockedIn()
                           .Mine(9999, GenVBTestTime(24999), 0).TestLockedIn()
                           .Mine(10000, GenVBTestTime(24999), 0).TestActive(); // activate at 10000 after full period of times > lock-in + 5000
    }
}

BOOST_AUTO_TEST_SUITE_END()
