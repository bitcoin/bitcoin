// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/solver.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/strencodings.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/feebumper.h>
#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
namespace feebumper {
BOOST_FIXTURE_TEST_SUITE(feebumper_tests, WalletTestingSetup)

static void CheckMaxWeightComputation(const std::string& script_str, const std::vector<std::string>& witness_str_stack, const std::string& prevout_script_str, int64_t expected_max_weight)
{
    std::vector script_data(ParseHex(script_str));
    CScript script(script_data.begin(), script_data.end());
    CTxIn input(Txid{}, 0, script);

    for (const auto& s : witness_str_stack) {
        input.scriptWitness.stack.push_back(ParseHex(s));
    }

    std::vector prevout_script_data(ParseHex(prevout_script_str));
    CScript prevout_script(prevout_script_data.begin(), prevout_script_data.end());

    int64_t weight = GetTransactionInputWeight(input);
    SignatureWeights weights;
    SignatureWeightChecker size_checker(weights, DUMMY_CHECKER);
    bool script_ok = VerifyScript(input.scriptSig, prevout_script, &input.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, size_checker);
    BOOST_CHECK(script_ok);
    weight += weights.GetWeightDiffToMax();
    BOOST_CHECK_EQUAL(weight, expected_max_weight);
}

BOOST_AUTO_TEST_CASE(external_max_weight_test)
{
    // P2PKH
    CheckMaxWeightComputation("453042021f03c8957c5ce12940ee6e3333ecc3f633d9a1ac53a55b3ce0351c617fa96abe021f0dccdcce3ef45a63998be9ec748b561baf077b8e862941d0cd5ec08f5afe68012102fccfeb395f0ecd3a77e7bc31c3bc61dc987418b18e395d441057b42ca043f22c", {}, "76a914f60dcfd3392b28adc7662669603641f578eed72d88ac", 593);
    // P2SH-P2WPKH
    CheckMaxWeightComputation("160014001dca1b22c599b5a56a87c78417ad2ff39552f1", {"3042021f5443c58eaf45f3e5ef46f8516f966b334a7d497cedda4edb2b9fad57c90c3b021f63a77cb56cde848e2e2dd20b487eec2f53101f634193786083f60b4d23a82301", "026cfe86116f161057deb240201d6b82ebd4f161e0200d63dc9aca65a1d6b38bb7"}, "a9147c8ab5ad7708b97ccb6b483d57aba48ee85214df87", 364);
    // P2WPKH
    CheckMaxWeightComputation("", {"3042021f0f8906f0394979d5b737134773e5b88bf036c7d63542301d600ab677ba5a59021f0e9fe07e62c113045fa1c1532e2914720e8854d189c4f5b8c88f57956b704401", "0359edba11ed1a0568094a6296a16c4d5ee4c8cfe2f5e2e6826871b5ecf8188f79"}, "00149961a78658030cc824af4c54fbf5294bec0cabdd", 272);
    // P2WSH HTLC
    CheckMaxWeightComputation("", {"3042021f5c4c29e6b686aae5b6d0751e90208592ea96d26bc81d78b0d3871a94a21fa8021f74dc2f971e438ccece8699c8fd15704c41df219ab37b63264f2147d15c34d801", "01", "6321024cf55e52ec8af7866617dc4e7ff8433758e98799906d80e066c6f32033f685f967029000b275210214827893e2dcbe4ad6c20bd743288edad21100404eb7f52ccd6062fd0e7808f268ac"}, "002089e84892873c679b1129edea246e484fd914c2601f776d4f2f4a001eb8059703", 318);
}

// Helper to create a simple transaction for mempool population.
static CTransactionRef MakeMempoolTx(const std::vector<COutPoint>& inputs, const CScript& spk)
{
    CMutableTransaction tx;
    tx.vin.reserve(inputs.size());
    for (const auto& input : inputs) {
        tx.vin.emplace_back(input);
    }
    tx.vout.emplace_back(COIN, spk);
    return MakeTransactionRef(tx);
}

BOOST_FIXTURE_TEST_CASE(combined_bump_fee_fails_for_enormous_cluster, TestChain100Setup)
{
    // Test that CreateRateBumpTransaction returns WALLET_ERROR with the
    // appropriate error message when the transaction's unconfirmed inputs
    // are part of clusters that collectively exceed the 500-transaction
    // GatherClusters limit, causing calculateCombinedBumpFee to return
    // nullopt. This covers feebumper.cpp lines 83-87.

    CTxMemPool& pool = *Assert(m_node.mempool);
    const CScript spk = GetScriptForRawPubKey(coinbaseKey.GetPubKey());

    // Build 501 mempool transactions across 10 chains of ~50 each.
    // GatherClusters returns empty when the total exceeds 500.
    // Keep references to the tip transactions so we can add them to the wallet
    // later (needed for AllInputsMine to recognize inputs as ours).
    std::vector<CTransactionRef> tip_txs;
    {
        LOCK2(cs_main, pool.cs);
        TestMemPoolEntryHelper entry;

        for (int chain = 0; chain < 10; ++chain) {
            CTransactionRef lasttx = m_coinbase_txns[chain];
            const int chain_length = (chain == 0) ? 51 : 50; // 501 total
            for (int i = 0; i < chain_length; ++i) {
                auto tx = MakeMempoolTx({COutPoint{lasttx->GetHash(), 0}}, spk);
                TryAddToMempool(pool, entry.Fee(CENT).FromTx(tx));
                lasttx = tx;
            }
            tip_txs.push_back(lasttx);
        }
    }
    BOOST_REQUIRE_EQUAL(pool.size(), 501U);

    // Verify the cluster limit is indeed breached
    {
        LOCK2(cs_main, pool.cs);
        std::vector<Txid> tip_txids;
        for (const auto& tip_tx : tip_txs) {
            tip_txids.push_back(tip_tx->GetHash());
        }
        const auto cluster = pool.GatherClusters(tip_txids);
        BOOST_CHECK_EQUAL(cluster.size(), 0); // Empty means >500 limit hit
    }

    // Create a wallet synced to the chain
    auto wallet = CreateSyncedWallet(*m_node.chain, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain()), coinbaseKey);

