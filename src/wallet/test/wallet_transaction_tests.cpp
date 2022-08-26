// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/transaction.h>
#include <wallet/coincontrol.h>
#include <kernel/chain.h>
#include <validation.h>
#include <wallet/receive.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(wallet_transaction_tests, WalletTestingSetup)

// Create a descriptor wallet synced with the current chain.
// The wallet will have all the coinbase txes created up to block 110.
// So, only 10 UTXO available for spending. The other 100 UTXO immature.
std::unique_ptr<CWallet> CreateDescriptorWallet(TestChain100Setup* context)
{
    std::unique_ptr<CWallet> wallet = CreateSyncedWallet(*context->m_node.chain, WITH_LOCK(::cs_main, return context->m_node.chainman->ActiveChain()), context->coinbaseKey);
    for (int i=0; i<10; i++) {
        const CBlock& block = context->CreateAndProcessBlock({}, GetScriptForDestination(PKHash(context->coinbaseKey.GetPubKey().GetID())));
        wallet->blockConnected(ChainstateRole::NORMAL, kernel::MakeBlockInfo(WITH_LOCK(::cs_main, return context->m_node.chainman->ActiveChain().Tip()), &block));
    }
    BOOST_REQUIRE(WITH_LOCK(wallet->cs_wallet, return AvailableCoins(*wallet).GetTotalAmount()) == 50 * COIN * 10);
    return wallet;
}

// Create a legacy wallet synced with the current chain.
// The wallet will have all the coinbase txes created up to block 110.
// So, only 10 UTXO available for spending. The other 100 UTXO immature.
std::unique_ptr<CWallet> CreateLegacyWallet(TestChain100Setup* context, WalletFeature wallet_feature, CKey coinbase_key, bool gen_blocks)
{
    auto wallet = std::make_unique<CWallet>(context->m_node.chain.get(), "", CreateMockableWalletDatabase());
    {
        LOCK2(wallet->cs_wallet, ::cs_main);
        const auto& active_chain = context->m_node.chainman->ActiveChain();
        wallet->SetLastBlockProcessed(active_chain.Height(), active_chain.Tip()->GetBlockHash());
        BOOST_REQUIRE(wallet->LoadWallet() == DBErrors::LOAD_OK);
        wallet->SetMinVersion(wallet_feature);
        wallet->SetupLegacyScriptPubKeyMan();
        BOOST_CHECK(wallet->GetLegacyScriptPubKeyMan()->SetupGeneration());

        std::map<CKeyID, CKey> privkey_map;
        privkey_map.insert(std::make_pair(coinbase_key.GetPubKey().GetID(), coinbase_key));
        BOOST_REQUIRE(wallet->ImportPrivKeys(privkey_map, 0));

        WalletRescanReserver reserver(*wallet);
        reserver.reserve();
        CWallet::ScanResult result = wallet->ScanForWalletTransactions(
                active_chain.Genesis()->GetBlockHash(), /*start_height=*/0, /*max_height=*/{}, reserver, /*fUpdate=*/
                false, /*save_progress=*/false);
        BOOST_CHECK_EQUAL(result.status, CWallet::ScanResult::SUCCESS);
        BOOST_CHECK_EQUAL(result.last_scanned_block, active_chain.Tip()->GetBlockHash());
        BOOST_CHECK_EQUAL(*result.last_scanned_height, active_chain.Height());
        BOOST_CHECK(result.last_failed_block.IsNull());
    }
    // Gen few more blocks to have available balance
    if (gen_blocks) {
        for (int i = 0; i < 15; i++) {
            const CBlock& block = context->CreateAndProcessBlock({}, GetScriptForDestination(PKHash(context->coinbaseKey.GetPubKey().GetID())));
            wallet->blockConnected(ChainstateRole::NORMAL, kernel::MakeBlockInfo(WITH_LOCK(::cs_main, return context->m_node.chainman->ActiveChain().Tip()), &block));
        }
    }
    return wallet;
}

std::unique_ptr<CWallet> CreateEmtpyWallet(TestChain100Setup* context, bool only_legacy, WalletFeature legacy_wallet_features = FEATURE_BASE)
{
    CKey random_key;
    random_key.MakeNewKey(true);
    if (only_legacy) return CreateLegacyWallet(context, legacy_wallet_features, random_key, /*gen_blocks=*/false);
    else return CreateSyncedWallet(*context->m_node.chain,
                                   WITH_LOCK(Assert(context->m_node.chainman)->GetMutex(), return context->m_node.chainman->ActiveChain()), random_key);
}

bool OutputIsChange(const std::unique_ptr<CWallet>& wallet, const CTransactionRef& tx, unsigned int change_pos)
{
    return WITH_LOCK(wallet->cs_wallet, return OutputIsChange(*wallet, tx->vout.at(change_pos)));
}

// Creates a transaction and adds it to the wallet via the mempool signal
CreatedTransactionResult CreateAndAddTx(const std::unique_ptr<CWallet>& wallet, const CTxDestination& dest, CAmount amount)
{
    CCoinControl coin_control;
    auto op_tx = *Assert(CreateTransaction(*wallet, {{dest, amount, true}}, /*change_pos=*/std::nullopt, coin_control));
    // Prior to adding it to the wallet, check if the wallet can detect the change script
    BOOST_CHECK(OutputIsChange(wallet, op_tx.tx, *op_tx.change_pos));
    wallet->transactionAddedToMempool(op_tx.tx);
    return op_tx;
}

