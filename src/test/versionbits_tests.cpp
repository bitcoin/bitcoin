// Copyright (c) 2014-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <test/util/random.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>
#include <versionbits.h>
#include <versionbits_impl.h>

#include <boost/test/unit_test.hpp>

/* Define a virtual block time, one block per 10 minutes after Nov 14 2014, 0:55:36am */
static int32_t TestTime(int nHeight) { return 1415926536 + 600 * nHeight; }

class TestConditionChecker final : public VersionBitsConditionChecker
{
private:
    mutable ThresholdConditionCache cache;

public:
    // constructor is implicit to allow for easier initialization of vector<TestConditionChecker>
    explicit(false) TestConditionChecker(const Consensus::BIP9Deployment& dep) : VersionBitsConditionChecker{dep} { }
    ~TestConditionChecker() override = default;

    ThresholdState StateFor(const CBlockIndex* pindexPrev) const { return AbstractThresholdConditionChecker::GetStateFor(pindexPrev, cache); }
    int StateSinceHeightFor(const CBlockIndex* pindexPrev) const { return AbstractThresholdConditionChecker::GetStateSinceHeightFor(pindexPrev, cache); }
    void clear() { cache.clear(); }
};

namespace {
struct Deployments
{
    const Consensus::BIP9Deployment normal{
        .bit = 8,
        .nStartTime = TestTime(10000),
        .nTimeout = TestTime(20000),
        .min_activation_height = 0,
        .period = 1000,
        .threshold = 900,
    };
    Consensus::BIP9Deployment always, never, delayed;
    Deployments()
    {
        delayed = normal; delayed.min_activation_height = 15000;
        always = normal; always.nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        never = normal; never.nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
    }
};
}

#define CHECKERS 6

class VersionBitsTester
{
    FastRandomContext& m_rng;
    // A fake blockchain
    std::vector<CBlockIndex*> vpblock;

    // Used to automatically set the top bits for manual calls to Mine()
    const int32_t nVersionBase{0};

    // Setup BIP9Deployment structs for the checkers
    const Deployments test_deployments;

    // 6 independent checkers for the same bit.
    // The first one performs all checks, the second only 50%, the third only 25%, etc...
    // This is to test whether lack of cached information leads to the same results.
    std::vector<TestConditionChecker> checker{CHECKERS, {test_deployments.normal}};
    // Another 6 that assume delayed activation
    std::vector<TestConditionChecker> checker_delayed{CHECKERS, {test_deployments.delayed}};
    // Another 6 that assume always active activation
    std::vector<TestConditionChecker> checker_always{CHECKERS, {test_deployments.always}};
    // Another 6 that assume never active activation
    std::vector<TestConditionChecker> checker_never{CHECKERS, {test_deployments.never}};

    // Test counter (to identify failures)
    int num{1000};

public:
    explicit VersionBitsTester(FastRandomContext& rng, int32_t nVersionBase=0) : m_rng{rng}, nVersionBase{nVersionBase} { }

