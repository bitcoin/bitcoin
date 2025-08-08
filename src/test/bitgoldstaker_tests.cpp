#include <wallet/bitgoldstaker.h>
#include <wallet/test/util.h>
#include <pos/stake.h>
#include <test/util/setup_common.h>
#include <consensus/amount.h>
#include <key.h>
#include <script/standard.h>
#include <test/util/logging.h>
#include <wallet/coincontrol.h>
#include <wallet/spend.h>
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

BOOST_AUTO_TEST_CASE(stake_fails_no_utxos)
{
    // Wallet without any matching UTXOs
    CKey unused_key;
    unused_key.MakeNewKey(/* compressed = */ true);
    auto wallet = wallet::CreateSyncedWallet(*m_node.chain,
        WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain()),
        unused_key);

    wallet::BitGoldStaker staker(*wallet);
    int start_height = WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Height());
    ASSERT_DEBUG_LOG("BitGoldStaker: no eligible UTXOs");
    staker.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    staker.Stop();
    int end_height = WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Height());
    BOOST_CHECK_EQUAL(start_height, end_height);
}

BOOST_AUTO_TEST_CASE(stake_fails_kernel_check)
{
    // Wallet with coins but kernel hash does not meet difficulty
    auto wallet = wallet::CreateSyncedWallet(*m_node.chain,
        WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain()),
        coinbaseKey);

    {
        LOCK(cs_main);
        m_node.chainman->ActiveChain().Tip()->nBits = 0x1;
    }

    wallet::BitGoldStaker staker(*wallet);
    int start_height = WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Height());
    ASSERT_DEBUG_LOG("BitGoldStaker: kernel check failed");
    staker.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    staker.Stop();
    int end_height = WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Height());
    BOOST_CHECK_EQUAL(start_height, end_height);
}

BOOST_AUTO_TEST_CASE(stake_fails_coin_age_or_depth)
{
    // Create wallet with coins
    auto wallet = wallet::CreateSyncedWallet(*m_node.chain,
        WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain()),
        coinbaseKey);

    // Create unconfirmed transaction so resulting UTXO has depth 0 and age 0
    CRecipient recipient{GetScriptForRawPubKey(coinbaseKey.GetPubKey()), 1 * COIN, false};
    CCoinControl coin_control;
    auto res = CreateTransaction(*wallet, {recipient}, /*change_pos=*/std::nullopt, coin_control);
    BOOST_REQUIRE(res);
    CTransactionRef tx = res->tx;
    wallet->CommitTransaction(tx, {}, {});

    // Lock all other UTXOs leaving only the unconfirmed output
    {
        LOCK(wallet->cs_wallet);
        for (const COutput& out : AvailableCoins(*wallet).All()) {
            if (out.outpoint.hash != tx->GetHash()) wallet->LockCoin(out.outpoint);
        }
    }

    wallet::BitGoldStaker staker(*wallet);
    int start_height = WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Height());
    ASSERT_DEBUG_LOG("BitGoldStaker: no eligible UTXOs");
    staker.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    staker.Stop();
    int end_height = WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain().Height());
    BOOST_CHECK_EQUAL(start_height, end_height);
}

BOOST_AUTO_TEST_SUITE_END()
