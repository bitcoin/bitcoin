// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <node/types.h>
#include <policy/feerate.h>
#include <test/util/setup_common.h>
#include <txmempool.h>
#include <util/check.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/context.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>
#include <optional>

namespace wallet {

BOOST_FIXTURE_TEST_SUITE(wallet_interfaces_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(addhdkey)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    auto interface = interfaces::MakeWallet(context, wallet);

    auto result = interface->addHDKey(std::nullopt);
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->pubkey.IsValid());
}

BOOST_AUTO_TEST_CASE(addhdkey_with_key)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    auto interface = interfaces::MakeWallet(context, wallet);

    CKey seed_key = GenerateRandomKey();
    CExtKey key;
    key.SetSeed(seed_key);

    auto result = interface->addHDKey(key);
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->pubkey.IsValid());
    BOOST_CHECK_EQUAL(EncodeExtPubKey(*result), EncodeExtPubKey(key.Neuter()));
}

BOOST_AUTO_TEST_CASE(addhdkey_with_malformed_key)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    auto interface = interfaces::MakeWallet(context, wallet);

    CExtKey key;
    key.SetSeed(GenerateRandomKey());
    key.nChild = 1;

    auto result = interface->addHDKey(key);
    BOOST_REQUIRE(!result);
    BOOST_CHECK(result.error().code == WalletErrorCode::GenericError);
    BOOST_CHECK_EQUAL(result.error().message.original, "Invalid HD key");
}

BOOST_AUTO_TEST_CASE(addhdkey_wallet_locked)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    BOOST_REQUIRE(wallet->EncryptWallet("hunter2"));
    BOOST_REQUIRE(wallet->Lock());

    auto interface = interfaces::MakeWallet(context, wallet);
    auto result = interface->addHDKey(std::nullopt);
    BOOST_REQUIRE(!result);
    BOOST_CHECK(result.error().code == WalletErrorCode::UnlockNeeded);
}

BOOST_FIXTURE_TEST_CASE(commit_transaction, TestChain100Setup)
{
    CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    std::shared_ptr<CWallet> wallet{CreateSyncedWallet(*m_node.chain, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain()), coinbaseKey)};
    WalletContext context;
    context.chain = m_node.chain.get();
    auto interface = interfaces::MakeWallet(context, wallet);

    CCoinControl coin_control;
    coin_control.m_feerate = CFeeRate{1000};
    const auto created = interface->createTransaction({{PKHash{}, COIN, false}}, coin_control, /*sign=*/true, /*change_pos=*/std::nullopt);
    BOOST_REQUIRE(created);
    const auto txid = created->tx->GetHash();

    wallet->SetBroadcastTransactions(false);
    const auto skipped = interface->commitTransaction(created->tx, {});
    BOOST_REQUIRE(!skipped);
    BOOST_CHECK(skipped.error().code == node::TransactionError::SUBMISSION_SKIPPED);
    BOOST_CHECK(!m_node.mempool->exists(txid));

    wallet->SetBroadcastTransactions(true);
    BOOST_CHECK(interface->commitTransaction(created->tx, {}));
    BOOST_CHECK(m_node.mempool->exists(txid));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace wallet