void CreateTxAndVerifyChange(const std::unique_ptr<CWallet>& wallet, const std::unique_ptr<CWallet>& external_wallet, OutputType dest_type, CAmount dest_amount)
{
    CTxDestination dest = *Assert(external_wallet->GetNewDestination(dest_type, ""));
    auto res = CreateAndAddTx(wallet, dest, dest_amount);
    assert(res.tx->vout.size() == 2); // always expect 2 outputs (destination and change).
    unsigned int change_pos = *res.change_pos;
    BOOST_CHECK(res.tx->vout.at(change_pos).nValue == (50 * COIN - dest_amount));
    BOOST_CHECK(OutputIsChange(wallet, res.tx, change_pos));
    BOOST_CHECK(!OutputIsChange(wallet, res.tx, !change_pos));
}

/**
 * Test descriptor wallet change output detection.
 *
 * Context:
 * A descriptor wallet with legacy support is created ("local" wallet from now on).
 * A second wallet that provides external destinations is created ("external" wallet from now on).
 *
 * General test concept:
 * Create transactions using the local wallet to different destinations provided by the external wallet.
 * Verifying that the wallet can detect the change output prior and post the tx is added to the wallet.
 *
 * The following cases are covered:
 * 1) Create tx that sends to a legacy p2pkh addr and verify change detection.
 * 2) Create tx that sends to a p2wpkh addr and verify change detection.
 * 3) Create tx that sends to a wrapped p2wpkh addr and verify change detection.
 * 4) Create tx that sends to a taproot addr and verify change detection.
 */
BOOST_FIXTURE_TEST_CASE(descriptors_wallet_detect_change_output, TestChain100Setup)
{
    // Create wallet, 10 UTXO spendable, 100 UTXO immature.
    std::unique_ptr<CWallet> wallet = CreateDescriptorWallet(this);
    wallet->SetupLegacyScriptPubKeyMan();
    // External wallet mimicking another user
    std::unique_ptr<CWallet> external_wallet = CreateEmtpyWallet(this, /*only_legacy=*/false);
    external_wallet->SetupLegacyScriptPubKeyMan();

    // 1) send to legacy p2pkh and verify that the wallet detects the change output
    // 2) send to p2wpkh and verify that the wallet detects the change output
    // 3) send to a wrapped p2wpkh and verify that the wallet detects the change output
    // 4) send to a taproot and verify that the wallet detects the change output
    for (const auto& dest_type : {OutputType::LEGACY, OutputType::BECH32, OutputType::P2SH_SEGWIT, OutputType::BECH32M}) {
        CreateTxAndVerifyChange(wallet, external_wallet, dest_type, 15 * COIN);
    }
}

/**
 * Test legacy-only wallet change output detection.
 *
 * Context:
 * A legacy-only wallet is created ("local" wallet from now on).
 * A second wallet that provides external destinations is created ("external" wallet from now on).
 *
 * General test concept:
 * Create transactions using the local wallet to different destinations provided by the external wallet.
 * Verifying that the wallet can detect the change output prior and post the tx is added to the wallet.
 *
 * The following cases are covered:
 * 1) Create tx that sends to a legacy p2pkh addr and verify change detection.
 * 2) Create tx that sends to a p2wpkh addr and verify change detection.
 * 3) Create tx that sends to a wrapped p2wpkh addr and verify change detection.
 */
BOOST_FIXTURE_TEST_CASE(legacy_wallet_detect_change_output, TestChain100Setup)
{
    // FEATURE_HD doesn't create the internal derivation path, all the addresses are external
    // FEATURE_HD_SPLIT creates internal and external paths.
    for (WalletFeature feature : {FEATURE_HD, FEATURE_HD_SPLIT}) {
        // Create wallet, 10 UTXO spendable, 100 UTXO immature.
        std::unique_ptr<CWallet> wallet = CreateLegacyWallet(this, feature, coinbaseKey, /*gen_blocks=*/true);
        // External wallet mimicking another user
        std::unique_ptr<CWallet> external_wallet = CreateEmtpyWallet(this, /*only_legacy=*/true, feature);

        // 1) Create a transaction that sends to a legacy p2pkh and verify that the wallet detects the change output
        // 2) Create a transaction that sends to a p2wpkh and verify that the wallet detects the change output
        // 3) Create a transaction that sends to a wrapped p2wpkh and verify that the wallet detects the change output
        for (const auto& dest_type : {OutputType::LEGACY, OutputType::BECH32, OutputType::P2SH_SEGWIT}) {
            CreateTxAndVerifyChange(wallet, external_wallet, dest_type, 10 * COIN);
        }
    }
}

BOOST_AUTO_TEST_CASE(roundtrip)
{
    for (uint8_t hash = 0; hash < 5; ++hash) {
        for (int index = -2; index < 3; ++index) {
            TxState state = TxStateInterpretSerialized(TxStateUnrecognized{uint256{hash}, index});
            BOOST_CHECK_EQUAL(TxStateSerializedBlockHash(state), uint256{hash});
            BOOST_CHECK_EQUAL(TxStateSerializedIndex(state), index);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