    VersionBitsTester& Reset() {
        // Have each group of tests be counted by the 1000s part, starting at 1000
        num = num - (num % 1000) + 1000;

        for (unsigned int i = 0; i < vpblock.size(); i++) {
            delete vpblock[i];
        }
        for (unsigned int  i = 0; i < CHECKERS; i++) {
            checker[i].clear();
            checker_delayed[i].clear();
            checker_always[i].clear();
            checker_never[i].clear();
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
            pindex->pprev = Tip();
            pindex->nTime = nTime;
            pindex->nVersion = (nVersionBase | nVersion);
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
        const CBlockIndex* tip = Tip();
        for (int i = 0; i < CHECKERS; i++) {
            if (m_rng.randbits(i) == 0) {
                BOOST_CHECK_MESSAGE(checker[i].StateSinceHeightFor(tip) == height, strprintf("Test %i for StateSinceHeight", num));
                BOOST_CHECK_MESSAGE(checker_delayed[i].StateSinceHeightFor(tip) == height_delayed, strprintf("Test %i for StateSinceHeight (delayed)", num));
                BOOST_CHECK_MESSAGE(checker_always[i].StateSinceHeightFor(tip) == 0, strprintf("Test %i for StateSinceHeight (always active)", num));
                BOOST_CHECK_MESSAGE(checker_never[i].StateSinceHeightFor(tip) == 0, strprintf("Test %i for StateSinceHeight (never active)", num));
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

        const CBlockIndex* pindex = Tip();
        for (int i = 0; i < CHECKERS; i++) {
            if (m_rng.randbits(i) == 0) {
                ThresholdState got = checker[i].StateFor(pindex);
                ThresholdState got_delayed = checker_delayed[i].StateFor(pindex);
                ThresholdState got_always = checker_always[i].StateFor(pindex);
                ThresholdState got_never = checker_never[i].StateFor(pindex);
                // nHeight of the next block. If vpblock is empty, the next (ie first)
                // block should be the genesis block with nHeight == 0.
                int height = pindex == nullptr ? 0 : pindex->nHeight + 1;
                BOOST_CHECK_MESSAGE(got == exp, strprintf("Test %i for %s height %d (got %s)", num, StateName(exp), height, StateName(got)));
                BOOST_CHECK_MESSAGE(got_delayed == exp_delayed, strprintf("Test %i for %s height %d (got %s; delayed case)", num, StateName(exp_delayed), height, StateName(got_delayed)));
                BOOST_CHECK_MESSAGE(got_always == ThresholdState::ACTIVE, strprintf("Test %i for ACTIVE height %d (got %s; always active case)", num, height, StateName(got_always)));
                BOOST_CHECK_MESSAGE(got_never == ThresholdState::FAILED, strprintf("Test %i for FAILED height %d (got %s; never active case)", num, height, StateName(got_never)));
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

    CBlockIndex* Tip() { return vpblock.empty() ? nullptr : vpblock.back(); }
};

BOOST_FIXTURE_TEST_SUITE(versionbits_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(versionbits_test)
{
    for (int i = 0; i < 64; i++) {
        // DEFINED -> STARTED after timeout reached -> FAILED
        VersionBitsTester(m_rng, VERSIONBITS_TOP_BITS).TestDefined().TestStateSinceHeight(0)
                           .Mine(1, TestTime(1), 0x100).TestDefined().TestStateSinceHeight(0)
                           .Mine(11, TestTime(11), 0x100).TestDefined().TestStateSinceHeight(0)
                           .Mine(989, TestTime(989), 0x100).TestDefined().TestStateSinceHeight(0)
                           .Mine(999, TestTime(20000), 0x100).TestDefined().TestStateSinceHeight(0) // Timeout and start time reached simultaneously
                           .Mine(1000, TestTime(20000), 0).TestStarted().TestStateSinceHeight(1000) // Hit started, stop signalling
                           .Mine(1999, TestTime(30001), 0).TestStarted().TestStateSinceHeight(1000)
                           .Mine(2000, TestTime(30002), 0x100).TestFailed().TestStateSinceHeight(2000) // Hit failed, start signalling again
                           .Mine(2001, TestTime(30003), 0x100).TestFailed().TestStateSinceHeight(2000)
                           .Mine(2999, TestTime(30004), 0x100).TestFailed().TestStateSinceHeight(2000)
                           .Mine(3000, TestTime(30005), 0x100).TestFailed().TestStateSinceHeight(2000)
                           .Mine(4000, TestTime(30006), 0x100).TestFailed().TestStateSinceHeight(2000)

        // DEFINED -> STARTED -> FAILED
                           .Reset().TestDefined().TestStateSinceHeight(0)
                           .Mine(1, TestTime(1), 0).TestDefined().TestStateSinceHeight(0)
                           .Mine(1000, TestTime(10000) - 1, 0x100).TestDefined().TestStateSinceHeight(0) // One second more and it would be defined
                           .Mine(2000, TestTime(10000), 0x100).TestStarted().TestStateSinceHeight(2000) // So that's what happens the next period
                           .Mine(2051, TestTime(10010), 0).TestStarted().TestStateSinceHeight(2000) // 51 old blocks
                           .Mine(2950, TestTime(10020), 0x100).TestStarted().TestStateSinceHeight(2000) // 899 new blocks
                           .Mine(3000, TestTime(20000), 0).TestFailed().TestStateSinceHeight(3000) // 50 old blocks (so 899 out of the past 1000)
                           .Mine(4000, TestTime(20010), 0x100).TestFailed().TestStateSinceHeight(3000)

        // DEFINED -> STARTED -> LOCKEDIN after timeout reached -> ACTIVE
                           .Reset().TestDefined().TestStateSinceHeight(0)
                           .Mine(1, TestTime(1), 0).TestDefined().TestStateSinceHeight(0)
                           .Mine(1000, TestTime(10000) - 1, 0x101).TestDefined().TestStateSinceHeight(0) // One second more and it would be defined
                           .Mine(2000, TestTime(10000), 0x101).TestStarted().TestStateSinceHeight(2000) // So that's what happens the next period
                           .Mine(2999, TestTime(30000), 0x100).TestStarted().TestStateSinceHeight(2000) // 999 new blocks
                           .Mine(3000, TestTime(30000), 0x100).TestLockedIn().TestStateSinceHeight(3000) // 1 new block (so 1000 out of the past 1000 are new)
                           .Mine(3999, TestTime(30001), 0).TestLockedIn().TestStateSinceHeight(3000)
                           .Mine(4000, TestTime(30002), 0).TestActiveDelayed().TestStateSinceHeight(4000, 3000)
                           .Mine(14333, TestTime(30003), 0).TestActiveDelayed().TestStateSinceHeight(4000, 3000)
                           .Mine(24000, TestTime(40000), 0).TestActive().TestStateSinceHeight(4000, 15000)

        // DEFINED -> STARTED -> LOCKEDIN before timeout -> ACTIVE
                           .Reset().TestDefined()
                           .Mine(1, TestTime(1), 0).TestDefined().TestStateSinceHeight(0)
                           .Mine(1000, TestTime(10000) - 1, 0x101).TestDefined().TestStateSinceHeight(0) // One second more and it would be defined
                           .Mine(2000, TestTime(10000), 0x101).TestStarted().TestStateSinceHeight(2000) // So that's what happens the next period
                           .Mine(2050, TestTime(10010), 0x200).TestStarted().TestStateSinceHeight(2000) // 50 old blocks
                           .Mine(2950, TestTime(10020), 0x100).TestStarted().TestStateSinceHeight(2000) // 900 new blocks
                           .Mine(2999, TestTime(19999), 0x200).TestStarted().TestStateSinceHeight(2000) // 49 old blocks
                           .Mine(3000, TestTime(29999), 0x200).TestLockedIn().TestStateSinceHeight(3000) // 1 old block (so 900 out of the past 1000)
                           .Mine(3999, TestTime(30001), 0).TestLockedIn().TestStateSinceHeight(3000)
                           .Mine(4000, TestTime(30002), 0).TestActiveDelayed().TestStateSinceHeight(4000, 3000) // delayed will not become active until height=15000
                           .Mine(14333, TestTime(30003), 0).TestActiveDelayed().TestStateSinceHeight(4000, 3000)
                           .Mine(15000, TestTime(40000), 0).TestActive().TestStateSinceHeight(4000, 15000)
                           .Mine(24000, TestTime(40000), 0).TestActive().TestStateSinceHeight(4000, 15000)

        // DEFINED multiple periods -> STARTED multiple periods -> FAILED
                           .Reset().TestDefined().TestStateSinceHeight(0)
                           .Mine(999, TestTime(999), 0).TestDefined().TestStateSinceHeight(0)
                           .Mine(1000, TestTime(1000), 0).TestDefined().TestStateSinceHeight(0)
                           .Mine(2000, TestTime(2000), 0).TestDefined().TestStateSinceHeight(0)
                           .Mine(3000, TestTime(10000), 0).TestStarted().TestStateSinceHeight(3000)
                           .Mine(4000, TestTime(10000), 0).TestStarted().TestStateSinceHeight(3000)
                           .Mine(5000, TestTime(10000), 0).TestStarted().TestStateSinceHeight(3000)
                           .Mine(5999, TestTime(20000), 0).TestStarted().TestStateSinceHeight(3000)
                           .Mine(6000, TestTime(20000), 0).TestFailed().TestStateSinceHeight(6000)
                           .Mine(7000, TestTime(20000), 0x100).TestFailed().TestStateSinceHeight(6000)
                           .Mine(24000, TestTime(20000), 0x100).TestFailed().TestStateSinceHeight(6000) // stay in FAILED no matter how much we signal
        ;
    }
}

struct BlockVersionTest : BasicTestingSetup {
/** Check that ComputeBlockVersion will set the appropriate bit correctly
 * Also checks IsActiveAfter() behaviour */
void check_computeblockversion(VersionBitsCache& versionbitscache, const Consensus::Params& params, Consensus::DeploymentPos dep)
{
    // Clear the cache every time
    versionbitscache.Clear();

    int64_t bit = params.vDeployments[dep].bit;
    int64_t nStartTime = params.vDeployments[dep].nStartTime;
    int64_t nTimeout = params.vDeployments[dep].nTimeout;
    int min_activation_height = params.vDeployments[dep].min_activation_height;
    uint32_t period = params.vDeployments[dep].period;
    uint32_t threshold = params.vDeployments[dep].threshold;

    BOOST_REQUIRE(period > 0); // no division by zero, thankyou
    BOOST_REQUIRE(0 < threshold); // must be able to have a window that doesn't activate
    BOOST_REQUIRE(threshold < period); // must be able to have a window that does activate

    // should not be any signalling for first block
    BOOST_CHECK_EQUAL(versionbitscache.ComputeBlockVersion(nullptr, params), VERSIONBITS_TOP_BITS);

    // always/never active deployments shouldn't need to be tested further
    if (nStartTime == Consensus::BIP9Deployment::ALWAYS_ACTIVE ||
        nStartTime == Consensus::BIP9Deployment::NEVER_ACTIVE)
    {
        if (nStartTime == Consensus::BIP9Deployment::ALWAYS_ACTIVE) {
            BOOST_CHECK(versionbitscache.IsActiveAfter(nullptr, params, dep));
        } else {
            BOOST_CHECK(!versionbitscache.IsActiveAfter(nullptr, params, dep));
        }
        BOOST_CHECK_EQUAL(min_activation_height, 0);
        BOOST_CHECK_EQUAL(nTimeout, Consensus::BIP9Deployment::NO_TIMEOUT);
        return;
    }

    BOOST_REQUIRE(nStartTime < nTimeout);
    BOOST_REQUIRE(nStartTime >= 0);
    BOOST_REQUIRE(nTimeout <= std::numeric_limits<uint32_t>::max() || nTimeout == Consensus::BIP9Deployment::NO_TIMEOUT);
    BOOST_REQUIRE(0 <= bit && bit < 32);
    // Make sure that no deployment tries to set an invalid bit.
    BOOST_REQUIRE(((1 << bit) & VERSIONBITS_TOP_MASK) == 0);
    BOOST_REQUIRE(min_activation_height >= 0);
    // Check min_activation_height is on a retarget boundary
    BOOST_REQUIRE_EQUAL(min_activation_height % period, 0U);

    // In the first chain, test that the bit is set by CBV until it has failed.
    // In the second chain, test the bit is set by CBV while STARTED and
    // LOCKED-IN, and then no longer set while ACTIVE.
    VersionBitsTester firstChain{m_rng}, secondChain{m_rng};

    int64_t nTime = nStartTime;

    const CBlockIndex *lastBlock = nullptr;

    // Before MedianTimePast of the chain has crossed nStartTime, the bit
    // should not be set.
    if (nTime == 0) {
        // since CBlockIndex::nTime is uint32_t we can't represent any
        // earlier time, so will transition from DEFINED to STARTED at the
        // end of the first period by mining blocks at nTime == 0
        lastBlock = firstChain.Mine(period - 1, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
        BOOST_CHECK_EQUAL(versionbitscache.ComputeBlockVersion(lastBlock, params) & (1 << bit), 0);
        BOOST_CHECK(!versionbitscache.IsActiveAfter(lastBlock, params, dep));
        lastBlock = firstChain.Mine(period, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
        BOOST_CHECK((versionbitscache.ComputeBlockVersion(lastBlock, params) & (1 << bit)) != 0);
        BOOST_CHECK(!versionbitscache.IsActiveAfter(lastBlock, params, dep));
        // then we'll keep mining at nStartTime...
    } else {
        // use a time 1s earlier than start time to check we stay DEFINED
        --nTime;

        // Start generating blocks before nStartTime
        lastBlock = firstChain.Mine(period, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
        BOOST_CHECK_EQUAL(versionbitscache.ComputeBlockVersion(lastBlock, params) & (1 << bit), 0);
        BOOST_CHECK(!versionbitscache.IsActiveAfter(lastBlock, params, dep));

        // Mine more blocks (4 less than the adjustment period) at the old time, and check that CBV isn't setting the bit yet.
        for (uint32_t i = 1; i < period - 4; i++) {
            lastBlock = firstChain.Mine(period + i, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
            BOOST_CHECK_EQUAL(versionbitscache.ComputeBlockVersion(lastBlock, params) & (1 << bit), 0);
            BOOST_CHECK(!versionbitscache.IsActiveAfter(lastBlock, params, dep));
        }
        // Now mine 5 more blocks at the start time -- MTP should not have passed yet, so
        // CBV should still not yet set the bit.
        nTime = nStartTime;
        for (uint32_t i = period - 4; i <= period; i++) {
            lastBlock = firstChain.Mine(period + i, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
            BOOST_CHECK_EQUAL(versionbitscache.ComputeBlockVersion(lastBlock, params) & (1 << bit), 0);
            BOOST_CHECK(!versionbitscache.IsActiveAfter(lastBlock, params, dep));
        }
        // Next we will advance to the next period and transition to STARTED,
    }

    lastBlock = firstChain.Mine(period * 3, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
    // so ComputeBlockVersion should now set the bit,
    BOOST_CHECK((versionbitscache.ComputeBlockVersion(lastBlock, params) & (1 << bit)) != 0);
    // and should also be using the VERSIONBITS_TOP_BITS.
    BOOST_CHECK_EQUAL(versionbitscache.ComputeBlockVersion(lastBlock, params) & VERSIONBITS_TOP_MASK, VERSIONBITS_TOP_BITS);
    BOOST_CHECK(!versionbitscache.IsActiveAfter(lastBlock, params, dep));

    // Check that ComputeBlockVersion will set the bit until nTimeout
    nTime += 600;
    uint32_t blocksToMine = period * 2; // test blocks for up to 2 time periods
    uint32_t nHeight = period * 3;
    // These blocks are all before nTimeout is reached.
    while (nTime < nTimeout && blocksToMine > 0) {
        lastBlock = firstChain.Mine(nHeight+1, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
        BOOST_CHECK((versionbitscache.ComputeBlockVersion(lastBlock, params) & (1 << bit)) != 0);
        BOOST_CHECK_EQUAL(versionbitscache.ComputeBlockVersion(lastBlock, params) & VERSIONBITS_TOP_MASK, VERSIONBITS_TOP_BITS);
        BOOST_CHECK(!versionbitscache.IsActiveAfter(lastBlock, params, dep));
        blocksToMine--;
        nTime += 600;
        nHeight += 1;
    }

    if (nTimeout != Consensus::BIP9Deployment::NO_TIMEOUT) {
        // can reach any nTimeout other than NO_TIMEOUT due to earlier BOOST_REQUIRE

        nTime = nTimeout;

        // finish the last period before we start timing out
        while (nHeight % period != 0) {
            lastBlock = firstChain.Mine(nHeight+1, nTime - 1, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
            BOOST_CHECK((versionbitscache.ComputeBlockVersion(lastBlock, params) & (1 << bit)) != 0);
            BOOST_CHECK(!versionbitscache.IsActiveAfter(lastBlock, params, dep));
            nHeight += 1;
        }

        // FAILED is only triggered at the end of a period, so CBV should be setting
        // the bit until the period transition.
        for (uint32_t i = 0; i < period - 1; i++) {
            lastBlock = firstChain.Mine(nHeight+1, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
            BOOST_CHECK((versionbitscache.ComputeBlockVersion(lastBlock, params) & (1 << bit)) != 0);
            BOOST_CHECK(!versionbitscache.IsActiveAfter(lastBlock, params, dep));
            nHeight += 1;
        }
        // The next block should trigger no longer setting the bit.
        lastBlock = firstChain.Mine(nHeight+1, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
        BOOST_CHECK_EQUAL(versionbitscache.ComputeBlockVersion(lastBlock, params) & (1 << bit), 0);
        BOOST_CHECK(!versionbitscache.IsActiveAfter(lastBlock, params, dep));
    }

    // On a new chain:
    // verify that the bit will be set after lock-in, and then stop being set
    // after activation.
    nTime = nStartTime;

    // Mine one period worth of blocks, and check that the bit will be on for the
    // next period.
    lastBlock = secondChain.Mine(period, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
    BOOST_CHECK((versionbitscache.ComputeBlockVersion(lastBlock, params) & (1 << bit)) != 0);
    BOOST_CHECK(!versionbitscache.IsActiveAfter(lastBlock, params, dep));

    // Mine another period worth of blocks, signaling the new bit.
    lastBlock = secondChain.Mine(period * 2, nTime, VERSIONBITS_TOP_BITS | (1<<bit)).Tip();
    // After one period of setting the bit on each block, it should have locked in.
    // We keep setting the bit for one more period though, until activation.
    BOOST_CHECK((versionbitscache.ComputeBlockVersion(lastBlock, params) & (1 << bit)) != 0);
    BOOST_CHECK(!versionbitscache.IsActiveAfter(lastBlock, params, dep));

    // Now check that we keep mining the block until the end of this period, and
    // then stop at the beginning of the next period.
    lastBlock = secondChain.Mine((period * 3) - 1, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
    BOOST_CHECK((versionbitscache.ComputeBlockVersion(lastBlock, params) & (1 << bit)) != 0);
    BOOST_CHECK(!versionbitscache.IsActiveAfter(lastBlock, params, dep));
    lastBlock = secondChain.Mine(period * 3, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();

    if (lastBlock->nHeight + 1 < min_activation_height) {
        // check signalling continues while min_activation_height is not reached
        lastBlock = secondChain.Mine(min_activation_height - 1, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
        BOOST_CHECK((versionbitscache.ComputeBlockVersion(lastBlock, params) & (1 << bit)) != 0);
        BOOST_CHECK(!versionbitscache.IsActiveAfter(lastBlock, params, dep));
        // then reach min_activation_height, which was already REQUIRE'd to start a new period
        lastBlock = secondChain.Mine(min_activation_height, nTime, VERSIONBITS_LAST_OLD_BLOCK_VERSION).Tip();
    }

    // Check that we don't signal after activation
    BOOST_CHECK_EQUAL(versionbitscache.ComputeBlockVersion(lastBlock, params) & (1 << bit), 0);
    BOOST_CHECK(versionbitscache.IsActiveAfter(lastBlock, params, dep));
}
}; // struct BlockVersionTest

BOOST_FIXTURE_TEST_CASE(versionbits_computeblockversion, BlockVersionTest)
{
    VersionBitsCache vbcache;

    // check that any deployment on any chain can conceivably reach both
    // ACTIVE and FAILED states in roughly the way we expect
    for (const auto& chain_type: {ChainType::MAIN, ChainType::TESTNET, ChainType::TESTNET4, ChainType::SIGNET, ChainType::REGTEST}) {
        const auto chainParams = CreateChainParams(*m_node.args, chain_type);
        uint32_t chain_all_vbits{0};
        for (int i = 0; i < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++i) {
            const auto dep = static_cast<Consensus::DeploymentPos>(i);
            // Check that no bits are reused (within the same chain). This is
            // disallowed because the transition to FAILED (on timeout) does
            // not take precedence over STARTED/LOCKED_IN. So all softforks on
            // the same bit might overlap, even when non-overlapping start-end
            // times are picked.
            const uint32_t dep_mask{uint32_t{1} << chainParams->GetConsensus().vDeployments[dep].bit};
            BOOST_CHECK(!(chain_all_vbits & dep_mask));
            chain_all_vbits |= dep_mask;
            check_computeblockversion(vbcache, chainParams->GetConsensus(), dep);
        }
    }

    {
        // Use regtest/testdummy to ensure we always exercise some
        // deployment that's not always/never active
        ArgsManager args;
        args.ForceSetArg("-vbparams", "testdummy:1199145601:1230767999"); // January 1, 2008 - December 31, 2008
        const auto chainParams = CreateChainParams(args, ChainType::REGTEST);
        check_computeblockversion(vbcache, chainParams->GetConsensus(), Consensus::DEPLOYMENT_TESTDUMMY);
    }

    {
        // Use regtest/testdummy to ensure we always exercise the
        // min_activation_height test, even if we're not using that in a
        // live deployment
        ArgsManager args;
        args.ForceSetArg("-vbparams", "testdummy:1199145601:1230767999:403200"); // January 1, 2008 - December 31, 2008, min act height 403200
        const auto chainParams = CreateChainParams(args, ChainType::REGTEST);
        check_computeblockversion(vbcache, chainParams->GetConsensus(), Consensus::DEPLOYMENT_TESTDUMMY);
    }
}

/**
 * Test condition checker with max_activation_height for mandatory activation deadline.
 * When max_activation_height is set, the deployment forces LOCKED_IN one period before
 * max_activation_height, even if threshold signaling was not met.
 */
class TestMaxActivationHeightConditionChecker : public AbstractThresholdConditionChecker
{
private:
    mutable ThresholdConditionCache cache;
    int m_max_activation_height;

public:
    explicit TestMaxActivationHeightConditionChecker(int max_height) : m_max_activation_height(max_height) {}

    int64_t BeginTime() const override { return 0; }
    int64_t EndTime() const override { return Consensus::BIP9Deployment::NO_TIMEOUT; }
    int Period() const override { return 144; }
    int Threshold() const override { return 108; }
    int MaxActivationHeight() const override { return m_max_activation_height; }
    bool Condition(const CBlockIndex* pindex) const override { return (pindex->nVersion & 0x100); }

    ThresholdState GetStateFor(const CBlockIndex* pindexPrev) const { return AbstractThresholdConditionChecker::GetStateFor(pindexPrev, cache); }
    int GetStateSinceHeightFor(const CBlockIndex* pindexPrev) const { return AbstractThresholdConditionChecker::GetStateSinceHeightFor(pindexPrev, cache); }
    void ClearCache() { cache.clear(); }
};

BOOST_AUTO_TEST_CASE(versionbits_max_activation_height)
{
    // Test that max_activation_height forces LOCKED_IN one period before max_activation_height
    // even without sufficient signaling.
    //
    // Timeline with period=144, max_activation_height=432:
    // - Period 0 (0-143): DEFINED
    // - Period 1 (144-287): STARTED (no signaling -> normally would stay STARTED)
    // - Period 2 (288-431): LOCKED_IN (forced because 288 >= 432 - 144)
    // - Period 3 (432+): ACTIVE

    std::vector<CBlockIndex*> blocks;
    auto cleanup = [&blocks]() {
        for (auto* b : blocks) delete b;
        blocks.clear();
    };

    // max_activation_height = 432 (period 3 start)
    TestMaxActivationHeightConditionChecker checker(432);

    // Helper to create blocks
    auto mine_block = [&blocks](int32_t nVersion) -> CBlockIndex* {
        CBlockIndex* pindex = new CBlockIndex();
        pindex->nHeight = blocks.size();
        pindex->pprev = blocks.empty() ? nullptr : blocks.back();
        pindex->nTime = 1415926536 + 600 * pindex->nHeight;
        pindex->nVersion = nVersion;
        pindex->BuildSkip();
        blocks.push_back(pindex);
        return pindex;
    };

    // Mine through period 0 (DEFINED) - 144 blocks (0-143)
    for (int i = 0; i < 144; i++) {
        mine_block(0); // No signaling
    }
    BOOST_CHECK_EQUAL(blocks.back()->nHeight, 143);
    // At tip 143, next block (144) would be STARTED
    BOOST_CHECK(checker.GetStateFor(blocks.back()) == ThresholdState::STARTED);
    BOOST_CHECK_EQUAL(checker.GetStateSinceHeightFor(blocks.back()), 144);

    // Mine through period 1 (STARTED) without signaling - blocks 144-287
    for (int i = 0; i < 144; i++) {
        mine_block(0); // No signaling
    }
    BOOST_CHECK_EQUAL(blocks.back()->nHeight, 287);
    // At tip 287, next block (288) would be LOCKED_IN due to max_activation_height
    // 288 >= 432 - 144, so forced LOCKED_IN
    BOOST_CHECK(checker.GetStateFor(blocks.back()) == ThresholdState::LOCKED_IN);
    BOOST_CHECK_EQUAL(checker.GetStateSinceHeightFor(blocks.back()), 288);

    // Mine through period 2 (LOCKED_IN) - blocks 288-431
    for (int i = 0; i < 144; i++) {
        mine_block(0);
    }
    BOOST_CHECK_EQUAL(blocks.back()->nHeight, 431);
    // At tip 431, next block (432) would be ACTIVE
    BOOST_CHECK(checker.GetStateFor(blocks.back()) == ThresholdState::ACTIVE);
    BOOST_CHECK_EQUAL(checker.GetStateSinceHeightFor(blocks.back()), 432);

    // Mine into period 3 (ACTIVE) - blocks 432+
    for (int i = 0; i < 10; i++) {
        mine_block(0);
    }
    BOOST_CHECK_EQUAL(blocks.back()->nHeight, 441);
    BOOST_CHECK(checker.GetStateFor(blocks.back()) == ThresholdState::ACTIVE);
    BOOST_CHECK_EQUAL(checker.GetStateSinceHeightFor(blocks.back()), 432);

    cleanup();

    // Test 2: Verify that signaling still works to activate earlier than max_activation_height
    TestMaxActivationHeightConditionChecker checker2(1000); // max_activation_height far in future

    // Period 0: DEFINED
    for (int i = 0; i < 144; i++) {
        mine_block(0);
    }
    BOOST_CHECK(checker2.GetStateFor(blocks.back()) == ThresholdState::STARTED);

    // Period 1: Signal 108+ blocks (threshold)
    for (int i = 0; i < 108; i++) {
        mine_block(0x100); // Signal
    }
    for (int i = 0; i < 36; i++) {
        mine_block(0); // No signal
    }
    BOOST_CHECK_EQUAL(blocks.back()->nHeight, 287);
    // Should be LOCKED_IN via signaling, not via max_activation_height
    BOOST_CHECK(checker2.GetStateFor(blocks.back()) == ThresholdState::LOCKED_IN);
    BOOST_CHECK_EQUAL(checker2.GetStateSinceHeightFor(blocks.back()), 288);

    // Period 2: LOCKED_IN -> ACTIVE
    for (int i = 0; i < 144; i++) {
        mine_block(0);
    }
    BOOST_CHECK(checker2.GetStateFor(blocks.back()) == ThresholdState::ACTIVE);
    BOOST_CHECK_EQUAL(checker2.GetStateSinceHeightFor(blocks.back()), 432);

    cleanup();
}

BOOST_AUTO_TEST_CASE(versionbits_max_activation_height_boundary)
{
    // Test edge case: verify exact boundary where LOCKED_IN is forced
    // With period=144 and max_activation_height=432:
    // - At height 287, next block is 288, which is >= 432-144=288, so LOCKED_IN
    // - At height 286, next block is 287, which is < 288, so would stay STARTED

    std::vector<CBlockIndex*> blocks;
    auto cleanup = [&blocks]() {
        for (auto* b : blocks) delete b;
        blocks.clear();
    };

    TestMaxActivationHeightConditionChecker checker(432);

    auto mine_block = [&blocks](int32_t nVersion) -> CBlockIndex* {
        CBlockIndex* pindex = new CBlockIndex();
        pindex->nHeight = blocks.size();
        pindex->pprev = blocks.empty() ? nullptr : blocks.back();
        pindex->nTime = 1415926536 + 600 * pindex->nHeight;
        pindex->nVersion = nVersion;
        pindex->BuildSkip();
        blocks.push_back(pindex);
        return pindex;
    };

    // Mine to height 143 (end of period 0)
    for (int i = 0; i < 144; i++) {
        mine_block(0);
    }
    BOOST_CHECK_EQUAL(blocks.back()->nHeight, 143);
    // State for block 144 is STARTED
    BOOST_CHECK(checker.GetStateFor(blocks.back()) == ThresholdState::STARTED);

    // Mine period 1 without signaling (blocks 144-287)
    // But stop at block 286 first to check boundary
    for (int i = 0; i < 143; i++) {
        mine_block(0);
    }
    BOOST_CHECK_EQUAL(blocks.back()->nHeight, 286);
    // At tip 286, next block 287 is still in STARTED period
    // State is still STARTED
    BOOST_CHECK(checker.GetStateFor(blocks.back()) == ThresholdState::STARTED);

    // Mine block 287 (last block of period 1)
    mine_block(0);
    BOOST_CHECK_EQUAL(blocks.back()->nHeight, 287);
    // At tip 287, state for next block (288) is computed
    // 288 >= 432 - 144 = 288, so LOCKED_IN
    BOOST_CHECK(checker.GetStateFor(blocks.back()) == ThresholdState::LOCKED_IN);
    BOOST_CHECK_EQUAL(checker.GetStateSinceHeightFor(blocks.back()), 288);

    cleanup();
}

BOOST_FIXTURE_TEST_CASE(versionbits_active_duration, BasicTestingSetup)
{
    // Test active_duration parameter via -vbparams
    // Format: deployment:start:timeout:min_activation_height:max_activation_height:active_duration
    //
    // This tests that the parameter is parsed correctly. The actual expiry logic
    // is tested in DeploymentActiveAt/DeploymentActiveAfter which use active_duration.

    {
        ArgsManager args;
        // start=0, timeout=never, min_height=0, max_height=INT_MAX (disabled), active_duration=144
        args.ForceSetArg("-vbparams", "testdummy:0:999999999999:0:2147483647:144");
        const auto chainParams = CreateChainParams(args, ChainType::REGTEST);
        const auto& deployment = chainParams->GetConsensus().vDeployments[Consensus::DEPLOYMENT_TESTDUMMY];

        BOOST_CHECK_EQUAL(deployment.nStartTime, 0);
        BOOST_CHECK_EQUAL(deployment.nTimeout, 999999999999);
        BOOST_CHECK_EQUAL(deployment.min_activation_height, 0);
        BOOST_CHECK_EQUAL(deployment.max_activation_height, std::numeric_limits<int>::max());
        BOOST_CHECK_EQUAL(deployment.active_duration, 144);
    }

    {
        ArgsManager args;
        // Test with max_activation_height set
        // start=0, timeout=NO_TIMEOUT, min_height=288, max_height=432, active_duration=1008 (144*7)
        // NO_TIMEOUT = INT64_MAX = 9223372036854775807
        args.ForceSetArg("-vbparams", "testdummy:0:9223372036854775807:288:432:1008");
        const auto chainParams = CreateChainParams(args, ChainType::REGTEST);
        const auto& deployment = chainParams->GetConsensus().vDeployments[Consensus::DEPLOYMENT_TESTDUMMY];

        BOOST_CHECK_EQUAL(deployment.min_activation_height, 288);
        BOOST_CHECK_EQUAL(deployment.max_activation_height, 432);
        BOOST_CHECK_EQUAL(deployment.active_duration, 1008);
    }

    {
        ArgsManager args;
        // Test permanent deployment (active_duration = INT_MAX)
        args.ForceSetArg("-vbparams", "testdummy:0:999999999999:0:2147483647:2147483647");
        const auto chainParams = CreateChainParams(args, ChainType::REGTEST);
        const auto& deployment = chainParams->GetConsensus().vDeployments[Consensus::DEPLOYMENT_TESTDUMMY];

        BOOST_CHECK_EQUAL(deployment.active_duration, std::numeric_limits<int>::max());
    }
}

BOOST_FIXTURE_TEST_CASE(versionbits_max_activation_height_parsing, BasicTestingSetup)
{
    // Test max_activation_height parameter via -vbparams

    {
        ArgsManager args;
        // Test with max_activation_height=432 (mandatory activation deadline)
        // NO_TIMEOUT = INT64_MAX = 9223372036854775807
        args.ForceSetArg("-vbparams", "testdummy:0:9223372036854775807:0:432:2147483647");
        const auto chainParams = CreateChainParams(args, ChainType::REGTEST);
        const auto& deployment = chainParams->GetConsensus().vDeployments[Consensus::DEPLOYMENT_TESTDUMMY];

        BOOST_CHECK_EQUAL(deployment.max_activation_height, 432);
        // active_duration should be permanent when not specified differently
        BOOST_CHECK_EQUAL(deployment.active_duration, std::numeric_limits<int>::max());
    }

    {
        ArgsManager args;
        // Test combined: max_activation_height + active_duration (RDTS)
        // NO_TIMEOUT = INT64_MAX = 9223372036854775807
        args.ForceSetArg("-vbparams", "testdummy:0:9223372036854775807:288:576:144");
        const auto chainParams = CreateChainParams(args, ChainType::REGTEST);
        const auto& deployment = chainParams->GetConsensus().vDeployments[Consensus::DEPLOYMENT_TESTDUMMY];

        BOOST_CHECK_EQUAL(deployment.min_activation_height, 288);
        BOOST_CHECK_EQUAL(deployment.max_activation_height, 576);
        BOOST_CHECK_EQUAL(deployment.active_duration, 144);
    }
}

/**
 * Helper to create a BIP9Deployment for temporary deployment tests.
 * Uses bit 8 and version mask 0x100 for signaling (same as Deployments::normal).
 */
static Consensus::BIP9Deployment MakeTemporaryDeployment(int active_duration, int max_activation_height = std::numeric_limits<int>::max())
{
    return Consensus::BIP9Deployment{
        .bit = 8,
        .nStartTime = 0,
        .nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT,
        .min_activation_height = 0,
        .max_activation_height = max_activation_height,
        .period = 144,
        .threshold = 108,
        .active_duration = active_duration,
    };
}

BOOST_AUTO_TEST_CASE(versionbits_expired_state)
{
    // Test that a temporary deployment transitions from ACTIVE to EXPIRED
    // after active_duration blocks past activation.
    //
    // Timeline with period=144, active_duration=288:
    // - Period 0 (0-143): DEFINED
    // - Period 1 (144-287): STARTED, signal enough to lock in
    // - Period 2 (288-431): LOCKED_IN
    // - Period 3 (432-575): ACTIVE (activation_height=432)
    // - Period 4 (576-719): ACTIVE (blocks in this period are ACTIVE)
    // - At pindexPrev=719: EXPIRED for block 720+ (720 >= 432 + 288)

    std::vector<CBlockIndex*> blocks;
    auto cleanup = [&blocks]() {
        for (auto* b : blocks) delete b;
        blocks.clear();
    };

    const auto dep = MakeTemporaryDeployment(288);
    TestConditionChecker checker(dep);

    auto mine_block = [&blocks](int32_t nVersion) -> CBlockIndex* {
        CBlockIndex* pindex = new CBlockIndex();
        pindex->nHeight = blocks.size();
        pindex->pprev = blocks.empty() ? nullptr : blocks.back();
        pindex->nTime = 1415926536 + 600 * pindex->nHeight;
        pindex->nVersion = nVersion;
        pindex->BuildSkip();
        blocks.push_back(pindex);
        return pindex;
    };

    // Period 0: DEFINED (blocks 0-143)
    for (int i = 0; i < 144; i++) {
        mine_block(0);
    }
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::STARTED);

    // Period 1: STARTED, signal all blocks to lock in (blocks 144-287)
    for (int i = 0; i < 144; i++) {
        mine_block(VERSIONBITS_TOP_BITS | 0x100); // Signal
    }
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::LOCKED_IN);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks.back()), 288);

    // Period 2: LOCKED_IN (blocks 288-431)
    for (int i = 0; i < 144; i++) {
        mine_block(0);
    }
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::ACTIVE);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks.back()), 432);

    // Period 3: ACTIVE (blocks 432-575), activation_height = 432
    for (int i = 0; i < 144; i++) {
        mine_block(0);
    }
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::ACTIVE);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks.back()), 432);

    // Period 4 (blocks 576-719): blocks in this period are ACTIVE,
    // but at pindexPrev=719 the state for block 720+ is EXPIRED (720 >= 432 + 288)
    for (int i = 0; i < 144; i++) {
        mine_block(0);
    }
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::EXPIRED);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks.back()), 720);

    // Verify EXPIRED is terminal
    for (int i = 0; i < 144; i++) {
        mine_block(VERSIONBITS_TOP_BITS | 0x100); // Signal shouldn't matter
    }
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::EXPIRED);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks.back()), 720);

    cleanup();
}

BOOST_AUTO_TEST_CASE(versionbits_expired_unaligned_duration_rejected)
{
    // Test that active_duration that is NOT a multiple of the period is rejected.
    ArgsManager args;
    args.ForceSetArg("-vbparams", "testdummy:0:9223372036854775807:0:432:200");
    BOOST_CHECK_THROW(CreateChainParams(args, ChainType::REGTEST), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(versionbits_expired_minimum_duration)
{
    // Test active_duration = 1 period (144). Minimum useful duration.
    // Activation at 432, so 432+144=576. At pindexPrev=575, state for 576+:
    // 576 >= 576 -> EXPIRED. Only one period of ACTIVE.

    std::vector<CBlockIndex*> blocks;
    auto cleanup = [&blocks]() {
        for (auto* b : blocks) delete b;
        blocks.clear();
    };

    const auto dep = MakeTemporaryDeployment(144);
    TestConditionChecker checker(dep);

    auto mine_block = [&blocks](int32_t nVersion) -> CBlockIndex* {
        CBlockIndex* pindex = new CBlockIndex();
        pindex->nHeight = blocks.size();
        pindex->pprev = blocks.empty() ? nullptr : blocks.back();
        pindex->nTime = 1415926536 + 600 * pindex->nHeight;
        pindex->nVersion = nVersion;
        pindex->BuildSkip();
        blocks.push_back(pindex);
        return pindex;
    };

    // Period 0: DEFINED (0-143)
    for (int i = 0; i < 144; i++) mine_block(0);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::STARTED);

    // Period 1: Signal (144-287)
    for (int i = 0; i < 144; i++) mine_block(VERSIONBITS_TOP_BITS | 0x100);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::LOCKED_IN);

    // Period 2: LOCKED_IN (288-431)
    for (int i = 0; i < 144; i++) mine_block(0);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::ACTIVE);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks.back()), 432);

    // Period 3 (432-575): ACTIVE for this period, but state for 576+:
    // 576 >= 432 + 144 = 576 -> EXPIRED immediately at next boundary
    for (int i = 0; i < 144; i++) mine_block(0);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::EXPIRED);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks.back()), 576);

    cleanup();
}

