// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/transaction.h>
#include <wallet/coincontrol.h>
#include <key_io.h>
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
    return WITH_LOCK(wallet->cs_wallet, return IsOutputChange(*wallet, *tx, change_pos));
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

std::unique_ptr<Descriptor> CreateSingleKeyDescriptor(const std::string& type, FlatSigningProvider& provider)
{
    CKey seed;
    seed.MakeNewKey(true);
    CExtKey master_key;
    master_key.SetSeed(seed);
    std::string descriptor = type + "(" + EncodeExtKey(master_key) + ")";
    std::string error;
    std::unique_ptr<Descriptor> desc = Parse(descriptor, provider, error);
    BOOST_REQUIRE(desc);
    return desc;
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

    {
        // Inactive descriptors tests

        // 1) Verify importing a not-ranged internal descriptor, then create a tx and send coins to it.
        // the wallet should be able to detect inactive descriptors as change.
        FlatSigningProvider provider;
        std::unique_ptr<Descriptor> desc = CreateSingleKeyDescriptor("pkh", provider);
        WalletDescriptor wdesc(std::move(desc), 0, 0, 0, 0, /*_internal=*/true);
        ScriptPubKeyMan* spkm = WITH_LOCK(wallet->cs_wallet, return wallet->AddWalletDescriptor(wdesc, provider, "", /*internal=*/true));

        CTxDestination dest_change;
        BOOST_CHECK(ExtractDestination(*spkm->GetScriptPubKeys().begin(), dest_change));

        CCoinControl coin_control;
        coin_control.destChange = dest_change;
        auto op_tx = *Assert(CreateTransaction(*wallet, {{CNoDestination{CScript() << OP_TRUE}, 1 * COIN, true}}, /*change_pos=*/std::nullopt, coin_control));
        BOOST_CHECK(OutputIsChange(wallet, op_tx.tx, *op_tx.change_pos));


        // 2) Verify importing a not-ranged descriptor that contains the key of an output already stored in the wallet.
        //    The wallet should process the import and detect the output as change.
        FlatSigningProvider provider2;
        std::unique_ptr<Descriptor> desc2 = CreateSingleKeyDescriptor("wpkh", provider2);
        std::vector<CScript> scripts;
        FlatSigningProvider out_provider;
        BOOST_CHECK(desc2->Expand(/*pos=*/0, provider2, scripts, out_provider));
        BOOST_CHECK(ExtractDestination(scripts[0], dest_change));

        coin_control.destChange = dest_change;
        auto op_tx2 = *Assert(CreateTransaction(*wallet, {{CNoDestination{CScript() << OP_TRUE}, 1 * COIN, true}}, /*change_pos=*/std::nullopt, coin_control));
        BOOST_CHECK(!OutputIsChange(wallet, op_tx2.tx, *op_tx2.change_pos));

        // Now add the descriptor and verify that change is detected
        WalletDescriptor wdesc2(std::move(desc2), 0, 0, 0, 0, /*_internal=*/true);
        WITH_LOCK(wallet->cs_wallet, return wallet->AddWalletDescriptor(wdesc2, provider2, "", /*internal=*/true));
        BOOST_CHECK(OutputIsChange(wallet, op_tx2.tx, *op_tx2.change_pos));
    }


    // ### Now verify the negative paths:

    {
        // 1) The change output goes to an address in one of the wallet external path
        CCoinControl coin_control;
        coin_control.destChange = *Assert(wallet->GetNewDestination(OutputType::BECH32, ""));
        auto op_tx = *Assert(CreateTransaction(*wallet, {{CNoDestination{CScript() << OP_TRUE}, 1 * COIN, true}}, /*change_pos=*/std::nullopt, coin_control));
        BOOST_CHECK(!OutputIsChange(wallet, op_tx.tx, *op_tx.change_pos));
        wallet->transactionAddedToMempool(op_tx.tx);
        auto wtx = WITH_LOCK(wallet->cs_wallet, return wallet->GetWalletTx(op_tx.tx->GetHash()));
        assert(wtx);
        BOOST_CHECK(!OutputIsChange(wallet, wtx->tx, *op_tx.change_pos));
    }

    {
        // 2) The change goes back to the source.
        // As the source is an external destination, and we are currently detecting change through it output script, the change will be marked as external (not change).

        // Create source address
        auto dest = *Assert(wallet->GetNewDestination(OutputType::BECH32, ""));
        auto op_tx_source = CreateAndAddTx(wallet, dest, 15 * COIN);
        const CBlock& block = CreateAndProcessBlock({CMutableTransaction(*op_tx_source.tx)}, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey().GetID())));
        wallet->blockConnected(ChainstateRole::NORMAL, kernel::MakeBlockInfo(WITH_LOCK(::cs_main, return m_node.chainman->ActiveChain().Tip()), &block));

        // Now create the tx that spends from the source and sends the change to it.
        LOCK(wallet->cs_wallet);
        CCoinControl coin_control;
        unsigned int change_pos = *op_tx_source.change_pos;
        coin_control.Select(COutPoint(op_tx_source.tx->GetHash(), !change_pos));
        coin_control.destChange = dest;
        auto op_tx = *Assert(CreateTransaction(*wallet, {{CNoDestination{CScript() << OP_TRUE}, 1 * COIN, true}}, /*change_pos=*/std::nullopt, coin_control));
        BOOST_CHECK(!OutputIsChange(wallet, op_tx.tx, *op_tx.change_pos));
        wallet->transactionAddedToMempool(op_tx.tx);
        auto wtx = wallet->GetWalletTx(op_tx.tx->GetHash());
        assert(wtx);
        BOOST_CHECK(!OutputIsChange(wallet, wtx->tx, *op_tx.change_pos));
    }

    {
        // 3) Test what happens when the user sets an address book label to a destination created from an internal key.
        auto op_tx = CreateAndAddTx(wallet, *Assert(wallet->GetNewDestination(OutputType::BECH32, "")), 15 * COIN);
        BOOST_CHECK(OutputIsChange(wallet, op_tx.tx, *op_tx.change_pos));
        // Now change the change destination label
        CTxDestination change_dest;
        BOOST_CHECK(ExtractDestination(op_tx.tx->vout.at(*op_tx.change_pos).scriptPubKey, change_dest));
        BOOST_CHECK(wallet->SetAddressBook(change_dest, "not_a_change_address", AddressPurpose::RECEIVE));
        // TODO: FIXME, Currently, change detection fails when the user sets a custom label to a destination created from an internal path.
        BOOST_CHECK(OutputIsChange(wallet, op_tx.tx, *op_tx.change_pos));
    }

    {
        // 4) Test coins reception into an internal address directly.
        //    As the wallet is receiving those coins from outside, them should not be detected as change.

        // Add balance to external wallet
        auto op_funding_tx = CreateAndAddTx(wallet, *external_wallet->GetNewDestination(OutputType::BECH32, ""), 15 * COIN);
        const CBlock& block = CreateAndProcessBlock({CMutableTransaction(*op_funding_tx.tx)}, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey().GetID())));
        auto block_info = kernel::MakeBlockInfo(WITH_LOCK(::cs_main, return m_node.chainman->ActiveChain().Tip()), &block);
        for (const auto& it_wallet : {wallet.get(), external_wallet.get()}) it_wallet->blockConnected(ChainstateRole::NORMAL, block_info);

        // Send coins to the local wallet internal address directly.
        auto op_tx = CreateAndAddTx(external_wallet, *Assert(wallet->GetNewChangeDestination(OutputType::BECH32)), 10 * COIN);
        BOOST_CHECK(!OutputIsChange(wallet, op_tx.tx, *op_tx.change_pos));
    }
}

