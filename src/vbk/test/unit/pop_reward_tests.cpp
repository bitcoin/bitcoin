#include <boost/test/unit_test.hpp>
#include <chainparams.h>
#include <interfaces/chain.h>
#include <script/interpreter.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <wallet/wallet.h>

#include <vbk/config.hpp>
#include <vbk/pop_service/pop_service_impl.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/test/util/mock.hpp>

#include <fakeit.hpp>
using namespace fakeit;

struct PopRewardsTestFixture : public TestChain100Setup, VeriBlockTest::ServicesFixture {
    //VeriBlockTest::ServicesFixture  service_fixture;

    PopRewardsTestFixture()
    {
        When(Method(util_service_mock, getPopRewards)).AlwaysDo([&](const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams) {
            return util_service_impl.getPopRewards(pindexPrev, consensusParams);
        });

        When(Method(util_service_mock, addPopPayoutsIntoCoinbaseTx)).AlwaysDo([&](CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams) {
            util_service_impl.addPopPayoutsIntoCoinbaseTx(coinbaseTx, pindexPrev, consensusParams);
        });

        When(Method(util_service_mock, checkCoinbaseTxWithPopRewards)).AlwaysDo([&](const CTransaction& tx, const CAmount& PoWBlockReward, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams, BlockValidationState& state) -> bool {
            return util_service_impl.checkCoinbaseTxWithPopRewards(tx, PoWBlockReward, pindexPrev, consensusParams, state);
        });
    }
};

BOOST_AUTO_TEST_SUITE(pop_reward_tests)

static VeriBlock::PoPRewards getRewards()
{
    CScript payout1 = CScript() << std::vector<uint8_t>(5, 1);
    CScript payout2 = CScript() << std::vector<uint8_t>(5, 2);
    CScript payout3 = CScript() << std::vector<uint8_t>(5, 3);
    CScript payout4 = CScript() << std::vector<uint8_t>(5, 4);

    VeriBlock::PoPRewards rewards;
    rewards[payout1] = 24;
    rewards[payout2] = 13;
    rewards[payout3] = 12;
    rewards[payout4] = 56;

    return rewards;
}

BOOST_FIXTURE_TEST_CASE(addPopPayoutsIntoCoinbaseTx_test, PopRewardsTestFixture)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;

    while (ChainActive().Height() < VeriBlock::getService<VeriBlock::Config>().POP_REWARD_PAYMENT_DELAY) {
        CreateAndProcessBlock({}, scriptPubKey);
    }

    VeriBlock::PoPRewards rewards = getRewards();
    When(Method(pop_service_mock, rewardsCalculateOutputs)).AlwaysDo([&rewards](const int& blockHeight, const CBlockIndex& endorsedBlock, const CBlockIndex& contaningBlocksTip, const CBlockIndex* difficulty_start_interval, const CBlockIndex* difficulty_end_interval, std::map<CScript, int64_t>& outputs) {
        outputs = rewards;
    });

    CBlock block = CreateAndProcessBlock({}, scriptPubKey);

    CAmount PoWReward = GetBlockSubsidy(ChainActive().Height(), Params().GetConsensus());

    int n = 0;
    for (const auto& out : block.vtx[0]->vout) {
        if (rewards.find(out.scriptPubKey) != rewards.end()) {
            n++;
        }
    }
    // have found 4 pop rewards
    BOOST_CHECK(n == 4);
}

BOOST_FIXTURE_TEST_CASE(checkCoinbaseTxWithPopRewards, PopRewardsTestFixture)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;

    while (ChainActive().Height() < VeriBlock::getService<VeriBlock::Config>().POP_REWARD_PAYMENT_DELAY) {
        CreateAndProcessBlock({}, scriptPubKey);
    }

    VeriBlock::PoPRewards rewards = getRewards();
    When(Method(pop_service_mock, rewardsCalculateOutputs)).AlwaysDo([&rewards](const int& blockHeight, const CBlockIndex& endorsedBlock, const CBlockIndex& contaningBlocksTip, const CBlockIndex* difficulty_start_interval, const CBlockIndex* difficulty_end_interval, std::map<CScript, int64_t>& outputs) {
        outputs = rewards;
    });

    CBlock block = CreateAndProcessBlock({}, scriptPubKey);

    BOOST_CHECK(block.GetHash() == ChainActive().Tip()->GetBlockHash()); // means that pop rewards are valid
    Verify_Method(Method(util_service_mock, checkCoinbaseTxWithPopRewards)).AtLeastOnce();

    When(Method(util_service_mock, checkCoinbaseTxWithPopRewards)).AlwaysReturn(false);

    BOOST_CHECK_THROW(CreateAndProcessBlock({}, scriptPubKey), std::runtime_error);

    Verify_Method(Method(util_service_mock, checkCoinbaseTxWithPopRewards)).AtLeastOnce();
}