BOOST_AUTO_TEST_CASE(versionbits_expired_cold_cache)
{
    // Test that clearing the cache and re-querying correctly recovers
    // the EXPIRED state. This exercises the activation_height recovery
    // path in GetStateFor where it calls GetStateSinceHeightFor to find
    // when ACTIVE started.

    std::vector<CBlockIndex*> blocks;
    auto cleanup = [&blocks]() {
        for (auto* b : blocks) delete b;
        blocks.clear();
    };

    const auto dep = MakeTemporaryDeployment(288);
    TestConditionChecker checker(dep);

    auto mine_block = [&blocks](int32_t nVersion) -> CBlockIndex* {
        CBlockIndex* pindex = new CBlockIndex();
        pindex->nHeight = blocks.size();
        pindex->pprev = blocks.empty() ? nullptr : blocks.back();
        pindex->nTime = 1415926536 + 600 * pindex->nHeight;
        pindex->nVersion = nVersion;
        pindex->BuildSkip();
        blocks.push_back(pindex);
        return pindex;
    };

    // Build chain through to EXPIRED (same as basic test)
    // Period 0: DEFINED (0-143)
    for (int i = 0; i < 144; i++) mine_block(0);
    // Period 1: Signal (144-287)
    for (int i = 0; i < 144; i++) mine_block(VERSIONBITS_TOP_BITS | 0x100);
    // Period 2: LOCKED_IN (288-431)
    for (int i = 0; i < 144; i++) mine_block(0);
    // Period 3: ACTIVE (432-575)
    for (int i = 0; i < 144; i++) mine_block(0);
    // Period 4: (576-719) -> EXPIRED at 720
    for (int i = 0; i < 144; i++) mine_block(0);

    // Verify EXPIRED with warm cache
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::EXPIRED);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks.back()), 720);

    // Clear cache completely — simulates node restart
    checker.clear();

    // Re-query: must walk from genesis, recover activation_height, compute EXPIRED
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::EXPIRED);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks.back()), 720);

    // Also verify intermediate states are correct after cache rebuild
    checker.clear();
    // Query at end of period 3 (pindexPrev=575) — should be ACTIVE
    BOOST_CHECK(checker.StateFor(blocks[575]) == ThresholdState::ACTIVE);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks[575]), 432);

    // Now query at end of period 4 with partially-populated cache
    // (period 3 is cached as ACTIVE from the query above)
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::EXPIRED);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks.back()), 720);

    cleanup();
}

