// Copyright (c) 2014-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <versionbits.h>

#include <boost/test/unit_test.hpp>

/* Define a virtual block time, one block per 10 minutes after Nov 14 2014, 0:55:36am */
static int32_t TestTime(int nHeight) { return 1415926536 + 600 * nHeight; }

static const std::string StateName(ThresholdState state)
{
    switch (state) {
    case ThresholdState::DEFINED:   return "DEFINED";
    case ThresholdState::STARTED:   return "STARTED";
    case ThresholdState::LOCKED_IN: return "LOCKED_IN";
    case ThresholdState::ACTIVE:    return "ACTIVE";
    case ThresholdState::FAILED:    return "FAILED";
    } // no default case, so the compiler can warn about missing cases
    return "";
}

static const Consensus::Params paramsDummy = Consensus::Params();

class TestConditionChecker : public AbstractThresholdConditionChecker
{
private:
    mutable ThresholdConditionCache cache;

public:
    int StartHeight(const Consensus::Params& params) const override { return 100; }
    int TimeoutHeight(const Consensus::Params& params) const override { return 200; }
    int Period(const Consensus::Params& params) const override { return 10; }
    int Threshold(const Consensus::Params& params) const override { return 9; }
    int MinActivationHeight(const Consensus::Params& params) const override { return 0; }
    bool Condition(const CBlockIndex* pindex, const Consensus::Params& params) const override { return (pindex->nVersion & 0x100); }

    ThresholdState GetStateFor(const CBlockIndex* pindexPrev) const { return AbstractThresholdConditionChecker::GetStateFor(pindexPrev, paramsDummy, cache); }
    int GetStateSinceHeightFor(const CBlockIndex* pindexPrev) const { return AbstractThresholdConditionChecker::GetStateSinceHeightFor(pindexPrev, paramsDummy, cache); }
};

class TestDelayedActivationConditionChecker : public TestConditionChecker
{
public:
    int MinActivationHeight(const Consensus::Params& params) const override { return 250; }
};

class TestAlwaysActiveConditionChecker : public TestConditionChecker
{
public:
    int StartHeight(const Consensus::Params& params) const override { return Consensus::BIP9Deployment::ALWAYS_ACTIVE; }
};

class TestNeverActiveConditionChecker : public TestConditionChecker
{
public:
    int StartHeight(const Consensus::Params& params) const override { return Consensus::BIP9Deployment::NEVER_ACTIVE; }
    int TimeoutHeight(const Consensus::Params& params) const override { return Consensus::BIP9Deployment::NEVER_ACTIVE; }
};

#define CHECKERS 6

class VersionBitsTester
{
    // A fake blockchain
    std::vector<CBlockIndex*> vpblock;

    // 6 independent checkers for the same bit.
    // The first one performs all checks, the second only 50%, the third only 25%, etc...
    // This is to test whether lack of cached information leads to the same results.
    TestConditionChecker checker[CHECKERS];
    // Another 6 that assume delayed activation
    TestDelayedActivationConditionChecker checker_delayed[CHECKERS];
    // Another 6 that assume always active activation
    TestAlwaysActiveConditionChecker checker_always[CHECKERS];
    // Another 6 that assume never active activation
    TestNeverActiveConditionChecker checker_never[CHECKERS];

    // Test counter (to identify failures)
    int num;

public:
    VersionBitsTester() : num(1000) {}

    VersionBitsTester& Reset() {
        num = num - (num % 1000) + 1000;
        for (unsigned int i = 0; i < vpblock.size(); i++) {
            delete vpblock[i];
        }
        for (unsigned int  i = 0; i < CHECKERS; i++) {
            checker[i] = TestConditionChecker();
            checker_delayed[i] = TestDelayedActivationConditionChecker();
            checker_always[i] = TestAlwaysActiveConditionChecker();
            checker_never[i] = TestNeverActiveConditionChecker();
        }
        vpblock.clear();
        return *this;
    }

    ~VersionBitsTester() {
         Reset();
    }

    VersionBitsTester& Mine(unsigned int height, int32_t nTime, int32_t nVersion) {
        while (vpblock.size() < height) {
            CBlockIndex* pindex = new CBlockIndex();
            pindex->nHeight = vpblock.size();
            pindex->pprev = vpblock.size() > 0 ? vpblock.back() : nullptr;
            pindex->nTime = nTime;
            pindex->nVersion = nVersion;
            pindex->BuildSkip();
            vpblock.push_back(pindex);
        }
        return *this;
    }

    VersionBitsTester& TestStateSinceHeight(int height)
    {
        return TestStateSinceHeight(height, height);
    }

