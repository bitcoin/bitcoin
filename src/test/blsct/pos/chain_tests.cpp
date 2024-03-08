// Copyright (c) 2023 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/pos/pos.h>
#include <blsct/pos/proof_logic.h>
#include <blsct/wallet/txfactory.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <core_io.h>
#include <policy/fees.h>
#include <test/util/random.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/receive.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(pos_chain_tests, WalletTestingSetup)

Coin CreateCoin(const blsct::DoublePublicKey& recvAddress)
{
    Coin coin;
    auto out = blsct::CreateOutput(recvAddress, 1000 * COIN, "test", TokenId(), Scalar::Rand(), blsct::CreateOutputType::STAKED_COMMITMENT, 1000 * COIN);
    coin.nHeight = 1;
    coin.out = out.out;
    return coin;
}

BOOST_FIXTURE_TEST_CASE(StakedCommitment, TestBLSCTChain100Setup)
{
    auto setup = SetMemProofSetup<Arith>::Get();

    range_proof::GeneratorsFactory<Mcl> gf;
    range_proof::Generators<Arith> gen = gf.GetInstance(TokenId());

    SeedInsecureRand(SeedRand::ZEROS);
    CCoinsViewDB base{{.path = "test", .cache_bytes = 1 << 23, .memory_only = true}, {}};

    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    wallet.InitWalletFlags(wallet::WALLET_FLAG_BLSCT);

    LOCK(wallet.cs_wallet);
    auto blsct_km = wallet.GetOrCreateBLSCTKeyMan();
    BOOST_CHECK(blsct_km->SetupGeneration(true));

    auto recvAddress = std::get<blsct::DoublePublicKey>(blsct_km->GetNewDestination(0).value());

    COutPoint outpoint{Txid::FromUint256(InsecureRand256()), /*nIn=*/0};
    COutPoint outpoint2{Txid::FromUint256(InsecureRand256()), /*nIn=*/1};
    COutPoint outpoint3{Txid::FromUint256(InsecureRand256()), /*nIn=*/2};

    Coin coin = CreateCoin(recvAddress);
    Coin coin2 = CreateCoin(recvAddress);
    Coin coin3;

    auto out3 = blsct::CreateOutput(recvAddress, 1000 * COIN, "test", TokenId(), Scalar::Rand(), blsct::CreateOutputType::STAKED_COMMITMENT, 1000 * COIN);
    coin3.nHeight = 1;
    coin3.out = out3.out;

    {
        CCoinsViewCache coins_view_cache{&base, /*deterministic=*/true};
        coins_view_cache.SetBestBlock(InsecureRand256());
        coins_view_cache.AddCoin(outpoint, std::move(coin), true);
        coins_view_cache.AddCoin(outpoint2, std::move(coin2), true);

        BOOST_ASSERT(coins_view_cache.GetStakedCommitments().Exists(coin.out.blsctData.rangeProof.Vs[0]));
        BOOST_ASSERT(coins_view_cache.GetStakedCommitments().Exists(coin2.out.blsctData.rangeProof.Vs[0]));

        coins_view_cache.SpendCoin(outpoint);

        BOOST_ASSERT(!coins_view_cache.GetStakedCommitments().Exists(coin.out.blsctData.rangeProof.Vs[0]));
        BOOST_ASSERT(coins_view_cache.GetStakedCommitments().Exists(coin2.out.blsctData.rangeProof.Vs[0]));
        BOOST_ASSERT(coins_view_cache.Flush());
        BOOST_ASSERT(!coins_view_cache.GetStakedCommitments().Exists(coin.out.blsctData.rangeProof.Vs[0]));
        BOOST_ASSERT(coins_view_cache.GetStakedCommitments().Exists(coin2.out.blsctData.rangeProof.Vs[0]));
    }

    BOOST_ASSERT(!base.GetStakedCommitments().Exists(coin.out.blsctData.rangeProof.Vs[0]));
    BOOST_ASSERT(base.GetStakedCommitments().Exists(coin2.out.blsctData.rangeProof.Vs[0]));

    CCoinsViewCache coins_view_cache{&base, /*deterministic=*/true};

    coins_view_cache.AddCoin(outpoint3, std::move(coin3), true);
    BOOST_ASSERT(coins_view_cache.GetStakedCommitments().Exists(coin3.out.blsctData.rangeProof.Vs[0]));
    BOOST_ASSERT(coins_view_cache.GetStakedCommitments().Exists(coin2.out.blsctData.rangeProof.Vs[0]));

    CBlockIndex index;
    index.phashBlock = new uint256(InsecureRand256());
    CBlock block;

    blsct::ProofOfStake posProof;
    bool fStop = false;

    while (!fStop) {
        posProof = blsct::ProofOfStakeLogic::Create(coins_view_cache, index, out3.value, out3.gamma);

        arith_uint256 targetBn;
        targetBn.SetCompact(blsct::GetNextTargetRequired(&index, m_node.chainman->GetConsensus()));
        arith_uint256 kernelHash = UintToArith256(blsct::CalculateKernelHash(index.nTime, index.nStakeModifier, posProof.setMemProof.phi, block.nTime));

        std::cout << kernelHash.ToString() << " < " << targetBn.ToString() << " " << (kernelHash < targetBn) << " " << (out3.value.GetUint64() - (kernelHash / targetBn).GetLow64()) << "\n";

        if (kernelHash < targetBn)
            fStop = true;
        else
            block.nTime += 1;
    }

    blsct::ProofOfStakeLogic posProofLogic(posProof);

    BOOST_ASSERT(posProofLogic.Verify(coins_view_cache, index, block, m_node.chainman->GetConsensus()));
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