    // Construct a wallet transaction that spends the tips of all clusters.
    // This simulates a transaction whose inputs are spread across enormous
    // clusters of unconfirmed transactions.
    CMutableTransaction orig_tx;
    for (const auto& tip_tx : tip_txs) {
        orig_tx.vin.emplace_back(COutPoint{tip_tx->GetHash(), 0});
    }
    orig_tx.vout.emplace_back(5 * COIN, spk);
    const auto orig_tx_ref = MakeTransactionRef(orig_tx);
    const Txid orig_txid = orig_tx_ref->GetHash();

    // Add the cluster tip transactions and the original transaction to the
    // wallet so that AllInputsMine() recognizes the inputs as ours.
    {
        LOCK(wallet->cs_wallet);
        for (const auto& tip_tx : tip_txs) {
            wallet->AddToWallet(tip_tx, TxStateInMempool{});
        }
        wallet->AddToWallet(orig_tx_ref, TxStateInMempool{});
    }

    // Attempt to bump the fee with an explicit fee rate.
    // This triggers CheckFeeRate -> calculateCombinedBumpFee -> MiniMiner,
    // which fails because GatherClusters exceeds the 500-tx DoS limit.
    CCoinControl coin_control;
    coin_control.m_feerate = CFeeRate(2000); // 2000 sat/kvB
    std::vector<bilingual_str> errors;
    CAmount old_fee{0};
    CAmount new_fee{0};
    CMutableTransaction bumped_mtx;

    Result result = CreateRateBumpTransaction(
        *wallet, orig_txid, coin_control, errors,
        old_fee, new_fee, bumped_mtx,
        /*require_mine=*/true, /*outputs=*/{});

    // Use static_cast because Result has no operator<< for Boost.Test diagnostics.
    BOOST_CHECK_EQUAL(static_cast<int>(result), static_cast<int>(Result::WALLET_ERROR));
    BOOST_REQUIRE_EQUAL(errors.size(), 1U);
    BOOST_CHECK_EQUAL(errors[0].original, "Failed to calculate bump fees, because unconfirmed UTXOs depend on an enormous cluster of unconfirmed transactions.");
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace feebumper
} // namespace wallet
