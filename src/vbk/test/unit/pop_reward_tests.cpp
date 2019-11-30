#include <boost/test/unit_test.hpp>
#include <chainparams.h>
#include <test/setup_common.h>
#include <validation.h>
#include <wallet/wallet.h>
#include <interfaces/chain.h>
#include <script/interpreter.h>

#include <vbk/service_locator.hpp>
#include <vbk/test/util/mock.hpp>

#include <fakeit.hpp>
using namespace fakeit;

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


BOOST_FIXTURE_TEST_CASE(addPopPayoutsIntoCoinbaseTx_test, TestChain100Setup)
{
    VeriBlockTest::ServicesFixture serviceFixture;

    VeriBlock::PoPRewards rewards = getRewards();

    When(Method(serviceFixture.util_service_mock, getPopRewards)).AlwaysDo([&rewards](const CBlockIndex& pindexPrev) -> VeriBlock::PoPRewards {
        return rewards;
    });

    When(Method(serviceFixture.util_service_mock, addPopPayoutsIntoCoinbaseTx)).AlwaysDo([&serviceFixture](CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev) {
        serviceFixture.util_service_impl.addPopPayoutsIntoCoinbaseTx(coinbaseTx, pindexPrev);
    });

    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);

    CAmount PoWReward = GetBlockSubsidy(ChainActive().Height(), Params().GetConsensus());

    BOOST_CHECK((PoWReward + 105) == block.vtx[0]->GetValueOut());

    for (auto i = 0u; i < rewards.size(); ++i) {
        CScript payoutInfo = block.vtx[0]->vout[i + 1].scriptPubKey;
        BOOST_CHECK(block.vtx[0]->vout[i + 1].nValue == rewards[payoutInfo]);
    }
}

BOOST_FIXTURE_TEST_CASE(checkCoinbaseTxWithPopRewards, TestChain100Setup)
{
    VeriBlockTest::ServicesFixture serviceFixture;

    VeriBlock::PoPRewards rewards = getRewards();

    When(Method(serviceFixture.util_service_mock, getPopRewards)).AlwaysDo([rewards](const CBlockIndex& pindexPrev) -> VeriBlock::PoPRewards {
        return rewards;
    });

    When(Method(serviceFixture.util_service_mock, addPopPayoutsIntoCoinbaseTx)).AlwaysDo([&serviceFixture](CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev) {
        serviceFixture.util_service_impl.addPopPayoutsIntoCoinbaseTx(coinbaseTx, pindexPrev);
    });

    When(Method(serviceFixture.util_service_mock, checkCoinbaseTxWithPopRewards)).AlwaysDo([&serviceFixture](const CTransaction& tx, const CAmount& PoWBlockReward, const CBlockIndex& pindexPrev, CValidationState& state) -> bool {
        return serviceFixture.util_service_impl.checkCoinbaseTxWithPopRewards(tx, PoWBlockReward, pindexPrev, state);
    });

    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);

    BOOST_CHECK(block.GetHash() == ChainActive().Tip()->GetBlockHash()); // means that pop rewards are valid
    Verify_Method(Method(serviceFixture.util_service_mock, checkCoinbaseTxWithPopRewards)).AtLeastOnce();

    When(Method(serviceFixture.util_service_mock, checkCoinbaseTxWithPopRewards)).AlwaysReturn(false);

    BOOST_CHECK_THROW(CreateAndProcessBlock({}, scriptPubKey), std::runtime_error);

    Verify_Method(Method(serviceFixture.util_service_mock, checkCoinbaseTxWithPopRewards)).AtLeastOnce();
}

BOOST_FIXTURE_TEST_CASE(check_wallet_balance_with_pop_reward, TestChain100Setup)
{
    VeriBlockTest::ServicesFixture serviceFixture;
    
    CAmount PoPReward = 56;
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    VeriBlock::PoPRewards rewards;
    rewards[scriptPubKey] = PoPReward;

    When(Method(serviceFixture.util_service_mock, getPopRewards)).AlwaysDo([rewards](const CBlockIndex& pindexPrev) -> VeriBlock::PoPRewards {
        return rewards;
    });

    When(Method(serviceFixture.util_service_mock, addPopPayoutsIntoCoinbaseTx)).AlwaysDo([&serviceFixture](CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev) {
        serviceFixture.util_service_impl.addPopPayoutsIntoCoinbaseTx(coinbaseTx, pindexPrev);
    });

    CreateAndProcessBlock({}, scriptPubKey);

    CAmount PoWReward = GetBlockSubsidy(ChainActive().Height(), Params().GetConsensus());

    CBlockIndex* tip = ::ChainActive().Tip();
    auto chain = interfaces::MakeChain();
    LockAssertion lock(::cs_main);

    {
        CWallet wallet(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        {
            LOCK(wallet.cs_wallet);
            wallet.AddKeyPubKey(coinbaseKey, coinbaseKey.GetPubKey()); // add Pubkey to wallet
        }
        WalletRescanReserver reserver(&wallet);
        reserver.reserve();
        CWallet::ScanResult result = wallet.ScanForWalletTransactions(tip->GetBlockHash() /* start_block */, {} /* stop_block */, reserver, false /* update */);

        BOOST_CHECK_EQUAL(result.status, CWallet::ScanResult::SUCCESS);
        BOOST_CHECK(result.last_failed_block.IsNull());
        BOOST_CHECK_EQUAL(result.last_scanned_block, tip->GetBlockHash());
        BOOST_CHECK_EQUAL(*result.last_scanned_height, tip->nHeight);
        BOOST_CHECK_EQUAL(wallet.GetBalance().m_mine_immature, PoWReward + PoPReward);
    }
}

BOOST_AUTO_TEST_SUITE_END()