    VersionBitsTester& TestStateSinceHeight(int height, int height_delayed)
    {
        const CBlockIndex* pindex = vpblock.empty() ? nullptr : vpblock.back();
        for (int i = 0; i < CHECKERS; i++) {
            if (InsecureRandBits(i) == 0) {
                BOOST_CHECK_MESSAGE(checker[i].GetStateSinceHeightFor(pindex) == height, strprintf("Test %i for StateSinceHeight", num));
                BOOST_CHECK_MESSAGE(checker_delayed[i].GetStateSinceHeightFor(pindex) == height_delayed, strprintf("Test %i for StateSinceHeight (delayed)", num));
                BOOST_CHECK_MESSAGE(checker_always[i].GetStateSinceHeightFor(pindex) == 0, strprintf("Test %i for StateSinceHeight (always active)", num));

                // never active may go from DEFINED -> FAILED at the first period
                const auto never_height = checker_never[i].GetStateSinceHeightFor(vpblock.empty() ? nullptr : vpblock.back());
                BOOST_CHECK_MESSAGE(never_height == 0 || never_height == checker_never[i].Period(paramsDummy), strprintf("Test %i for StateSinceHeight (never active)", num));
            }
        }
        num++;
        return *this;
    }

    VersionBitsTester& TestState(ThresholdState exp)
    {
        return TestState(exp, exp);
    }

    VersionBitsTester& TestState(ThresholdState exp, ThresholdState exp_delayed)
    {
        if (exp != exp_delayed) {
            // only expected differences are that delayed stays in locked_in longer
            BOOST_CHECK_EQUAL(exp, ThresholdState::ACTIVE);
            BOOST_CHECK_EQUAL(exp_delayed, ThresholdState::LOCKED_IN);
        }
        for (int i = 0; i < CHECKERS; i++) {
            if (InsecureRandBits(i) == 0) {
                const CBlockIndex* pindex = vpblock.empty() ? nullptr : vpblock.back();
                ThresholdState got = checker[i].GetStateFor(pindex);
                ThresholdState got_delayed = checker_delayed[i].GetStateFor(pindex);
                ThresholdState got_always = checker_always[i].GetStateFor(pindex);
                ThresholdState got_never = checker_never[i].GetStateFor(pindex);
                // nHeight of the next block. If vpblock is empty, the next (ie first)
                // block should be the genesis block with nHeight == 0.
                int height = pindex == nullptr ? 0 : pindex->nHeight + 1;
                BOOST_CHECK_MESSAGE(got == exp, strprintf("Test %i for %s height %d (got %s)", num, StateName(exp), height, StateName(got)));
                BOOST_CHECK_MESSAGE(got_delayed == exp_delayed, strprintf("Test %i for %s height %d (got %s; delayed case)", num, StateName(exp_delayed), height, StateName(got_delayed)));
                BOOST_CHECK_MESSAGE(got_always == ThresholdState::ACTIVE, strprintf("Test %i for ACTIVE height %d (got %s; always active case)", num, height, StateName(got_always)));
                BOOST_CHECK_MESSAGE(got_never == ThresholdState::DEFINED|| got_never == ThresholdState::FAILED, strprintf("Test %i for DEFINED/FAILED height %d (got %s; never active case)", num, height, StateName(got_never)));
            }
        }
        num++;
        return *this;
    }

    VersionBitsTester& TestDefined() { return TestState(ThresholdState::DEFINED); }
    VersionBitsTester& TestStarted() { return TestState(ThresholdState::STARTED); }
    VersionBitsTester& TestLockedIn() { return TestState(ThresholdState::LOCKED_IN); }
    VersionBitsTester& TestActive() { return TestState(ThresholdState::ACTIVE); }
    VersionBitsTester& TestFailed() { return TestState(ThresholdState::FAILED); }

    // non-delayed should be active; delayed should still be locked in
    VersionBitsTester& TestActiveDelayed() { return TestState(ThresholdState::ACTIVE, ThresholdState::LOCKED_IN); }

    CBlockIndex * Tip() { return vpblock.size() ? vpblock.back() : nullptr; }
};

