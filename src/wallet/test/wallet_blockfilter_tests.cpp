// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/rawtransaction_util.h>
#include <test/lib/blockfilter.h>
#include <test/setup_common.h>
#include <test/util.h>
#include <univalue.h>
#include <validation.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>


void SignTx(const CWallet& keystore, CMutableTransaction& mtx)
{
    // Fetch previous transactions (inputs):
    std::map<COutPoint, Coin> coins;
    for (const CTxIn& txin : mtx.vin) {
        coins[txin.prevout]; // Create empty map entry keyed by prevout.
    }
    keystore.chain().findCoins(coins);

    // sign tx
    SignTransaction(mtx, keystore.GetLegacyScriptPubKeyMan(), coins, UniValue{});
}

BOOST_FIXTURE_TEST_SUITE(wallet_blockfilter_tests, RegTestingSetup)

BOOST_AUTO_TEST_CASE(element_set_matches)
{
    // Generate an utxo to be used in the test
    auto utxo = MineBlock(OP_TRUE);
    for (int num_blocks{COINBASE_MATURITY}; num_blocks > 0; --num_blocks) {
        MineBlock(OP_FALSE);
    }

    // Helper to assert a receive and send matches
    auto CheckFilterMatches = [&utxo](const CScript& to_scriptPubKey, const CWallet& wallet) {
        // the amount we send around
        constexpr CAmount PING_PONG_AMOUNT{333};
        // receive
        auto tx_receive = CMutableTransaction{};
        tx_receive.vin.push_back(utxo);
        tx_receive.vout.emplace_back(PING_PONG_AMOUNT, to_scriptPubKey);
        {
            auto block = PrepareBlock(OP_FALSE);
            block->vtx.push_back(MakeTransactionRef(tx_receive));
            BOOST_REQUIRE(MineBlock(block));

            SyncWithValidationInterfaceQueue();
            BOOST_CHECK_EQUAL(wallet.GetBalance().m_mine_trusted, PING_PONG_AMOUNT);

            BlockFilter block_filter;
            BOOST_REQUIRE(ComputeFilter(BlockFilterType::BASIC, ChainActive().Tip(), block_filter));
            BOOST_CHECK(block_filter.GetFilter().MatchAny(wallet.GetLegacyScriptPubKeyMan()->GetAllRelevantScriptPubKeys()));
        }

        // send
        {
            auto block = PrepareBlock(OP_FALSE);
            auto tx_send = CMutableTransaction{};
            tx_send.vin.emplace_back(tx_receive.GetHash(), 0);
            tx_send.vout.emplace_back(PING_PONG_AMOUNT, OP_TRUE);
            SignTx(wallet, tx_send);
            block->vtx.push_back(MakeTransactionRef(tx_send));
            utxo = {tx_send.GetHash(), 0};
            BOOST_REQUIRE(MineBlock(block));

            SyncWithValidationInterfaceQueue();
            BOOST_CHECK_EQUAL(wallet.GetBalance().m_mine_trusted, 0);

            BlockFilter block_filter;
            BOOST_REQUIRE(ComputeFilter(BlockFilterType::BASIC, ChainActive().Tip(), block_filter));
            BOOST_CHECK(block_filter.GetFilter().MatchAny(wallet.GetLegacyScriptPubKeyMan()->GetAllRelevantScriptPubKeys()));
        }
    };

    NodeContext node;
    std::unique_ptr<interfaces::Chain> chain = interfaces::MakeChain(node);
    auto FreshWallet = [&chain]() {
        auto ret = MakeUnique<CWallet>(chain.get(), WalletLocation(), WalletDatabase::CreateDummy());
        ret->handleNotifications(); // To update the balance
        return ret;
    };

    // Run multiple times, because the element set could happen to match due to the false positive rate
    for (int num_runs{10}; num_runs > 0; --num_runs) {
        CKey keys[2];
        for (int i = 0; i < 2; i++) {
            keys[i].MakeNewKey(true);
        }
        CKey uncompressedKey;
        uncompressedKey.MakeNewKey(false);

        BOOST_TEST_MESSAGE("P2PK compressed");
        {
            auto keystore = FreshWallet();
            {
                LOCK(keystore->cs_wallet);
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
            }
            const CScript scriptPubKey = GetScriptForRawPubKey(keys[0].GetPubKey());
            CheckFilterMatches(scriptPubKey, *keystore);
        }

        BOOST_TEST_MESSAGE("P2PK uncompressed");
        {
            auto keystore = FreshWallet();
            {
                LOCK(keystore->cs_wallet);
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddKey(uncompressedKey));
            }
            const CScript scriptPubKey = GetScriptForRawPubKey(uncompressedKey.GetPubKey());
            CheckFilterMatches(scriptPubKey, *keystore);
        }

        BOOST_TEST_MESSAGE("P2PKH compressed");
        {
            auto keystore = FreshWallet();
            {
                LOCK(keystore->cs_wallet);
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
            }
            const CScript scriptPubKey = GetScriptForDestination(PKHash(keys[0].GetPubKey()));
            CheckFilterMatches(scriptPubKey, *keystore);
        }

        BOOST_TEST_MESSAGE("P2PKH uncompressed");
        {
            auto keystore = FreshWallet();
            {
                LOCK(keystore->cs_wallet);
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddKey(uncompressedKey));
            }
            const CScript scriptPubKey = GetScriptForDestination(PKHash(uncompressedKey.GetPubKey()));
            CheckFilterMatches(scriptPubKey, *keystore);
        }

        BOOST_TEST_MESSAGE("P2SH");
        {
            auto keystore = FreshWallet();
            const CScript redeemScript = GetScriptForDestination(PKHash(keys[0].GetPubKey()));
            {
                LOCK(keystore->cs_wallet);
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddCScript(redeemScript));
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
            }
            const CScript scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));
            CheckFilterMatches(scriptPubKey, *keystore);
        }

        BOOST_TEST_MESSAGE("P2WPKH compressed");
        {
            auto keystore = FreshWallet();
            const CScript scriptPubKey = GetScriptForDestination(WitnessV0KeyHash(PKHash(keys[0].GetPubKey())));
            {
                LOCK(keystore->cs_wallet);
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey));
            }
            CheckFilterMatches(scriptPubKey, *keystore);
        }

        BOOST_TEST_MESSAGE("P2SH multisig");
        {
            auto keystore = FreshWallet();
            const CScript redeemScript = GetScriptForMultisig(2, {uncompressedKey.GetPubKey(), keys[1].GetPubKey()});
            {
                LOCK(keystore->cs_wallet);
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddKey(uncompressedKey));
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddKey(keys[1]));
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddCScript(redeemScript));
            }
            const CScript scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));
            CheckFilterMatches(scriptPubKey, *keystore);
        }

        BOOST_TEST_MESSAGE("P2WSH multisig with compressed keys");
        {
            auto keystore = FreshWallet();
            const CScript witnessScript = GetScriptForMultisig(2, {keys[0].GetPubKey(), keys[1].GetPubKey()});
            const CScript scriptPubKey = GetScriptForDestination(WitnessV0ScriptHash(witnessScript));
            {
                LOCK(keystore->cs_wallet);
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddKey(keys[1]));
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddCScript(witnessScript));
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddCScript(scriptPubKey));
            }
            CheckFilterMatches(scriptPubKey, *keystore);
        }

        BOOST_TEST_MESSAGE("P2WSH multisig wrapped in P2SH");
        {
            auto keystore = FreshWallet();
            const CScript witnessScript = GetScriptForMultisig(2, {keys[0].GetPubKey(), keys[1].GetPubKey()});
            const CScript redeemScript = GetScriptForDestination(WitnessV0ScriptHash(witnessScript));
            {
                LOCK(keystore->cs_wallet);
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddCScript(redeemScript));
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddCScript(witnessScript));
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddKey(keys[0]));
                BOOST_CHECK(keystore->GetLegacyScriptPubKeyMan()->AddKey(keys[1]));
            }
            const CScript scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));
            CheckFilterMatches(scriptPubKey, *keystore);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
