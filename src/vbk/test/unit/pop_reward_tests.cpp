// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <script/interpreter.h>
#include <vbk/test/util/e2e_fixture.hpp>

struct PopRewardsTestFixture : public E2eFixture {
};

BOOST_AUTO_TEST_SUITE(pop_reward_tests)

BOOST_FIXTURE_TEST_CASE(addPopPayoutsIntoCoinbaseTx_test, PopRewardsTestFixture)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;

    auto tip = ChainActive().Tip();
    BOOST_CHECK(tip != nullptr);
    std::vector<uint8_t> payoutInfo{scriptPubKey.begin(), scriptPubKey.end()};
    CBlock block = endorseAltBlockAndMine(tip->GetBlockHash(), tip->GetBlockHash(), payoutInfo, 0);
    {
        LOCK(cs_main);
        BOOST_CHECK(ChainActive().Tip()->GetBlockHash() == block.GetHash());
    }

    // Generate a chain whith rewardInterval of blocks
    int rewardInterval = (int)VeriBlock::GetPop().config->alt->getPayoutParams().getPopPayoutDelay();
    // do not add block with rewards
    // do not add block before block with rewards
    for (int i = 0; i < (rewardInterval - 3); i++) {
        CBlock b = CreateAndProcessBlock({}, scriptPubKey);
        m_coinbase_txns.push_back(b.vtx[0]);
    }

    CBlock beforePayoutBlock = CreateAndProcessBlock({}, scriptPubKey);

    int n = 0;
    for (const auto& out : beforePayoutBlock.vtx[0]->vout) {
        if (out.nValue > 0) n++;
    }
    BOOST_CHECK(n == 1);

    CBlock payoutBlock = CreateAndProcessBlock({}, scriptPubKey);
    n = 0;
    for (const auto& out : payoutBlock.vtx[0]->vout) {
        if (out.nValue > 0) n++;
    }

    // we've got additional coinbase out
    BOOST_CHECK(n > 1);

    // assume POP reward is the output after the POW reward
    BOOST_CHECK(payoutBlock.vtx[0]->vout[1].scriptPubKey == scriptPubKey);
    BOOST_CHECK(payoutBlock.vtx[0]->vout[1].nValue > 0);

    CMutableTransaction spending;
    spending.nVersion = 1;
    spending.vin.resize(1);
    spending.vin[0].prevout.hash = payoutBlock.vtx[0]->GetHash();
    // use POP payout as an input
    spending.vin[0].prevout.n = 1;
    spending.vout.resize(1);
    spending.vout[0].nValue = 100;
    spending.vout[0].scriptPubKey = scriptPubKey;

    std::vector<unsigned char> vchSig;
    uint256 hash = SignatureHash(scriptPubKey, spending, 0, SIGHASH_ALL, 0, SigVersion::BASE);
    BOOST_CHECK(coinbaseKey.Sign(hash, vchSig));
    vchSig.push_back((unsigned char)SIGHASH_ALL);
    spending.vin[0].scriptSig << vchSig;

    CBlock spendingBlock;
    // make sure we cannot spend till coinbase maturity
    spendingBlock = CreateAndProcessBlock({spending}, scriptPubKey);
    {
        LOCK(cs_main);
        BOOST_CHECK(ChainActive().Tip()->GetBlockHash() != spendingBlock.GetHash());
    }

    for (int i = 0; i < COINBASE_MATURITY; i++) {
        CBlock b = CreateAndProcessBlock({}, scriptPubKey);
        m_coinbase_txns.push_back(b.vtx[0]);
    }

    spendingBlock = CreateAndProcessBlock({spending}, scriptPubKey);
    {
        LOCK(cs_main);
        BOOST_CHECK(ChainActive().Tip()->GetBlockHash() == spendingBlock.GetHash());
    }
}

