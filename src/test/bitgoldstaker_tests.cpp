#include <wallet/bitgoldstaker.h>
#include <wallet/test/util.h>
#include <pos/stake.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(bitgoldstaker_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(stake_block_passes_check)
{
    // Create a wallet synced with the pre-mined chain
    auto wallet = wallet::CreateSyncedWallet(*m_node.chain, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain()), coinbaseKey);

    wallet::BitGoldStaker staker(*wallet);
    staker.Start();

    int start_height = WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Height());
    int target_height = start_height + 1;
    for (int i = 0; i < 50; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        int h = WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Height());
        if (h >= target_height) break;
    }
    staker.Stop();

    LOCK(cs_main);
    CBlockIndex* tip = m_node.chainman->ActiveChain().Tip();
    CBlock block;
    BOOST_REQUIRE(m_node.chainman->m_blockman.ReadBlock(block, *tip));
    const Consensus::Params& consensus = m_node.chainman->GetParams().GetConsensus();
    BOOST_CHECK(CheckProofOfStake(block, tip->pprev, consensus));
}

BOOST_AUTO_TEST_SUITE_END()