CTxDestination deriveDestination(const std::unique_ptr<Descriptor>& descriptor, int index, const FlatSigningProvider& privkey_provider)
{
    FlatSigningProvider provider;
    std::vector<CScript> scripts;
    BOOST_CHECK(descriptor->Expand(index, privkey_provider, scripts, provider));
    CTxDestination dest;
    BOOST_CHECK(ExtractDestination(scripts[0], dest));
    return dest;
}

/**
 * The old change detection process assumption can easily be broken by creating an address manually,
 * from an external derivation path, and send coins to it. As the address was not created by the
 * wallet, it will not be in the addressbook, there by will be treated as change when it's clearly not.
 *
 * The wallet will properly detect it once the transaction gets added to the wallet and the address
 * (and all the previous unused address) are derived and stored in the address book.
 */
BOOST_FIXTURE_TEST_CASE(external_tx_creation_change_output_detection, TestChain100Setup)
{
    // Create miner wallet, 10 UTXO spendable, 100 UTXO immature.
    std::unique_ptr<CWallet> external_wallet = CreateDescriptorWallet(this);
    external_wallet->SetupLegacyScriptPubKeyMan();
    // Local wallet
    std::unique_ptr<CWallet> wallet = CreateEmtpyWallet(this, /*only_legacy=*/false);
    wallet->SetupLegacyScriptPubKeyMan();

    auto external_desc_spkm = static_cast<DescriptorScriptPubKeyMan*>(wallet->GetScriptPubKeyMan(OutputType::BECH32, /*internal=*/false));
    std::string desc_str;
    BOOST_CHECK(external_desc_spkm->GetDescriptorString(desc_str, true));
    FlatSigningProvider key_provider;
    std::string error;
    std::unique_ptr<Descriptor> descriptor = Assert(Parse(desc_str, key_provider, error, /*require_checksum=*/false));

    {
        // Now derive a key at an index that wasn't created by the wallet and send coins to it
        // TODO: Test this same point on a legacy wallet.
        int addr_index = 100;
        const CTxDestination& dest = deriveDestination(descriptor, addr_index, key_provider);
        CAmount dest_amount = 1 * COIN;
        CCoinControl coin_control;
        auto res_tx = CreateTransaction(*external_wallet, {{dest, dest_amount, true}},
                                               /*change_pos=*/std::nullopt, coin_control);
        assert(res_tx);
        assert(res_tx->tx->vout.size() == 2); // dest + change
        unsigned int change_pos = *res_tx->change_pos;
        unsigned int pos_not_change = !change_pos;

        // Prior to adding the tx to the local wallet, check if can detect the output script
        const CTxOut& output = res_tx->tx->vout.at(pos_not_change);
        BOOST_CHECK(output.nValue == dest_amount);
        BOOST_CHECK(WITH_LOCK(wallet->cs_wallet, return wallet->IsMine(output)));
        // ----> todo: currently the wallet invalidly detects the external receive address as change when it is not.
        //             this is due an non-existent address book record.
        BOOST_CHECK(!OutputIsChange(wallet, res_tx->tx, pos_not_change));

        // Now add it to the wallet and verify that is not invalidly marked as change
        wallet->transactionAddedToMempool(res_tx->tx);
        const CWalletTx* wtx = Assert(WITH_LOCK(wallet->cs_wallet, return wallet->GetWalletTx(res_tx->tx->GetHash())));
        BOOST_CHECK(!OutputIsChange(wallet, wtx->tx, pos_not_change));
        BOOST_CHECK_EQUAL(TxGetChange(*wallet, *res_tx->tx), 0);
    }

    {
        // Now check the wallet limitations by sending coins to an address that is above the keypool size
        int addr_index = DEFAULT_KEYPOOL_SIZE + 300;
        const CTxDestination& dest = deriveDestination(descriptor, addr_index, key_provider);
        CAmount dest_amount = 2 * COIN;
        CCoinControl coin_control;
        auto op_tx = *Assert(CreateTransaction(*external_wallet, {{dest, dest_amount, true}}, /*change_pos=*/std::nullopt, coin_control));

        // Prior to adding the tx to the local wallet, check if can detect the output script
        unsigned int change_pos = *op_tx.change_pos;
        unsigned int pos_not_change = !change_pos;
        const CTxOut& output = op_tx.tx->vout.at(pos_not_change);
        BOOST_CHECK(output.nValue == dest_amount);

        // As this address is above our keypool, we are not able to consider it from the wallet!
        BOOST_CHECK(!WITH_LOCK(wallet->cs_wallet, return wallet->IsMine(output)));
        BOOST_CHECK(!OutputIsChange(wallet, op_tx.tx, pos_not_change));

        const CBlock& block = CreateAndProcessBlock({CMutableTransaction(*op_tx.tx)}, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey().GetID())));
        wallet->blockConnected(ChainstateRole::NORMAL, kernel::MakeBlockInfo(WITH_LOCK(::cs_main, return m_node.chainman->ActiveChain().Tip()), &block));

        const CWalletTx* wtx = WITH_LOCK(wallet->cs_wallet, return wallet->GetWalletTx(op_tx.tx->GetHash()));
        BOOST_CHECK(!wtx); // --> the transaction is not added.
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