//BOOST_FIXTURE_TEST_CASE(addPopPayoutsIntoCoinbaseTx_test, PopRewardsTestFixture)
//{
//    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
//
//    while (ChainActive().Height() < VeriBlock::getService<VeriBlock::Config>().POP_REWARD_PAYMENT_DELAY) {
//        CreateAndProcessBlock({}, scriptPubKey);
//    }
//
//    VeriBlock::PoPRewards rewards = getRewards();
//    ON_CALL(pop_service_mock, rewardsCalculateOutputs)
//        .WillByDefault(
//            [&rewards](const int& blockHeight,
//              const CBlockIndex& endorsedBlock,
//              const CBlockIndex& contaningBlocksTip,
//              const CBlockIndex* difficulty_start_interval,
//              const CBlockIndex* difficulty_end_interval,
//              std::map<CScript, int64_t>& outputs) {
//                outputs = rewards;
//              });
//
//    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
//
//    int n = 0;
//    for (const auto& out : block.vtx[0]->vout) {
//        if (rewards.find(out.scriptPubKey) != rewards.end()) {
//            n++;
//        }
//    }
//    // have found 4 pop rewards
//    BOOST_CHECK(n == 4);
//}
//
//BOOST_FIXTURE_TEST_CASE(checkCoinbaseTxWithPopRewards, PopRewardsTestFixture)
//{
//    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
//
//    while (ChainActive().Height() < VeriBlock::getService<VeriBlock::Config>().POP_REWARD_PAYMENT_DELAY) {
//        CreateAndProcessBlock({}, scriptPubKey);
//    }
//
//    VeriBlock::PoPRewards rewards = getRewards();
//    ON_CALL(pop_service_mock, rewardsCalculateOutputs)
//        .WillByDefault(
//            [&rewards](const int& blockHeight,
//              const CBlockIndex& endorsedBlock,
//              const CBlockIndex& contaningBlocksTip,
//              const CBlockIndex* difficulty_start_interval,
//              const CBlockIndex* difficulty_end_interval,
//              std::map<CScript, int64_t>& outputs) {
//                outputs = rewards;
//              });
//    EXPECT_CALL(util_service_mock, checkCoinbaseTxWithPopRewards).Times(testing::AtLeast(1));
//
//    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
//
//    BOOST_CHECK(block.GetHash() == ChainActive().Tip()->GetBlockHash()); // means that pop rewards are valid
//    testing::Mock::VerifyAndClearExpectations(&util_service_mock);
//
//    ON_CALL(util_service_mock, checkCoinbaseTxWithPopRewards).WillByDefault(Return(false));
//    EXPECT_CALL(util_service_mock, checkCoinbaseTxWithPopRewards).Times(testing::AtLeast(1));
//
//    BOOST_CHECK_THROW(CreateAndProcessBlock({}, scriptPubKey), std::runtime_error);
//}
//
//BOOST_FIXTURE_TEST_CASE(pop_reward_halving_test, PopRewardsTestFixture)
//{
//    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
//
//    while (ChainActive().Height() < Params().GetConsensus().nSubsidyHalvingInterval || ChainActive().Height() < VeriBlock::getService<VeriBlock::Config>().POP_REWARD_PAYMENT_DELAY) {
//        CreateAndProcessBlock({}, scriptPubKey);
//    }
//
//    VeriBlock::PoPRewards rewards;
//    CScript pop_payout = CScript() << std::vector<uint8_t>(5, 1);
//    rewards[pop_payout] = 50;
//
//   ON_CALL(pop_service_mock, rewardsCalculateOutputs)
//        .WillByDefault(
//            [&rewards](const int& blockHeight,
//              const CBlockIndex& endorsedBlock,
//              const CBlockIndex& contaningBlocksTip,
//              const CBlockIndex* difficulty_start_interval,
//              const CBlockIndex* difficulty_end_interval,
//              std::map<CScript, int64_t>& outputs) {
//                outputs = rewards;
//              });
//
//    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
//
//    for (const auto& out : block.vtx[0]->vout) {
//        if (out.scriptPubKey == pop_payout) {
//            // 3 times halving (50 / 8)
//            BOOST_CHECK(6 == out.nValue);
//        }
//    }
//}
//
//BOOST_FIXTURE_TEST_CASE(check_wallet_balance_with_pop_reward, PopRewardsTestFixture)
//{
//    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
//    VeriBlock::PoPRewards rewards;
//    rewards[scriptPubKey] = 56;
//
//    while (ChainActive().Height() < VeriBlock::getService<VeriBlock::Config>().POP_REWARD_PAYMENT_DELAY) {
//        CreateAndProcessBlock({}, scriptPubKey);
//    }
//
//    ON_CALL(pop_service_mock, rewardsCalculateOutputs)
//        .WillByDefault(
//            [&rewards](const int& blockHeight,
//              const CBlockIndex& endorsedBlock,
//              const CBlockIndex& contaningBlocksTip,
//              const CBlockIndex* difficulty_start_interval,
//              const CBlockIndex* difficulty_end_interval,
//              std::map<CScript, int64_t>& outputs) {
//                outputs = rewards;
//            });
//
//    auto block = CreateAndProcessBlock({}, scriptPubKey);
//
//    CAmount PoWReward = GetBlockSubsidy(ChainActive().Height(), Params().GetConsensus());
//
//    CBlockIndex* tip = ::ChainActive().Tip();
//    NodeContext node;
//    node.chain = interfaces::MakeChain(node);
//    auto& chain = *node.chain;
//    {
//        CWallet wallet(&chain, WalletLocation(), WalletDatabase::CreateDummy());
//
//        {
//            LOCK(wallet.GetLegacyScriptPubKeyMan()->cs_wallet);
//            // add Pubkey to wallet
//            BOOST_REQUIRE(wallet.GetLegacyScriptPubKeyMan()->AddKeyPubKey(coinbaseKey, coinbaseKey.GetPubKey()));
//        }
//
//        WalletRescanReserver reserver(&wallet);
//        reserver.reserve();
//        CWallet::ScanResult result = wallet.ScanForWalletTransactions(tip->GetBlockHash() /* start_block */, {} /* stop_block */, reserver, false /* update */);
//
//        {
//            LOCK(wallet.cs_wallet);
//            wallet.SetLastBlockProcessed(tip->nHeight, tip->GetBlockHash());
//        }
//
//        BOOST_CHECK_EQUAL(result.status, CWallet::ScanResult::SUCCESS);
//        BOOST_CHECK(result.last_failed_block.IsNull());
//        BOOST_CHECK_EQUAL(result.last_scanned_block, tip->GetBlockHash());
//        BOOST_CHECK_EQUAL(*result.last_scanned_height, tip->nHeight);
//        // 3 times halving (56 / 8)
//        BOOST_CHECK_EQUAL(wallet.GetBalance().m_mine_immature, PoWReward + 7);
//    }
//}

BOOST_AUTO_TEST_SUITE_END()