BOOST_FIXTURE_TEST_SUITE(versionbits_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(versionbits_test)
{
    for (int i = 0; i < 64; i++) {
        // DEFINED -> STARTED -> FAILED
        VersionBitsTester().TestDefined().TestStateSinceHeight(0)
                           .Mine(1, TestTime(1), 0).TestDefined().TestStateSinceHeight(0)
                           .Mine(99, TestTime(10000) - 1, 0x100).TestDefined().TestStateSinceHeight(0) // One block more and it would be defined
                           .Mine(100, TestTime(10000), 0x100).TestStarted().TestStateSinceHeight(100) // So that's what happens the next period
                           .Mine(101, TestTime(10010), 0).TestStarted().TestStateSinceHeight(100) // 1 old block
                           .Mine(109, TestTime(10020), 0x100).TestStarted().TestStateSinceHeight(100) // 8 new blocks
                           .Mine(110, TestTime(10020), 0).TestStarted().TestStateSinceHeight(100) // 1 old block (so 8 out of the past 10 are new)
                           .Mine(151, TestTime(10020), 0).TestStarted().TestStateSinceHeight(100)
                           .Mine(200, TestTime(20000), 0).TestFailed().TestStateSinceHeight(200)
                           .Mine(300, TestTime(20010), 0x100).TestFailed().TestStateSinceHeight(200)

        // DEFINED -> STARTED -> LOCKEDIN at the last minute -> ACTIVE
                           .Reset().TestDefined()
                           .Mine(1, TestTime(1), 0).TestDefined().TestStateSinceHeight(0)
                           .Mine(99, TestTime(10000) - 1, 0x101).TestDefined().TestStateSinceHeight(0) // One block more and it would be started
                           .Mine(100, TestTime(10000), 0x101).TestStarted().TestStateSinceHeight(100) // So that's what happens the next period
                           .Mine(109, TestTime(10020), 0x100).TestStarted().TestStateSinceHeight(100) // 9 new blocks
                           .Mine(110, TestTime(29999), 0x200).TestLockedIn().TestStateSinceHeight(110) // 1 old block (so 9 out of the past 10)
                           .Mine(119, TestTime(30001), 0).TestLockedIn().TestStateSinceHeight(110)
                           .Mine(120, TestTime(30002), 0).TestActiveDelayed().TestStateSinceHeight(120, 110) // Delayed will not become active until height 250
                           .Mine(200, TestTime(30003), 0).TestActiveDelayed().TestStateSinceHeight(120, 110)
                           .Mine(250, TestTime(30004), 0).TestActive().TestStateSinceHeight(120, 250)
                           .Mine(300, TestTime(40000), 0).TestActive().TestStateSinceHeight(120, 250)

        // DEFINED multiple periods -> STARTED multiple periods -> FAILED
                           .Reset().TestDefined().TestStateSinceHeight(0)
                           .Mine(9, TestTime(999), 0).TestDefined().TestStateSinceHeight(0)
                           .Mine(10, TestTime(1000), 0).TestDefined().TestStateSinceHeight(0)
                           .Mine(20, TestTime(2000), 0).TestDefined().TestStateSinceHeight(0)
                           .Mine(100, TestTime(10000), 0).TestStarted().TestStateSinceHeight(100)
                           .Mine(103, TestTime(10000), 0).TestStarted().TestStateSinceHeight(100)
                           .Mine(105, TestTime(10000), 0).TestStarted().TestStateSinceHeight(100)
                           .Mine(200, TestTime(20000), 0).TestFailed().TestStateSinceHeight(200)
                           .Mine(300, TestTime(20000), 0x100).TestFailed().TestStateSinceHeight(200);
    }
}

void sanity_check_params(const Consensus::Params& params)
{
    // Sanity checks of version bit deployments
    for (int i=0; i<(int) Consensus::MAX_VERSION_BITS_DEPLOYMENTS; i++) {

        // Verify the threshold is sane and isn't lower than the threshold
        // used for warning for unknown activations
        int threshold = params.vDeployments[i].threshold;
        BOOST_CHECK(threshold > 0);
        BOOST_CHECK((uint32_t)threshold >= params.m_vbits_min_threshold);
        BOOST_CHECK((uint32_t)threshold <= params.nMinerConfirmationWindow);

        uint32_t bitmask = VersionBitsMask(params, static_cast<Consensus::DeploymentPos>(i));
        // Make sure that no deployment tries to set an invalid bit.
        BOOST_CHECK_EQUAL(bitmask & ~(uint32_t)VERSIONBITS_TOP_MASK, bitmask);

        std::string error;
        BOOST_CHECK_MESSAGE(CheckVBitsHeights(error, params, params.vDeployments[i].startheight, params.vDeployments[i].timeoutheight, params.vDeployments[i].m_min_activation_height), error);

        // Verify that the deployment windows of different deployment using the
        // same bit are disjoint.
        // This test may need modification at such time as a new deployment
        // is proposed that reuses the bit of an activated soft fork, before the
        // end time of that soft fork.  (Alternatively, the end time of that
        // activated soft fork could be later changed to be earlier to avoid
        // overlap.)
        for (int j=i+1; j<(int) Consensus::MAX_VERSION_BITS_DEPLOYMENTS; j++) {
            if (VersionBitsMask(params, static_cast<Consensus::DeploymentPos>(j)) == bitmask) {
                BOOST_CHECK(params.vDeployments[j].startheight > params.vDeployments[i].timeoutheight ||
                        params.vDeployments[i].startheight > params.vDeployments[j].timeoutheight);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(versionbits_params)
{
    for (const auto& chain : {CBaseChainParams::MAIN, CBaseChainParams::TESTNET, CBaseChainParams::SIGNET, CBaseChainParams::REGTEST}) {
        const auto chainParams = CreateChainParams(*m_node.args, chain);
        const Consensus::Params &params = chainParams->GetConsensus();
        sanity_check_params(params);
    }
}

BOOST_AUTO_TEST_CASE(versionbits_computeblockversion)
{
    // Check that ComputeBlockVersion will set the appropriate bit correctly
    const auto period = CreateChainParams(*m_node.args, CBaseChainParams::REGTEST)->GetConsensus().nMinerConfirmationWindow;
    gArgs.ForceSetArg("-vbparams", strprintf("testdummy:@%s:@%s", period, period * 3));
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::REGTEST);
    const Consensus::Params &params = chainParams->GetConsensus();

    // Use the TESTDUMMY deployment for testing purposes.
    int64_t bit = params.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit;
    int64_t startheight = params.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].startheight;
    int64_t timeoutheight = params.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].timeoutheight;
    const int64_t nTime = TestTime(startheight);

    assert(startheight < timeoutheight);

    // In the first chain, test that the bit is set by CBV until it has failed.
    // In the second chain, test the bit is set by CBV while STARTED and
    // LOCKED-IN, and then no longer set while ACTIVE.
    VersionBitsTester firstChain, secondChain;

    // Start generating blocks before startheight
    // Before the chain has reached startheight-1, the bit should not be set.
    CBlockIndex *lastBlock = nullptr;
    lastBlock = firstChain.Mine(startheight - 1, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
    BOOST_CHECK_EQUAL(ComputeBlockVersion(lastBlock, params) & (1<<bit), 0);

    // Advance to the next block and transition to STARTED,
    lastBlock = firstChain.Mine(startheight, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
    // so ComputeBlockVersion should now set the bit,
    BOOST_CHECK((ComputeBlockVersion(lastBlock, params) & (1<<bit)) != 0);
    // and should also be using the VERSIONBITS_TOP_BITS.
    BOOST_CHECK_EQUAL(ComputeBlockVersion(lastBlock, params) & VERSIONBITS_TOP_MASK, VERSIONBITS_TOP_BITS);

    // Check that ComputeBlockVersion will set the bit until timeoutheight
    // These blocks are all before timeoutheight is reached.
    lastBlock = firstChain.Mine(timeoutheight - 1, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
    BOOST_CHECK((ComputeBlockVersion(lastBlock, params) & (1<<bit)) != 0);
    BOOST_CHECK_EQUAL(ComputeBlockVersion(lastBlock, params) & VERSIONBITS_TOP_MASK, VERSIONBITS_TOP_BITS);

    // The next block should trigger no longer setting the bit.
    lastBlock = firstChain.Mine(timeoutheight, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
    BOOST_CHECK_EQUAL(ComputeBlockVersion(lastBlock, params) & (1<<bit), 0);

    // On a new chain:
    // verify that the bit will be set after lock-in, and then stop being set
    // after activation.

    // Mine up until startheight-1, and check that the bit will be on for the
    // next period.
    lastBlock = secondChain.Mine(startheight, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
    BOOST_CHECK((ComputeBlockVersion(lastBlock, params) & (1<<bit)) != 0);

    // Mine another block, signaling the new bit.
    lastBlock = secondChain.Mine(startheight + params.nMinerConfirmationWindow, nTime, VERSIONBITS_TOP_BITS | (1<<bit)).Tip();
    // After one period of setting the bit on each block, it should have locked in.
    // We keep setting the bit for one more period though, until activation.
    BOOST_CHECK((ComputeBlockVersion(lastBlock, params) & (1<<bit)) != 0);

    // Now check that we keep mining the block until the end of this period, and
    // then stop at the beginning of the next period.
    lastBlock = secondChain.Mine(startheight + (params.nMinerConfirmationWindow * 2) - 1, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
    BOOST_CHECK((ComputeBlockVersion(lastBlock, params) & (1 << bit)) != 0);
    lastBlock = secondChain.Mine(startheight + (params.nMinerConfirmationWindow * 2), nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
    BOOST_CHECK_EQUAL(ComputeBlockVersion(lastBlock, params) & (1<<bit), 0);

    // Finally, verify that after a soft fork has activated, CBV no longer uses
    // VERSIONBITS_LAST_OLD_BLOCK_VERSION.
    //BOOST_CHECK_EQUAL(ComputeBlockVersion(lastBlock, params) & VERSIONBITS_TOP_MASK, VERSIONBITS_TOP_BITS);
}


BOOST_AUTO_TEST_SUITE_END()
