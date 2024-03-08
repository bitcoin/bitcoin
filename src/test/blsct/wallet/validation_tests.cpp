// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/wallet/txfactory.h>
#include <blsct/wallet/verification.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <txdb.h>
#include <wallet/receive.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(blsct_validation_tests)

BOOST_FIXTURE_TEST_CASE(validation_test, TestingSetup)
{
    SeedInsecureRand(SeedRand::ZEROS);
    CCoinsViewDB base{{.path = "test", .cache_bytes = 1 << 23, .memory_only = true}, {}};

    auto wallet = std::make_unique<wallet::CWallet>(m_node.chain.get(), "", wallet::CreateMockableWalletDatabase());
    wallet->InitWalletFlags(wallet::WALLET_FLAG_BLSCT);

    LOCK(wallet->cs_wallet);
    auto blsct_km = wallet->GetOrCreateBLSCTKeyMan();
    BOOST_CHECK(blsct_km->SetupGeneration(true));

    auto recvAddress = std::get<blsct::DoublePublicKey>(blsct_km->GetNewDestination(0).value());

    const auto txid = Txid::FromUint256(InsecureRand256());
    COutPoint outpoint(txid, /*nIn=*/0);

    Coin coin;
    auto out = blsct::CreateOutput(recvAddress, 1000 * COIN, "test");
    coin.nHeight = 1;
    coin.out = out.out;

    auto tx = blsct::TxFactory(blsct_km);

    {
        CCoinsViewCache coins_view_cache{&base, /*deterministic=*/true};
        coins_view_cache.SetBestBlock(InsecureRand256());
        coins_view_cache.AddCoin(outpoint, std::move(coin), true);
        BOOST_CHECK(coins_view_cache.Flush());
    }

    CCoinsViewCache coins_view_cache{&base, /*deterministic=*/true};
    BOOST_CHECK(tx.AddInput(coins_view_cache, outpoint));

    tx.AddOutput(recvAddress, 900 * COIN, "test");

    auto finalTx = tx.BuildTx();

    BOOST_CHECK(finalTx.has_value());
    BOOST_CHECK(blsct::VerifyTx(CTransaction(finalTx.value()), coins_view_cache));
}

BOOST_FIXTURE_TEST_CASE(validation_reward_test, TestingSetup)
{
    CCoinsViewDB base{{.path = "test", .cache_bytes = 1 << 23, .memory_only = true}, {}};
    CCoinsViewCache coins_view_cache{&base, /*deterministic=*/true};

    CMutableTransaction tx;

    auto out = blsct::CreateOutput(blsct::DoublePublicKey(), 900 * COIN, " Reward ");
    tx.vout.push_back(out.out);
    tx.txSig = out.GetSignature();

    BOOST_CHECK(!blsct::VerifyTx(CTransaction(tx), coins_view_cache));
    BOOST_CHECK(blsct::VerifyTx(CTransaction(tx), coins_view_cache, 900 * COIN));
}

BOOST_AUTO_TEST_SUITE_END()