BOOST_AUTO_TEST_CASE(versionbits_expired_with_max_activation_height)
{
    // Test interaction: deployment with BOTH max_activation_height AND active_duration.
    // max_activation_height forces LOCKED_IN, then active_duration causes EXPIRED.
    //
    // period=144, max_activation_height=432, active_duration=288
    // - Period 0 (0-143): DEFINED
    // - Period 1 (144-287): STARTED (no signaling, but forced LOCKED_IN at boundary
    //   because 288 >= 432 - 144)
    // - Period 2 (288-431): LOCKED_IN
    // - Period 3 (432-575): ACTIVE (activation_height=432)
    // - Period 4 (576-719): ACTIVE
    // - At pindexPrev=719: EXPIRED (720 >= 432 + 288)

    std::vector<CBlockIndex*> blocks;
    auto cleanup = [&blocks]() {
        for (auto* b : blocks) delete b;
        blocks.clear();
    };

    const auto dep = MakeTemporaryDeployment(288, /*max_activation_height=*/432);
    TestConditionChecker checker(dep);

    auto mine_block = [&blocks](int32_t nVersion) -> CBlockIndex* {
        CBlockIndex* pindex = new CBlockIndex();
        pindex->nHeight = blocks.size();
        pindex->pprev = blocks.empty() ? nullptr : blocks.back();
        pindex->nTime = 1415926536 + 600 * pindex->nHeight;
        pindex->nVersion = nVersion;
        pindex->BuildSkip();
        blocks.push_back(pindex);
        return pindex;
    };

    // Period 0: DEFINED (0-143), no signaling
    for (int i = 0; i < 144; i++) mine_block(0);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::STARTED);

    // Period 1: STARTED (144-287), no signaling — forced LOCKED_IN by max_activation_height
    for (int i = 0; i < 144; i++) mine_block(0);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::LOCKED_IN);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks.back()), 288);

    // Period 2: LOCKED_IN (288-431)
    for (int i = 0; i < 144; i++) mine_block(0);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::ACTIVE);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks.back()), 432);

    // Period 3: ACTIVE (432-575)
    for (int i = 0; i < 144; i++) mine_block(0);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::ACTIVE);

    // Period 4: (576-719) -> EXPIRED at 720
    for (int i = 0; i < 144; i++) mine_block(0);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::EXPIRED);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks.back()), 720);

    cleanup();
}

