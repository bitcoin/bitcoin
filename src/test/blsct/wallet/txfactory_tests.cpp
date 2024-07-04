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

BOOST_AUTO_TEST_SUITE(blsct_txfactory_tests)

BOOST_FIXTURE_TEST_CASE(ismine_test, TestingSetup)
{
    auto wallet = std::make_unique<wallet::CWallet>(m_node.chain.get(), "", wallet::CreateMockableWalletDatabase());
    wallet->InitWalletFlags(wallet::WALLET_FLAG_BLSCT);

    LOCK(wallet->cs_wallet);
    auto blsct_km = wallet->GetOrCreateBLSCTKeyMan();
    BOOST_CHECK(blsct_km->SetupGeneration(true));

    auto recvAddress = std::get<blsct::DoublePublicKey>(blsct_km->GetNewDestination(0).value());

    auto out = blsct::CreateOutput(recvAddress, 1000, "test");
    BOOST_CHECK(blsct_km->IsMine(out.out));

    auto hashId = blsct_km->GetHashId(out.out);
    blsct::SubAddress subAddressId;

    BOOST_CHECK(blsct_km->GetSubAddress(hashId, subAddressId));

    auto result = blsct_km->RecoverOutputs({out.out});

    BOOST_CHECK(result.is_completed);
    auto xs = result.amounts;
    BOOST_CHECK(xs.size() == 1);
    BOOST_CHECK(xs[0].amount == 1000);
    BOOST_CHECK(xs[0].message == "test");
}

BOOST_FIXTURE_TEST_CASE(createtransaction_test, TestingSetup)
{
    SeedInsecureRand(SeedRand::ZEROS);
    CCoinsViewDB base{{.path = "test", .cache_bytes = 1 << 23, .memory_only = true}, {}};

    wallet::CWallet* wallet(new wallet::CWallet(m_node.chain.get(), "", wallet::CreateMockableWalletDatabase()));
    wallet->InitWalletFlags(wallet::WALLET_FLAG_BLSCT);

    LOCK(wallet->cs_wallet);
    auto blsct_km = wallet->GetOrCreateBLSCTKeyMan();
    BOOST_CHECK(blsct_km->SetupGeneration(true));

    auto recvAddress = std::get<blsct::DoublePublicKey>(blsct_km->GetNewDestination(0).value());

    const auto txid = Txid::FromUint256(InsecureRand256());
    COutPoint outpoint{txid, /*nIn=*/0};

    Coin coin;
    auto out = blsct::CreateOutput(recvAddress, 1000 * COIN, "test");
    coin.nHeight = 1;
    coin.out = out.out;

    auto tx = blsct::TxFactory(blsct_km);
    TxValidationState tx_state;

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
    BOOST_CHECK(blsct::VerifyTx(CTransaction(finalTx.value()), coins_view_cache, tx_state));

    bool fFoundChange = false;

    // Wallet does not have the coins available yet
    BOOST_CHECK(blsct::TxFactory::CreateTransaction(wallet, wallet->GetOrCreateBLSCTKeyMan(), recvAddress, 900 * COIN, "test") == std::nullopt);

    auto result = blsct_km->RecoverOutputs(finalTx.value().vout);

    for (auto& res : result.amounts) {
        if (res.message == "Change" && res.amount == (1000 - 900 - 0.006) * COIN) fFoundChange = true;
    }

    BOOST_CHECK(fFoundChange);

    wallet->transactionAddedToMempool(MakeTransactionRef(finalTx.value()));

    // Wallet does not have the coins available yet (not confirmed in block)
    BOOST_CHECK(blsct::TxFactory::CreateTransaction(wallet, wallet->GetOrCreateBLSCTKeyMan(), recvAddress, 900 * COIN, "test") == std::nullopt);
}

BOOST_FIXTURE_TEST_CASE(addinput_test, TestingSetup)
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
    COutPoint outpoint{txid, /*nIn=*/0};

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
    TxValidationState tx_state;

    BOOST_CHECK(finalTx.has_value());
    BOOST_CHECK(blsct::VerifyTx(CTransaction(finalTx.value()), coins_view_cache, tx_state));

    bool fFoundChange = false;

    auto result = blsct_km->RecoverOutputs(finalTx.value().vout);

    for (auto& res : result.amounts) {
        if (res.message == "Change" && res.amount == (1000 - 900 - 0.006) * COIN) fFoundChange = true;
    }

    BOOST_CHECK(fFoundChange);

    wallet->transactionAddedToMempool(MakeTransactionRef(finalTx.value()));

    auto wtx = wallet->GetWalletTx(finalTx.value().GetHash());
    BOOST_CHECK(wtx != nullptr);

    fFoundChange = false;
    uint32_t nChangePosition = 0;

    for (auto& res : wtx->blsctRecoveryData) {
        if (res.second.message == "Change" && res.second.amount == (1000 - 900 - 0.006) * COIN) {
            nChangePosition = res.second.id;
            fFoundChange = true;
            break;
        }
    }

    BOOST_CHECK(fFoundChange);

    auto tx2 = blsct::TxFactory(blsct_km);
    auto outpoint2 = COutPoint(finalTx.value().GetHash(), nChangePosition);
    Coin coin2;
    coin2.nHeight = 1;
    coin2.out = finalTx.value().vout[nChangePosition];
    coins_view_cache.AddCoin(outpoint2, std::move(coin2), true);

    BOOST_CHECK(tx2.AddInput(coins_view_cache, outpoint2));

    blsct::SubAddress randomAddress(blsct::DoublePublicKey(MclG1Point::MapToPoint("test1"), MclG1Point::MapToPoint("test2")));
    tx2.AddOutput(randomAddress, 50 * COIN, "test");

    auto finalTx2 = tx2.BuildTx();
    wallet->transactionAddedToMempool(MakeTransactionRef(finalTx2.value()));

    BOOST_CHECK(wallet->GetDebit(CTransaction(finalTx2.value()), wallet::ISMINE_SPENDABLE_BLSCT) == (1000 - 900 - 0.006) * COIN);
    BOOST_CHECK(TxGetCredit(*wallet, CTransaction(finalTx2.value()), wallet::ISMINE_SPENDABLE_BLSCT) == (1000 - 900 - 0.006 - 50 - 0.006) * COIN);
}

BOOST_AUTO_TEST_SUITE_END()
