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
BOOST_FIXTURE_TEST_SUITE(wallet_transaction_tests, BasicTestingSetup)

const CWalletTx* GenBlockAndRetrieveMinedTx(TestChain100Setup& context, const CMutableTransaction& tx_to_mine, const CScript& coinbase_script, CWallet& wallet)
{
    const CBlock& block = context.CreateAndProcessBlock({tx_to_mine}, coinbase_script);
    wallet.blockConnected(kernel::MakeBlockInfo(WITH_LOCK(cs_main, return context.m_node.chainman->ActiveChain().Tip()), &block));
    return Assert(WITH_LOCK(wallet.cs_wallet, return wallet.GetWalletTx(tx_to_mine.GetHash())));
}

const CWalletTx* SendCoinsAndGenBlock(TestChain100Setup& context, const CScript& scriptpubkey, CAmount amount, CWallet& from_wallet, const CScript& coinbase_script)
{
    CCoinControl coin_control;
    auto op_tx = Assert(CreateTransaction(from_wallet, {{scriptpubkey, amount, true}}, 1, coin_control));
    return GenBlockAndRetrieveMinedTx(context, CMutableTransaction(*op_tx->tx), coinbase_script, from_wallet);
}

void CreateMultisigScriptAndImportDescriptor(CWallet& wallet, CScript& multisig_script_out, std::vector<CKey>& priv_keys)
{
    auto spkm = static_cast<DescriptorScriptPubKeyMan*>(wallet.GetScriptPubKeyMan(OutputType::BECH32, false));
    std::vector<CPubKey> pks;
    std::string pks_str; // descriptor data
    for (int i=0; i < 5; i++) {
        CTxDestination multi_dest = *Assert(spkm->GetNewDestination(OutputType::BECH32));
        auto provider = Assert(spkm->GetSolvingProvider(GetScriptForDestination(multi_dest), /*include_private=*/ true));
        CKeyID witness_id = ToKeyID(*Assert(std::get_if<WitnessV0KeyHash>(&multi_dest)));
        CPubKey pubkey;
        BOOST_CHECK(provider->GetPubKey(witness_id, pubkey));
        CKey priv_key;
        BOOST_CHECK(provider->GetKey(witness_id, priv_key));
        pks.emplace_back(pubkey);
        priv_keys.emplace_back(priv_key);
        pks_str += HexStr(pubkey) + ((i < 4) ? "," : "");
    }

    // Import descriptor to watch the multisig script
    std::string descriptor = "wsh(multi(3," + pks_str + "))";
    FlatSigningProvider keys;
    std::string error;
    std::unique_ptr<Descriptor> parsed_desc = Parse(descriptor, keys, error, /*require_checksum=*/false);
    WalletDescriptor w_desc(std::move(parsed_desc), 0, 0, 0, 0);
    Assert(WITH_LOCK(wallet.cs_wallet, return wallet.AddWalletDescriptor(w_desc, keys, "multisig_descriptor", false)));
    multisig_script_out = GetScriptForMultisig(3, pks);
}