BOOST_AUTO_TEST_CASE(versionbits_expired_zero_duration)
{
    // Test active_duration = 0. This means the deployment expires immediately
    // after activation — zero blocks of enforcement.
    // Activation at 432, so 432 + 0 = 432. At pindexPrev=431 (end of LOCKED_IN),
    // state for 432+: LOCKED_IN transitions to ACTIVE, sets activation_height=432.
    // But that's computed for this period boundary. The ACTIVE->EXPIRED check
    // happens on the NEXT iteration of the walk-forward.
    // At pindexPrev=575 (end of period 3): 576 >= 432 + 0 = 432 -> EXPIRED.
    // So active_duration=0 gives exactly one period of ACTIVE, same as
    // active_duration=144 with period=144 (since transitions are per-period).

    std::vector<CBlockIndex*> blocks;
    auto cleanup = [&blocks]() {
        for (auto* b : blocks) delete b;
        blocks.clear();
    };

    const auto dep = MakeTemporaryDeployment(0);
    TestConditionChecker checker(dep);

    auto mine_block = [&blocks](int32_t nVersion) -> CBlockIndex* {
        CBlockIndex* pindex = new CBlockIndex();
        pindex->nHeight = blocks.size();
        pindex->pprev = blocks.empty() ? nullptr : blocks.back();
        pindex->nTime = 1415926536 + 600 * pindex->nHeight;
        pindex->nVersion = nVersion;
        pindex->BuildSkip();
        blocks.push_back(pindex);
        return pindex;
    };

    // Period 0: DEFINED (0-143)
    for (int i = 0; i < 144; i++) mine_block(0);
    // Period 1: Signal (144-287)
    for (int i = 0; i < 144; i++) mine_block(VERSIONBITS_TOP_BITS | 0x100);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::LOCKED_IN);

    // Period 2: LOCKED_IN (288-431) -> ACTIVE at 432
    for (int i = 0; i < 144; i++) mine_block(0);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::ACTIVE);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks.back()), 432);

    // Period 3 (432-575): ACTIVE, but 576 >= 432+0 -> EXPIRED at next boundary
    for (int i = 0; i < 144; i++) mine_block(0);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::EXPIRED);
    BOOST_CHECK_EQUAL(checker.StateSinceHeightFor(blocks.back()), 576);

    cleanup();
}