BOOST_FIXTURE_TEST_CASE(pop_reward_halving_test, PopRewardsTestFixture)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;

    while (ChainActive().Height() < Params().GetConsensus().nSubsidyHalvingInterval || ChainActive().Height() < VeriBlock::getService<VeriBlock::Config>().POP_REWARD_PAYMENT_DELAY) {
        CreateAndProcessBlock({}, scriptPubKey);
    }

    VeriBlock::PoPRewards rewards;
    CScript pop_payout = CScript() << std::vector<uint8_t>(5, 1);
    rewards[pop_payout] = 50;

    When(Method(pop_service_mock, rewardsCalculateOutputs)).AlwaysDo([&rewards](const int& blockHeight, const CBlockIndex& endorsedBlock, const CBlockIndex& contaningBlocksTip, const CBlockIndex* difficulty_start_interval, const CBlockIndex* difficulty_end_interval, std::map<CScript, int64_t>& outputs) {
        outputs = rewards;
    });

    CBlock block = CreateAndProcessBlock({}, scriptPubKey);

    for (const auto& out : block.vtx[0]->vout) {
        if (out.scriptPubKey == pop_payout) {
            // 3 times halving (50 / 8)
            BOOST_CHECK(6 == out.nValue);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(check_wallet_balance_with_pop_reward, PopRewardsTestFixture)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    VeriBlock::PoPRewards rewards;
    rewards[scriptPubKey] = 56;

    while (ChainActive().Height() < VeriBlock::getService<VeriBlock::Config>().POP_REWARD_PAYMENT_DELAY) {
        CreateAndProcessBlock({}, scriptPubKey);
    }

    When(Method(pop_service_mock, rewardsCalculateOutputs)).AlwaysDo([&rewards](const int& blockHeight, const CBlockIndex& endorsedBlock, const CBlockIndex& contaningBlocksTip, const CBlockIndex* difficulty_start_interval, const CBlockIndex* difficulty_end_interval, std::map<CScript, int64_t>& outputs) {
        outputs = rewards;
    });

    auto block = CreateAndProcessBlock({}, scriptPubKey);

    CAmount PoWReward = GetBlockSubsidy(ChainActive().Height(), Params().GetConsensus());

    CBlockIndex* tip = ::ChainActive().Tip();
    NodeContext node;
    node.chain = interfaces::MakeChain(node);
    auto& chain = *node.chain;
    {
        CWallet wallet(&chain, WalletLocation(), WalletDatabase::CreateDummy());
        {
            LOCK(wallet.cs_wallet);
            // add Pubkey to wallet
            BOOST_REQUIRE(wallet.GetLegacyScriptPubKeyMan()->AddKeyPubKey(coinbaseKey, coinbaseKey.GetPubKey()));
        }

        WalletRescanReserver reserver(&wallet);
        reserver.reserve();
        CWallet::ScanResult result = wallet.ScanForWalletTransactions(tip->GetBlockHash() /* start_block */, {} /* stop_block */, reserver, false /* update */);

        {
            LOCK(wallet.cs_wallet);
            wallet.SetLastBlockProcessed(tip->nHeight, tip->GetBlockHash());
        }

        BOOST_CHECK_EQUAL(result.status, CWallet::ScanResult::SUCCESS);
        BOOST_CHECK(result.last_failed_block.IsNull());
        BOOST_CHECK_EQUAL(result.last_scanned_block, tip->GetBlockHash());
        BOOST_CHECK_EQUAL(*result.last_scanned_height, tip->nHeight);
        // 3 times halving (56 / 8)
        BOOST_CHECK_EQUAL(wallet.GetBalance().m_mine_immature, PoWReward + 7);
    }
}
BOOST_AUTO_TEST_SUITE_END()