// Test wallet receiving txes with the same id and different witness data.
// The following cases are covered:
//
//   1) Two p2wpkh transactions with the same hash are received:
//      The first one with segwit data stripped, and the second one with segwit data.
//      The wallet must update the stored tx, saving the witness data.
//
//   2) Two p2wsh multisig spending txes with the same hash but a different witness are received:
//      The first is added to the wallet by the mempool acceptance flow.
//      while the second one, is added to the wallet by the block connection flow.
//
//      Note: Right now, the wallet will NOT update the stored transaction, the first received
//            transaction will take precedence over any following-up transaction. Don't care
//            if the first transaction didn't get into a block and the second did.
//
BOOST_FIXTURE_TEST_CASE(store_segwit_tx_data, TestChain100Setup)
{
    // Create wallet and generate few more blocks to confirm balance
    std::unique_ptr<CWallet> wallet = CreateSyncedWallet(*m_node.chain, WITH_LOCK(cs_main, return m_node.chainman->ActiveChain()), m_args, coinbaseKey);
    const auto& coinbase_dest_script = GetScriptForDestination(*Assert(wallet->GetNewDestination(OutputType::BECH32, "coinbase")));
    for (int i=0; i<10; i++) {
        const CBlock& block = CreateAndProcessBlock({}, coinbase_dest_script);
        wallet->blockConnected(kernel::MakeBlockInfo(WITH_LOCK(cs_main, return m_node.chainman->ActiveChain().Tip()), &block));
    }
    BOOST_ASSERT(GetBalance(*wallet).m_mine_trusted == COIN * 50 * 10);

    // create the P2WPKH output that will later be spent
    const auto& dest_script = GetScriptForDestination(*Assert(wallet->GetNewDestination(OutputType::BECH32, "")));
    uint256 recv_tx_hash = SendCoinsAndGenBlock(*this, dest_script, 10 * COIN, *wallet, coinbase_dest_script)->GetHash();

    //   1) Two p2wpkh transactions with the same hash are received:
    //      The first one with segwit data stripped, and the second one with segwit data.
    //      The wallet must update the stored tx, saving the witness data.
    {
        // Create the spending tx, strip the witness data and verify that the wallet accepts it
        CCoinControl coin_control;
        coin_control.m_allow_other_inputs = false;
        coin_control.Select({recv_tx_hash, 0});
        auto op_spend_tx = Assert(CreateTransaction(*wallet, {{dest_script, 10 * COIN, true}}, 1, coin_control));
        BOOST_ASSERT(op_spend_tx->tx->HasWitness());
        const uint256& txid = op_spend_tx->tx->GetHash();

        CMutableTransaction mtx(*op_spend_tx->tx);
        CScriptWitness witness_copy = mtx.vin[0].scriptWitness;
        mtx.vin[0].scriptWitness.SetNull();
        wallet->transactionAddedToMempool(MakeTransactionRef(mtx), /*mempool_sequence=*/0);
        const CWalletTx* wtx_no_witness = Assert(WITH_LOCK(wallet->cs_wallet, return wallet->GetWalletTx(txid)));
        BOOST_CHECK(wtx_no_witness->GetWitnessHash() == txid);

        // Re-set the witness and verify that the wallet updates the tx witness data by including the tx in a block
        mtx.vin[0].scriptWitness = witness_copy;
        const CWalletTx* wtx_with_witness = GenBlockAndRetrieveMinedTx(*this, mtx, coinbase_dest_script, *wallet);
        BOOST_CHECK(wtx_with_witness->GetWitnessHash() != txid);

        // Reload the wallet as it would be reloaded from disk and check that the witness data is still there.
        // (flush the previous wallet first)
        wallet->Flush();
        DatabaseOptions options;
        std::unique_ptr<CWallet> wallet_reloaded = std::make_unique<CWallet>(m_node.chain.get(), "", m_args,
                                                                             DuplicateMockDatabase(wallet->GetDatabase(),options));
        BOOST_ASSERT(wallet_reloaded->LoadWallet() == DBErrors::LOAD_OK);
        const CWalletTx* reloaded_wtx_with_witness = WITH_LOCK(wallet_reloaded->cs_wallet, return wallet_reloaded->GetWalletTx(txid));
        BOOST_CHECK_EQUAL(reloaded_wtx_with_witness->GetWitnessHash(), wtx_with_witness->GetWitnessHash());
    }


    //   2) Two p2wsh multisig transactions with the same hash but a different witness are received:
    //      The first is added to the wallet by the mempool acceptance flow.
    //      while the second one, is added to the wallet by the block connection flow.
    //
    //      Note: Right now, the wallet will NOT update the stored transaction, the first received
    //            transaction will take precedence over any following-up transaction. Don't care
    //            if the first transaction didn't get into a block and the second did.
    {
        // Setup context: Create the 3-of-5 multisig script and add the descriptor to the wallet
        CScript multisig_script;
        std::vector<CKey> priv_keys;
        CreateMultisigScriptAndImportDescriptor(*wallet, multisig_script, priv_keys);

        // Lock coins in the multisig script
        const CWalletTx* multisig_tx = SendCoinsAndGenBlock(*this, GetScriptForDestination(WitnessV0ScriptHash(multisig_script)), 3 * COIN, *wallet, coinbase_dest_script);

        // Now create a transaction that spends the funds locked in the multisig script
        CCoinControl coin_control;
        coin_control.m_allow_other_inputs = false;
        coin_control.Select({multisig_tx->GetHash(), 0});
        auto op_spend_tx = Assert(CreateTransaction(*wallet, {{dest_script, 2 * COIN, true}}, 1, coin_control, /*sign=*/false));

        // Now the real test begins, create unsigned tx
        CMutableTransaction unsigned_tx(*op_spend_tx->tx);
        std::map<COutPoint, Coin> coins;
        coins[unsigned_tx.vin[0].prevout] = Coin(multisig_tx->tx->vout[unsigned_tx.vin[0].prevout.n], /*nHeightIn=*/113, /*fCoinBaseIn=*/false);

        // 1) Use the first three keys and add tx to wallet via mempool acceptance:
        FillableSigningProvider keystore_1;
        for (int i=0; i<3; i++) BOOST_ASSERT(keystore_1.AddKey(priv_keys[i]));
        BOOST_ASSERT(keystore_1.AddCScript(multisig_script));
        std::map<int, bilingual_str> input_errors;
        CMutableTransaction signed_tx_1 = unsigned_tx;
        Assert(SignTransaction(signed_tx_1, &keystore_1, coins, SIGHASH_DEFAULT, input_errors));

        const CTransactionRef& spend_from_multisig_tx_1 = MakeTransactionRef(signed_tx_1);
        wallet->transactionAddedToMempool(spend_from_multisig_tx_1, /*mempool_sequence=*/0);
        const CWalletTx* wtx_spending_multisig_1 = Assert(WITH_LOCK(wallet->cs_wallet, return wallet->GetWalletTx(signed_tx_1.GetHash())));
        BOOST_CHECK(wtx_spending_multisig_1->GetWitnessHash() == spend_from_multisig_tx_1->GetWitnessHash());

        // 2) Use the last keys and try to add tx to wallet via block connection:
        FillableSigningProvider keystore_2;
        for (int i=priv_keys.size()-1; i>=2; i--) keystore_2.AddKey(priv_keys[i]);
        BOOST_ASSERT(keystore_2.AddCScript(multisig_script));
        CMutableTransaction signed_tx_2 = unsigned_tx;
        Assert(SignTransaction(signed_tx_2, &keystore_2, coins, SIGHASH_DEFAULT, input_errors));

        // Assert that the tx id is equal to the other tx but the witness data is different
        BOOST_ASSERT(signed_tx_1.GetHash() == signed_tx_2.GetHash());
        BOOST_ASSERT(signed_tx_1.vin[0].scriptWitness.stack != signed_tx_2.vin[0].scriptWitness.stack);

        // Now connect the block and verify current behavior
        const CWalletTx* wtx = GenBlockAndRetrieveMinedTx(*this, signed_tx_2, coinbase_dest_script, *wallet);

        // Important: current wallet behavior will NOT update the input witness data.
        // The wallet will only see the witness data of the first seen tx.
        BOOST_CHECK(wtx->GetHash() == signed_tx_2.GetHash());
        BOOST_CHECK(wtx->GetWitnessHash() != MakeTransactionRef(signed_tx_2)->GetWitnessHash());
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