BOOST_AUTO_TEST_CASE(versionbits_no_signaling_after_expired)
{
    // Test that ComputeBlockVersion does NOT set the deployment bit after EXPIRED.
    // Uses regtest with testdummy deployment configured with active_duration.
    //
    // ComputeBlockVersion only signals for STARTED and LOCKED_IN states.
    // After EXPIRED, the bit should not be set.

    ArgsManager args;
    // testdummy: start=0, timeout=never, min_height=0, max_height=INT_MAX, active_duration=144
    args.ForceSetArg("-vbparams", "testdummy:0:9223372036854775807:0:2147483647:144");
    const auto chainParams = CreateChainParams(args, ChainType::REGTEST);
    const auto& params = chainParams->GetConsensus();

    VersionBitsCache vbcache;
    const auto dep = Consensus::DEPLOYMENT_TESTDUMMY;
    TestConditionChecker checker(params.vDeployments[dep]);
    const uint32_t bitmask = uint32_t{1} << params.vDeployments[dep].bit;
    const int period = params.vDeployments[dep].period;

    std::vector<CBlockIndex*> blocks;
    auto cleanup = [&blocks]() {
        for (auto* b : blocks) delete b;
        blocks.clear();
    };

    auto mine_block = [&blocks](int32_t nVersion) -> CBlockIndex* {
        CBlockIndex* pindex = new CBlockIndex();
        pindex->nHeight = blocks.size();
        pindex->pprev = blocks.empty() ? nullptr : blocks.back();
        pindex->nTime = 1415926536 + 600 * pindex->nHeight;
        pindex->nVersion = nVersion;
        pindex->BuildSkip();
        blocks.push_back(pindex);
        return pindex;
    };

    // Period 0: DEFINED (0-143) — bit should not be set
    for (int i = 0; i < period; i++) mine_block(VERSIONBITS_TOP_BITS);
    BOOST_CHECK_EQUAL(vbcache.ComputeBlockVersion(blocks.back(), params) & bitmask, bitmask); // STARTED, bit set

    // Period 1: STARTED — signal to lock in (144-287), bit should be set
    for (int i = 0; i < period; i++) mine_block(VERSIONBITS_TOP_BITS | bitmask);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::LOCKED_IN);
    BOOST_CHECK_EQUAL(vbcache.ComputeBlockVersion(blocks.back(), params) & bitmask, bitmask); // LOCKED_IN, bit set

    // Period 2: LOCKED_IN (288-431), bit should be set
    for (int i = 0; i < period; i++) mine_block(VERSIONBITS_TOP_BITS | bitmask);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::ACTIVE);
    // ACTIVE: bit should NOT be set
    BOOST_CHECK_EQUAL(vbcache.ComputeBlockVersion(blocks.back(), params) & bitmask, 0u);

    // Period 3: ACTIVE (432-575)
    for (int i = 0; i < period; i++) mine_block(VERSIONBITS_TOP_BITS);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::EXPIRED);
    // EXPIRED: bit should NOT be set
    BOOST_CHECK_EQUAL(vbcache.ComputeBlockVersion(blocks.back(), params) & bitmask, 0u);

    // Period 4: EXPIRED (576+) — verify bit stays off
    for (int i = 0; i < period; i++) mine_block(VERSIONBITS_TOP_BITS);
    BOOST_CHECK(checker.StateFor(blocks.back()) == ThresholdState::EXPIRED);
    BOOST_CHECK_EQUAL(vbcache.ComputeBlockVersion(blocks.back(), params) & bitmask, 0u);

    cleanup();
}

BOOST_AUTO_TEST_SUITE_END()
