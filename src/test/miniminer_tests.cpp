// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <node/mini_miner.h>
#include <random.h>
#include <txmempool.h>
#include <util/time.h>

#include <test/util/setup_common.h>
#include <test/util/txmempool.h>

#include <boost/test/unit_test.hpp>
#include <optional>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(miniminer_tests, TestingSetup)

const CAmount low_fee{CENT/2000}; // 500 ṩ
const CAmount med_fee{CENT/200}; // 5000 ṩ
const CAmount high_fee{CENT/10}; // 100_000 ṩ


static inline CTransactionRef make_tx(const std::vector<COutPoint>& inputs, size_t num_outputs)
{
    CMutableTransaction tx = CMutableTransaction();
    tx.vin.resize(inputs.size());
    tx.vout.resize(num_outputs);
    for (size_t i = 0; i < inputs.size(); ++i) {
        tx.vin[i].prevout = inputs[i];
    }
    for (size_t i = 0; i < num_outputs; ++i) {
        tx.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        // The actual input and output values of these transactions don't really
        // matter, since all accounting will use the entries' cached fees.
        tx.vout[i].nValue = COIN;
    }
    return MakeTransactionRef(tx);
}

static inline bool sanity_check(const std::vector<CTransactionRef>& transactions,
                                const std::map<COutPoint, CAmount>& bumpfees)
{
    // No negative bumpfees.
    for (const auto& [outpoint, fee] : bumpfees) {
        if (fee < 0) return false;
        if (fee == 0) continue;
        auto outpoint_ = outpoint; // structured bindings can't be captured in C++17, so we need to use a variable
        const bool found = std::any_of(transactions.cbegin(), transactions.cend(), [&](const auto& tx) {
            return outpoint_.hash == tx->GetHash() && outpoint_.n < tx->vout.size();
        });
        if (!found) return false;
    }
    for (const auto& tx : transactions) {
        // If tx has multiple outputs, they must all have the same bumpfee (if they exist).
        if (tx->vout.size() > 1) {
            std::set<CAmount> distinct_bumpfees;
            for (size_t i{0}; i < tx->vout.size(); ++i) {
                const auto bumpfee = bumpfees.find(COutPoint{tx->GetHash(), static_cast<uint32_t>(i)});
                if (bumpfee != bumpfees.end()) distinct_bumpfees.insert(bumpfee->second);
            }
            if (distinct_bumpfees.size() > 1) return false;
        }
    }
    return true;
}

template <typename Key, typename Value>
Value Find(const std::map<Key, Value>& map, const Key& key)
{
    auto it = map.find(key);
    BOOST_CHECK_MESSAGE(it != map.end(), strprintf("Cannot find %s", key.ToString()));
    return it->second;
}

BOOST_FIXTURE_TEST_CASE(miniminer_negative, TestChain100Setup)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    LOCK2(::cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    // Create a transaction that will be prioritised to have a negative modified fee.
    const CAmount positive_base_fee{1000};
    const CAmount negative_fee_delta{-50000};
    const CAmount negative_modified_fees{positive_base_fee + negative_fee_delta};
    BOOST_CHECK(negative_modified_fees < 0);
    const auto tx_mod_negative = make_tx({COutPoint{m_coinbase_txns[4]->GetHash(), 0}}, /*num_outputs=*/1);
    AddToMempool(pool, entry.Fee(positive_base_fee).FromTx(tx_mod_negative));
    pool.PrioritiseTransaction(tx_mod_negative->GetHash(), negative_fee_delta);
    const COutPoint only_outpoint{tx_mod_negative->GetHash(), 0};

    // When target feerate is 0, transactions with negative fees are not selected.
    node::MiniMiner mini_miner_target0(pool, {only_outpoint});
    BOOST_CHECK(mini_miner_target0.IsReadyToCalculate());
    const CFeeRate feerate_zero(0);
    mini_miner_target0.BuildMockTemplate(feerate_zero);
    // Check the quit condition:
    BOOST_CHECK(negative_modified_fees < feerate_zero.GetFee(Assert(pool.GetEntry(tx_mod_negative->GetHash()))->GetTxSize()));
    BOOST_CHECK(mini_miner_target0.GetMockTemplateTxids().empty());

    // With no target feerate, the template includes all transactions, even negative feerate ones.
    node::MiniMiner mini_miner_no_target(pool, {only_outpoint});
    BOOST_CHECK(mini_miner_no_target.IsReadyToCalculate());
    mini_miner_no_target.BuildMockTemplate(std::nullopt);
    const auto template_txids{mini_miner_no_target.GetMockTemplateTxids()};
    BOOST_CHECK_EQUAL(template_txids.size(), 1);
    BOOST_CHECK(template_txids.count(tx_mod_negative->GetHash()) > 0);
}

BOOST_FIXTURE_TEST_CASE(miniminer_1p1c, TestChain100Setup)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    LOCK2(::cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    // Create a parent tx0 and child tx1 with normal fees:
    const auto tx0 = make_tx({COutPoint{m_coinbase_txns[0]->GetHash(), 0}}, /*num_outputs=*/2);
    AddToMempool(pool, entry.Fee(med_fee).FromTx(tx0));
    const auto tx1 = make_tx({COutPoint{tx0->GetHash(), 0}}, /*num_outputs=*/1);
    AddToMempool(pool, entry.Fee(med_fee).FromTx(tx1));

    // Create a low-feerate parent tx2 and high-feerate child tx3 (cpfp)
    const auto tx2 = make_tx({COutPoint{m_coinbase_txns[1]->GetHash(), 0}}, /*num_outputs=*/2);
    AddToMempool(pool, entry.Fee(low_fee).FromTx(tx2));
    const auto tx3 = make_tx({COutPoint{tx2->GetHash(), 0}}, /*num_outputs=*/1);
    AddToMempool(pool, entry.Fee(high_fee).FromTx(tx3));

    // Create a parent tx4 and child tx5 where both have low fees
    const auto tx4 = make_tx({COutPoint{m_coinbase_txns[2]->GetHash(), 0}}, /*num_outputs=*/2);
    AddToMempool(pool, entry.Fee(low_fee).FromTx(tx4));
    const auto tx5 = make_tx({COutPoint{tx4->GetHash(), 0}}, /*num_outputs=*/1);
    AddToMempool(pool, entry.Fee(low_fee).FromTx(tx5));
    const CAmount tx5_delta{CENT/100};
    // Make tx5's modified fee much higher than its base fee. This should cause it to pass
    // the fee-related checks despite being low-feerate.
    pool.PrioritiseTransaction(tx5->GetHash(), tx5_delta);
    const CAmount tx5_mod_fee{low_fee + tx5_delta};

    // Create a high-feerate parent tx6, low-feerate child tx7
    const auto tx6 = make_tx({COutPoint{m_coinbase_txns[3]->GetHash(), 0}}, /*num_outputs=*/2);
    AddToMempool(pool, entry.Fee(high_fee).FromTx(tx6));
    const auto tx7 = make_tx({COutPoint{tx6->GetHash(), 0}}, /*num_outputs=*/1);
    AddToMempool(pool, entry.Fee(low_fee).FromTx(tx7));

    std::vector<COutPoint> all_unspent_outpoints({
        COutPoint{tx0->GetHash(), 1},
        COutPoint{tx1->GetHash(), 0},
        COutPoint{tx2->GetHash(), 1},
        COutPoint{tx3->GetHash(), 0},
        COutPoint{tx4->GetHash(), 1},
        COutPoint{tx5->GetHash(), 0},
        COutPoint{tx6->GetHash(), 1},
        COutPoint{tx7->GetHash(), 0}
    });
    for (const auto& outpoint : all_unspent_outpoints) BOOST_CHECK(!pool.isSpent(outpoint));

    std::vector<COutPoint> all_spent_outpoints({
        COutPoint{tx0->GetHash(), 0},
        COutPoint{tx2->GetHash(), 0},
        COutPoint{tx4->GetHash(), 0},
        COutPoint{tx6->GetHash(), 0}
    });
    for (const auto& outpoint : all_spent_outpoints) BOOST_CHECK(pool.GetConflictTx(outpoint) != nullptr);

    std::vector<COutPoint> all_parent_outputs({
        COutPoint{tx0->GetHash(), 0},
        COutPoint{tx0->GetHash(), 1},
        COutPoint{tx2->GetHash(), 0},
        COutPoint{tx2->GetHash(), 1},
        COutPoint{tx4->GetHash(), 0},
        COutPoint{tx4->GetHash(), 1},
        COutPoint{tx6->GetHash(), 0},
        COutPoint{tx6->GetHash(), 1}
    });


    std::vector<CTransactionRef> all_transactions{tx0, tx1, tx2, tx3, tx4, tx5, tx6, tx7};
    struct TxDimensions {
        int32_t vsize; CAmount mod_fee; CFeeRate feerate;
    };
    std::map<Txid, TxDimensions> tx_dims;
    for (const auto& tx : all_transactions) {
        const auto& entry{*Assert(pool.GetEntry(tx->GetHash()))};
        tx_dims.emplace(tx->GetHash(), TxDimensions{entry.GetTxSize(), entry.GetModifiedFee(),
                                              CFeeRate(entry.GetModifiedFee(), entry.GetTxSize())});
    }

    const std::vector<CFeeRate> various_normal_feerates({CFeeRate(0), CFeeRate(500), CFeeRate(999),
                                                         CFeeRate(1000), CFeeRate(2000), CFeeRate(2500),
                                                         CFeeRate(3333), CFeeRate(7800), CFeeRate(11199),
                                                         CFeeRate(23330), CFeeRate(50000), CFeeRate(5*CENT)});

    // All nonexistent entries have a bumpfee of zero, regardless of feerate
    std::vector<COutPoint> nonexistent_outpoints({ COutPoint{Txid::FromUint256(GetRandHash()), 0}, COutPoint{Txid::FromUint256(GetRandHash()), 3} });
    for (const auto& outpoint : nonexistent_outpoints) BOOST_CHECK(!pool.isSpent(outpoint));
    for (const auto& feerate : various_normal_feerates) {
        node::MiniMiner mini_miner(pool, nonexistent_outpoints);
        BOOST_CHECK(mini_miner.IsReadyToCalculate());
        auto bump_fees = mini_miner.CalculateBumpFees(feerate);
        BOOST_CHECK(!mini_miner.IsReadyToCalculate());
        BOOST_CHECK(sanity_check(all_transactions, bump_fees));
        BOOST_CHECK(bump_fees.size() == nonexistent_outpoints.size());
        for (const auto& outpoint: nonexistent_outpoints) {
            auto it = bump_fees.find(outpoint);
            BOOST_CHECK(it != bump_fees.end());
            BOOST_CHECK_EQUAL(it->second, 0);
        }
    }

    // Gather bump fees for all available UTXOs.
    for (const auto& target_feerate : various_normal_feerates) {
        node::MiniMiner mini_miner(pool, all_unspent_outpoints);
        BOOST_CHECK(mini_miner.IsReadyToCalculate());
        auto bump_fees = mini_miner.CalculateBumpFees(target_feerate);
        BOOST_CHECK(!mini_miner.IsReadyToCalculate());
        BOOST_CHECK(sanity_check(all_transactions, bump_fees));
        BOOST_CHECK_EQUAL(bump_fees.size(), all_unspent_outpoints.size());

        // Check tx0 bumpfee: no other bumper.
        const TxDimensions& tx0_dimensions = tx_dims.find(tx0->GetHash())->second;
        CAmount bumpfee0 = Find(bump_fees, COutPoint{tx0->GetHash(), 1});
        if (target_feerate <= tx0_dimensions.feerate) {
            BOOST_CHECK_EQUAL(bumpfee0, 0);
        } else {
            // Difference is fee to bump tx0 from current to target feerate.
            BOOST_CHECK_EQUAL(bumpfee0, target_feerate.GetFee(tx0_dimensions.vsize) - tx0_dimensions.mod_fee);
        }

        // Check tx2 bumpfee: assisted by tx3.
        const TxDimensions& tx2_dimensions = tx_dims.find(tx2->GetHash())->second;
        const TxDimensions& tx3_dimensions = tx_dims.find(tx3->GetHash())->second;
        const CFeeRate tx2_feerate = CFeeRate(tx2_dimensions.mod_fee + tx3_dimensions.mod_fee, tx2_dimensions.vsize + tx3_dimensions.vsize);
        CAmount bumpfee2 = Find(bump_fees, COutPoint{tx2->GetHash(), 1});
        if (target_feerate <= tx2_feerate) {
            // As long as target feerate is below tx3's ancestor feerate, there is no bump fee.
            BOOST_CHECK_EQUAL(bumpfee2, 0);
        } else {
            // Difference is fee to bump tx2 from current to target feerate, without tx3.
            BOOST_CHECK_EQUAL(bumpfee2, target_feerate.GetFee(tx2_dimensions.vsize) - tx2_dimensions.mod_fee);
        }

        // If tx5’s modified fees are sufficient for tx4 and tx5 to be picked
        // into the block, our prospective new transaction would not need to
        // bump tx4 when using tx4’s second output. If however even tx5’s
        // modified fee (which essentially indicates "effective feerate") is
        // not sufficient to bump tx4, using the second output of tx4 would
        // require our transaction to bump tx4 from scratch since we evaluate
        // transaction packages per ancestor sets and do not consider multiple
        // children’s fees.
        const TxDimensions& tx4_dimensions = tx_dims.find(tx4->GetHash())->second;
        const TxDimensions& tx5_dimensions = tx_dims.find(tx5->GetHash())->second;
        const CFeeRate tx4_feerate = CFeeRate(tx4_dimensions.mod_fee + tx5_dimensions.mod_fee, tx4_dimensions.vsize + tx5_dimensions.vsize);
        CAmount bumpfee4 = Find(bump_fees, COutPoint{tx4->GetHash(), 1});
        if (target_feerate <= tx4_feerate) {
            // As long as target feerate is below tx5's ancestor feerate, there is no bump fee.
            BOOST_CHECK_EQUAL(bumpfee4, 0);
        } else {
            // Difference is fee to bump tx4 from current to target feerate, without tx5.
            BOOST_CHECK_EQUAL(bumpfee4, target_feerate.GetFee(tx4_dimensions.vsize) - tx4_dimensions.mod_fee);
        }
    }
    // Spent outpoints should usually not be requested as they would not be
    // considered available. However, when they are explicitly requested, we
    // can calculate their bumpfee to facilitate RBF-replacements
    for (const auto& target_feerate : various_normal_feerates) {
        node::MiniMiner mini_miner_all_spent(pool, all_spent_outpoints);
        BOOST_CHECK(mini_miner_all_spent.IsReadyToCalculate());
        auto bump_fees_all_spent = mini_miner_all_spent.CalculateBumpFees(target_feerate);
        BOOST_CHECK(!mini_miner_all_spent.IsReadyToCalculate());
        BOOST_CHECK_EQUAL(bump_fees_all_spent.size(), all_spent_outpoints.size());
        node::MiniMiner mini_miner_all_parents(pool, all_parent_outputs);
        BOOST_CHECK(mini_miner_all_parents.IsReadyToCalculate());
        auto bump_fees_all_parents = mini_miner_all_parents.CalculateBumpFees(target_feerate);
        BOOST_CHECK(!mini_miner_all_parents.IsReadyToCalculate());
        BOOST_CHECK_EQUAL(bump_fees_all_parents.size(), all_parent_outputs.size());
        for (auto& bump_fees : {bump_fees_all_parents, bump_fees_all_spent}) {
            // For all_parents case, both outputs from the parent should have the same bump fee,
            // even though only one of them is in a to-be-replaced transaction.
            BOOST_CHECK(sanity_check(all_transactions, bump_fees));

            // Check tx0 bumpfee: no other bumper.
            const TxDimensions& tx0_dimensions = tx_dims.find(tx0->GetHash())->second;
            CAmount it0_spent = Find(bump_fees, COutPoint{tx0->GetHash(), 0});
            if (target_feerate <= tx0_dimensions.feerate) {
                BOOST_CHECK_EQUAL(it0_spent, 0);
            } else {
                // Difference is fee to bump tx0 from current to target feerate.
                BOOST_CHECK_EQUAL(it0_spent, target_feerate.GetFee(tx0_dimensions.vsize) - tx0_dimensions.mod_fee);
            }

            // Check tx2 bumpfee: no other bumper, because tx3 is to-be-replaced.
            const TxDimensions& tx2_dimensions = tx_dims.find(tx2->GetHash())->second;
            const CFeeRate tx2_feerate_unbumped = tx2_dimensions.feerate;
            auto it2_spent = Find(bump_fees, COutPoint{tx2->GetHash(), 0});
            if (target_feerate <= tx2_feerate_unbumped) {
                BOOST_CHECK_EQUAL(it2_spent, 0);
            } else {
                // Difference is fee to bump tx2 from current to target feerate, without tx3.
                BOOST_CHECK_EQUAL(it2_spent, target_feerate.GetFee(tx2_dimensions.vsize) - tx2_dimensions.mod_fee);
            }

            // Check tx4 bumpfee: no other bumper, because tx5 is to-be-replaced.
            const TxDimensions& tx4_dimensions = tx_dims.find(tx4->GetHash())->second;
            const CFeeRate tx4_feerate_unbumped = tx4_dimensions.feerate;
            auto it4_spent = Find(bump_fees, COutPoint{tx4->GetHash(), 0});
            if (target_feerate <= tx4_feerate_unbumped) {
                BOOST_CHECK_EQUAL(it4_spent, 0);
            } else {
                // Difference is fee to bump tx4 from current to target feerate, without tx5.
                BOOST_CHECK_EQUAL(it4_spent, target_feerate.GetFee(tx4_dimensions.vsize) - tx4_dimensions.mod_fee);
            }
        }
    }

    // Check m_inclusion_order for equivalent mempool- and manually-constructed MiniMiners.
    // (We cannot check bump fees in manually-constructed MiniMiners because it doesn't know what
    // outpoints are requested).
    std::vector<node::MiniMinerMempoolEntry> miniminer_info;
    {
        const int32_t tx0_vsize{tx_dims.at(tx0->GetHash()).vsize};
        const int32_t tx1_vsize{tx_dims.at(tx1->GetHash()).vsize};
        const int32_t tx2_vsize{tx_dims.at(tx2->GetHash()).vsize};
        const int32_t tx3_vsize{tx_dims.at(tx3->GetHash()).vsize};
        const int32_t tx4_vsize{tx_dims.at(tx4->GetHash()).vsize};
        const int32_t tx5_vsize{tx_dims.at(tx5->GetHash()).vsize};
        const int32_t tx6_vsize{tx_dims.at(tx6->GetHash()).vsize};
        const int32_t tx7_vsize{tx_dims.at(tx7->GetHash()).vsize};

        miniminer_info.emplace_back(tx0,/*vsize_self=*/tx0_vsize,/*vsize_ancestor=*/tx0_vsize,/*fee_self=*/med_fee,/*fee_ancestor=*/med_fee);
        miniminer_info.emplace_back(tx1,               tx1_vsize,       tx0_vsize + tx1_vsize,             med_fee,               2*med_fee);
        miniminer_info.emplace_back(tx2,               tx2_vsize,                   tx2_vsize,             low_fee,                 low_fee);
        miniminer_info.emplace_back(tx3,               tx3_vsize,       tx2_vsize + tx3_vsize,            high_fee,      low_fee + high_fee);
        miniminer_info.emplace_back(tx4,               tx4_vsize,                   tx4_vsize,             low_fee,                 low_fee);
        miniminer_info.emplace_back(tx5,               tx5_vsize,       tx4_vsize + tx5_vsize,         tx5_mod_fee,   low_fee + tx5_mod_fee);
        miniminer_info.emplace_back(tx6,               tx6_vsize,                   tx6_vsize,            high_fee,                high_fee);
        miniminer_info.emplace_back(tx7,               tx7_vsize,       tx6_vsize + tx7_vsize,             low_fee,      high_fee + low_fee);
    }
    std::map<Txid, std::set<Txid>> descendant_caches;
    descendant_caches.emplace(tx0->GetHash(), std::set<Txid>{tx0->GetHash(), tx1->GetHash()});
    descendant_caches.emplace(tx1->GetHash(), std::set<Txid>{tx1->GetHash()});
    descendant_caches.emplace(tx2->GetHash(), std::set<Txid>{tx2->GetHash(), tx3->GetHash()});
    descendant_caches.emplace(tx3->GetHash(), std::set<Txid>{tx3->GetHash()});
    descendant_caches.emplace(tx4->GetHash(), std::set<Txid>{tx4->GetHash(), tx5->GetHash()});
    descendant_caches.emplace(tx5->GetHash(), std::set<Txid>{tx5->GetHash()});
    descendant_caches.emplace(tx6->GetHash(), std::set<Txid>{tx6->GetHash(), tx7->GetHash()});
    descendant_caches.emplace(tx7->GetHash(), std::set<Txid>{tx7->GetHash()});

    node::MiniMiner miniminer_manual(miniminer_info, descendant_caches);
    // Use unspent outpoints to avoid entries being omitted.
    node::MiniMiner miniminer_pool(pool, all_unspent_outpoints);
    BOOST_CHECK(miniminer_manual.IsReadyToCalculate());
    BOOST_CHECK(miniminer_pool.IsReadyToCalculate());
    for (const auto& sequences : {miniminer_manual.Linearize(), miniminer_pool.Linearize()}) {
        // tx6 is selected first: high feerate with no parents to bump
        BOOST_CHECK_EQUAL(Find(sequences, tx6->GetHash()), 0);

        // tx2 + tx3 CPFP are selected next
        BOOST_CHECK_EQUAL(Find(sequences, tx2->GetHash()), 1);
        BOOST_CHECK_EQUAL(Find(sequences, tx3->GetHash()), 1);

        // tx4 + prioritised tx5 CPFP
        BOOST_CHECK_EQUAL(Find(sequences, tx4->GetHash()), 2);
        BOOST_CHECK_EQUAL(Find(sequences, tx5->GetHash()), 2);

        BOOST_CHECK_EQUAL(Find(sequences, tx0->GetHash()), 3);
        BOOST_CHECK_EQUAL(Find(sequences, tx1->GetHash()), 3);


        // tx7 is selected last: low feerate with no children
        BOOST_CHECK_EQUAL(Find(sequences, tx7->GetHash()), 4);
    }
}

BOOST_FIXTURE_TEST_CASE(miniminer_overlap, TestChain100Setup)
{
/*      Tx graph for `miniminer_overlap` unit test:
 *
 *     coinbase_tx [mined]        ... block-chain
 *  -------------------------------------------------
 *      /   |   \          \      ... mempool
 *     /    |    \         |
 *   tx0   tx1   tx2      tx4
 *  [low] [med] [high]   [high]
 *     \    |    /         |
 *      \   |   /         tx5
 *       \  |  /         [low]
 *         tx3          /     \
 *        [high]       tx6    tx7
 *                    [med]  [high]
 *
 *  NOTE:
 *  -> "low"/"med"/"high" denote the _absolute_ fee of each tx
 *  -> tx3 has 3 inputs and 3 outputs, all other txs have 1 input and 2 outputs
 *  -> tx3's feerate is lower than tx2's, as tx3 has more weight (due to having more inputs and outputs)
 *
 *  -> tx2_FR = high / tx2_vsize
 *  -> tx3_FR = high / tx3_vsize
 *  -> tx3_ASFR = (low+med+high+high) / (tx0_vsize + tx1_vsize + tx2_vsize + tx3_vsize)
 *  -> tx4_FR = high / tx4_vsize
 *  -> tx6_ASFR = (high+low+med) / (tx4_vsize + tx5_vsize + tx6_vsize)
 *  -> tx7_ASFR = (high+low+high) / (tx4_vsize + tx5_vsize + tx7_vsize) */

    CTxMemPool& pool = *Assert(m_node.mempool);
    LOCK2(::cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    // Create 3 parents of different feerates, and 1 child spending outputs from all 3 parents.
    const auto tx0 = make_tx({COutPoint{m_coinbase_txns[0]->GetHash(), 0}}, /*num_outputs=*/2);
    AddToMempool(pool, entry.Fee(low_fee).FromTx(tx0));
    const auto tx1 = make_tx({COutPoint{m_coinbase_txns[1]->GetHash(), 0}}, /*num_outputs=*/2);
    AddToMempool(pool, entry.Fee(med_fee).FromTx(tx1));
    const auto tx2 = make_tx({COutPoint{m_coinbase_txns[2]->GetHash(), 0}}, /*num_outputs=*/2);
    AddToMempool(pool, entry.Fee(high_fee).FromTx(tx2));
    const auto tx3 = make_tx({COutPoint{tx0->GetHash(), 0}, COutPoint{tx1->GetHash(), 0}, COutPoint{tx2->GetHash(), 0}}, /*num_outputs=*/3);
    AddToMempool(pool, entry.Fee(high_fee).FromTx(tx3));

    // Create 1 grandparent and 1 parent, then 2 children.
    const auto tx4 = make_tx({COutPoint{m_coinbase_txns[3]->GetHash(), 0}}, /*num_outputs=*/2);
    AddToMempool(pool, entry.Fee(high_fee).FromTx(tx4));
    const auto tx5 = make_tx({COutPoint{tx4->GetHash(), 0}}, /*num_outputs=*/3);
    AddToMempool(pool, entry.Fee(low_fee).FromTx(tx5));
    const auto tx6 = make_tx({COutPoint{tx5->GetHash(), 0}}, /*num_outputs=*/2);
    AddToMempool(pool, entry.Fee(med_fee).FromTx(tx6));
    const auto tx7 = make_tx({COutPoint{tx5->GetHash(), 1}}, /*num_outputs=*/2);
    AddToMempool(pool, entry.Fee(high_fee).FromTx(tx7));

    std::vector<CTransactionRef> all_transactions{tx0, tx1, tx2, tx3, tx4, tx5, tx6, tx7};
    std::vector<int64_t> tx_vsizes;
    tx_vsizes.reserve(all_transactions.size());
    for (const auto& tx : all_transactions) tx_vsizes.push_back(GetVirtualTransactionSize(*tx));

    std::vector<COutPoint> all_unspent_outpoints({
        COutPoint{tx0->GetHash(), 1},
        COutPoint{tx1->GetHash(), 1},
        COutPoint{tx2->GetHash(), 1},
        COutPoint{tx3->GetHash(), 0},
        COutPoint{tx3->GetHash(), 1},
        COutPoint{tx3->GetHash(), 2},
        COutPoint{tx4->GetHash(), 1},
        COutPoint{tx5->GetHash(), 2},
        COutPoint{tx6->GetHash(), 0},
        COutPoint{tx7->GetHash(), 0}
    });
    for (const auto& outpoint : all_unspent_outpoints) BOOST_CHECK(!pool.isSpent(outpoint));

    const auto tx2_feerate = CFeeRate(high_fee, tx_vsizes[2]);
    const auto tx3_feerate = CFeeRate(high_fee, tx_vsizes[3]);
    // tx3's feerate is lower than tx2's. same fee, different weight.
    BOOST_CHECK(tx2_feerate > tx3_feerate);
    const auto tx3_anc_feerate = CFeeRate(low_fee + med_fee + high_fee + high_fee, tx_vsizes[0] + tx_vsizes[1] + tx_vsizes[2] + tx_vsizes[3]);
    const auto& tx3_entry{*Assert(pool.GetEntry(tx3->GetHash()))};

    // Check that ancestor feerate is calculated correctly.
    size_t dummy_count{0};
    CAmount mod_fees{0};
    size_t ancestor_vsize{0};
    pool.CalculateAncestorData(tx3_entry, dummy_count, ancestor_vsize, mod_fees);
    BOOST_CHECK(tx3_anc_feerate == CFeeRate(mod_fees, ancestor_vsize));

    const auto tx4_feerate = CFeeRate(high_fee, tx_vsizes[4]);
    const auto tx6_anc_feerate = CFeeRate(high_fee + low_fee + med_fee, tx_vsizes[4] + tx_vsizes[5] + tx_vsizes[6]);
    const auto& tx6_entry{*Assert(pool.GetEntry(tx6->GetHash()))};

    pool.CalculateAncestorData(tx6_entry, dummy_count, ancestor_vsize, mod_fees);
    BOOST_CHECK(tx6_anc_feerate == CFeeRate(mod_fees, ancestor_vsize));
    const auto tx7_anc_feerate = CFeeRate(high_fee + low_fee + high_fee, tx_vsizes[4] + tx_vsizes[5] + tx_vsizes[7]);
    const auto& tx7_entry{*Assert(pool.GetEntry(tx7->GetHash()))};

    pool.CalculateAncestorData(tx7_entry, dummy_count, ancestor_vsize, mod_fees);
    BOOST_CHECK(tx7_anc_feerate == CFeeRate(mod_fees, ancestor_vsize));
    BOOST_CHECK(tx4_feerate > tx6_anc_feerate);
    BOOST_CHECK(tx4_feerate > tx7_anc_feerate);

    // Extremely high feerate: everybody's bumpfee is from their full ancestor set.
    {
        node::MiniMiner mini_miner(pool, all_unspent_outpoints);
        const CFeeRate very_high_feerate(COIN);
        BOOST_CHECK(tx3_anc_feerate < very_high_feerate);
        BOOST_CHECK(mini_miner.IsReadyToCalculate());
        auto bump_fees = mini_miner.CalculateBumpFees(very_high_feerate);
        BOOST_CHECK_EQUAL(bump_fees.size(), all_unspent_outpoints.size());
        BOOST_CHECK(!mini_miner.IsReadyToCalculate());
        BOOST_CHECK(sanity_check(all_transactions, bump_fees));
        const auto tx0_bumpfee = bump_fees.find(COutPoint{tx0->GetHash(), 1});
        BOOST_CHECK(tx0_bumpfee != bump_fees.end());
        BOOST_CHECK_EQUAL(tx0_bumpfee->second, very_high_feerate.GetFee(tx_vsizes[0]) - low_fee);
        const auto tx3_bumpfee = bump_fees.find(COutPoint{tx3->GetHash(), 0});
        BOOST_CHECK(tx3_bumpfee != bump_fees.end());
        BOOST_CHECK_EQUAL(tx3_bumpfee->second,
            very_high_feerate.GetFee(tx_vsizes[0] + tx_vsizes[1] + tx_vsizes[2] + tx_vsizes[3]) - (low_fee + med_fee + high_fee + high_fee));
        const auto tx6_bumpfee = bump_fees.find(COutPoint{tx6->GetHash(), 0});
        BOOST_CHECK(tx6_bumpfee != bump_fees.end());
        BOOST_CHECK_EQUAL(tx6_bumpfee->second,
            very_high_feerate.GetFee(tx_vsizes[4] + tx_vsizes[5] + tx_vsizes[6]) - (high_fee + low_fee + med_fee));
        const auto tx7_bumpfee = bump_fees.find(COutPoint{tx7->GetHash(), 0});
        BOOST_CHECK(tx7_bumpfee != bump_fees.end());
        BOOST_CHECK_EQUAL(tx7_bumpfee->second,
            very_high_feerate.GetFee(tx_vsizes[4] + tx_vsizes[5] + tx_vsizes[7]) - (high_fee + low_fee + high_fee));
        // Total fees: if spending multiple outputs from tx3 don't double-count fees.
        node::MiniMiner mini_miner_total_tx3(pool, {COutPoint{tx3->GetHash(), 0}, COutPoint{tx3->GetHash(), 1}});
        BOOST_CHECK(mini_miner_total_tx3.IsReadyToCalculate());
        const auto tx3_bump_fee = mini_miner_total_tx3.CalculateTotalBumpFees(very_high_feerate);
        BOOST_CHECK(!mini_miner_total_tx3.IsReadyToCalculate());
        BOOST_CHECK(tx3_bump_fee.has_value());
        BOOST_CHECK_EQUAL(tx3_bump_fee.value(),
            very_high_feerate.GetFee(tx_vsizes[0] + tx_vsizes[1] + tx_vsizes[2] + tx_vsizes[3]) - (low_fee + med_fee + high_fee + high_fee));
        // Total fees: if spending both tx6 and tx7, don't double-count fees.
        node::MiniMiner mini_miner_tx6_tx7(pool, {COutPoint{tx6->GetHash(), 0}, COutPoint{tx7->GetHash(), 0}});
        BOOST_CHECK(mini_miner_tx6_tx7.IsReadyToCalculate());
        const auto tx6_tx7_bumpfee = mini_miner_tx6_tx7.CalculateTotalBumpFees(very_high_feerate);
        BOOST_CHECK(!mini_miner_tx6_tx7.IsReadyToCalculate());
        BOOST_CHECK(tx6_tx7_bumpfee.has_value());
        BOOST_CHECK_EQUAL(tx6_tx7_bumpfee.value(),
            very_high_feerate.GetFee(tx_vsizes[4] + tx_vsizes[5] + tx_vsizes[6] + tx_vsizes[7]) - (high_fee + low_fee + med_fee + high_fee));
    }
    // Feerate just below tx4: tx6 and tx7 have different bump fees.
    {
        const auto just_below_tx4 = CFeeRate(tx4_feerate.GetFeePerK() - 5);
        node::MiniMiner mini_miner(pool, all_unspent_outpoints);
        BOOST_CHECK(mini_miner.IsReadyToCalculate());
        auto bump_fees = mini_miner.CalculateBumpFees(just_below_tx4);
        BOOST_CHECK(!mini_miner.IsReadyToCalculate());
        BOOST_CHECK_EQUAL(bump_fees.size(), all_unspent_outpoints.size());
        BOOST_CHECK(sanity_check(all_transactions, bump_fees));
        const auto tx6_bumpfee = bump_fees.find(COutPoint{tx6->GetHash(), 0});
        BOOST_CHECK(tx6_bumpfee != bump_fees.end());
        BOOST_CHECK_EQUAL(tx6_bumpfee->second, just_below_tx4.GetFee(tx_vsizes[5] + tx_vsizes[6]) - (low_fee + med_fee));
        const auto tx7_bumpfee = bump_fees.find(COutPoint{tx7->GetHash(), 0});
        BOOST_CHECK(tx7_bumpfee != bump_fees.end());
        BOOST_CHECK_EQUAL(tx7_bumpfee->second, just_below_tx4.GetFee(tx_vsizes[5] + tx_vsizes[7]) - (low_fee + high_fee));
        // Total fees: if spending both tx6 and tx7, don't double-count fees.
        node::MiniMiner mini_miner_tx6_tx7(pool, {COutPoint{tx6->GetHash(), 0}, COutPoint{tx7->GetHash(), 0}});
        BOOST_CHECK(mini_miner_tx6_tx7.IsReadyToCalculate());
        const auto tx6_tx7_bumpfee = mini_miner_tx6_tx7.CalculateTotalBumpFees(just_below_tx4);
        BOOST_CHECK(!mini_miner_tx6_tx7.IsReadyToCalculate());
        BOOST_CHECK(tx6_tx7_bumpfee.has_value());
        BOOST_CHECK_EQUAL(tx6_tx7_bumpfee.value(), just_below_tx4.GetFee(tx_vsizes[5] + tx_vsizes[6]) - (low_fee + med_fee));
    }
    // Feerate between tx6 and tx7's ancestor feerates: don't need to bump tx5 because tx7 already does.
    {
        const auto just_above_tx6 = CFeeRate(med_fee + 10, tx_vsizes[6]);
        BOOST_CHECK(just_above_tx6 <= CFeeRate(low_fee + high_fee, tx_vsizes[5] + tx_vsizes[7]));
        node::MiniMiner mini_miner(pool, all_unspent_outpoints);
        BOOST_CHECK(mini_miner.IsReadyToCalculate());
        auto bump_fees = mini_miner.CalculateBumpFees(just_above_tx6);
        BOOST_CHECK(!mini_miner.IsReadyToCalculate());
        BOOST_CHECK_EQUAL(bump_fees.size(), all_unspent_outpoints.size());
        BOOST_CHECK(sanity_check(all_transactions, bump_fees));
        const auto tx6_bumpfee = bump_fees.find(COutPoint{tx6->GetHash(), 0});
        BOOST_CHECK(tx6_bumpfee != bump_fees.end());
        BOOST_CHECK_EQUAL(tx6_bumpfee->second, just_above_tx6.GetFee(tx_vsizes[6]) - (med_fee));
        const auto tx7_bumpfee = bump_fees.find(COutPoint{tx7->GetHash(), 0});
        BOOST_CHECK(tx7_bumpfee != bump_fees.end());
        BOOST_CHECK_EQUAL(tx7_bumpfee->second, 0);
    }
    // Check linearization order
    std::vector<node::MiniMinerMempoolEntry> miniminer_info;
    miniminer_info.emplace_back(tx0,/*vsize_self=*/tx_vsizes[0],                     /*vsize_ancestor=*/tx_vsizes[0], /*fee_self=*/low_fee,   /*fee_ancestor=*/low_fee);
    miniminer_info.emplace_back(tx1,               tx_vsizes[1],                                        tx_vsizes[1],              med_fee,                    med_fee);
    miniminer_info.emplace_back(tx2,               tx_vsizes[2],                                        tx_vsizes[2],             high_fee,                   high_fee);
    miniminer_info.emplace_back(tx3,               tx_vsizes[3], tx_vsizes[0]+tx_vsizes[1]+tx_vsizes[2]+tx_vsizes[3],             high_fee, low_fee+med_fee+2*high_fee);
    miniminer_info.emplace_back(tx4,               tx_vsizes[4],                                        tx_vsizes[4],             high_fee,                   high_fee);
    miniminer_info.emplace_back(tx5,               tx_vsizes[5],                           tx_vsizes[4]+tx_vsizes[5],              low_fee,         low_fee + high_fee);
    miniminer_info.emplace_back(tx6,               tx_vsizes[6],              tx_vsizes[4]+tx_vsizes[5]+tx_vsizes[6],              med_fee,   high_fee+low_fee+med_fee);
    miniminer_info.emplace_back(tx7,               tx_vsizes[7],              tx_vsizes[4]+tx_vsizes[5]+tx_vsizes[7],             high_fee,  high_fee+low_fee+high_fee);

    std::map<Txid, std::set<Txid>> descendant_caches;
    descendant_caches.emplace(tx0->GetHash(), std::set<Txid>{tx0->GetHash(), tx3->GetHash()});
    descendant_caches.emplace(tx1->GetHash(), std::set<Txid>{tx1->GetHash(), tx3->GetHash()});
    descendant_caches.emplace(tx2->GetHash(), std::set<Txid>{tx2->GetHash(), tx3->GetHash()});
    descendant_caches.emplace(tx3->GetHash(), std::set<Txid>{tx3->GetHash()});
    descendant_caches.emplace(tx4->GetHash(), std::set<Txid>{tx4->GetHash(), tx5->GetHash(), tx6->GetHash(), tx7->GetHash()});
    descendant_caches.emplace(tx5->GetHash(), std::set<Txid>{tx5->GetHash(), tx6->GetHash(), tx7->GetHash()});
    descendant_caches.emplace(tx6->GetHash(), std::set<Txid>{tx6->GetHash()});
    descendant_caches.emplace(tx7->GetHash(), std::set<Txid>{tx7->GetHash()});

    node::MiniMiner miniminer_manual(miniminer_info, descendant_caches);
    // Use unspent outpoints to avoid entries being omitted.
    node::MiniMiner miniminer_pool(pool, all_unspent_outpoints);
    BOOST_CHECK(miniminer_manual.IsReadyToCalculate());
    BOOST_CHECK(miniminer_pool.IsReadyToCalculate());
    for (const auto& sequences : {miniminer_manual.Linearize(), miniminer_pool.Linearize()}) {
        // tx2 and tx4 selected first: high feerate with nothing to bump
        BOOST_CHECK_EQUAL(Find(sequences, tx4->GetHash()), 0);
        BOOST_CHECK_EQUAL(Find(sequences, tx2->GetHash()), 1);

        // tx5 + tx7 CPFP
        BOOST_CHECK_EQUAL(Find(sequences, tx5->GetHash()), 2);
        BOOST_CHECK_EQUAL(Find(sequences, tx7->GetHash()), 2);

        // tx0 and tx1 CPFP'd by tx3
        BOOST_CHECK_EQUAL(Find(sequences, tx0->GetHash()), 3);
        BOOST_CHECK_EQUAL(Find(sequences, tx1->GetHash()), 3);
        BOOST_CHECK_EQUAL(Find(sequences, tx3->GetHash()), 3);

        // tx6 at medium feerate
        BOOST_CHECK_EQUAL(Find(sequences, tx6->GetHash()), 4);
    }
}
BOOST_FIXTURE_TEST_CASE(calculate_cluster, TestChain100Setup)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    LOCK2(cs_main, pool.cs);

    // Add 500 transactions in 10 clusters
    std::vector<Txid> last_txs;
    std::vector<Txid> chain_txids;
    auto& lasttx = m_coinbase_txns[0];
    TestMemPoolEntryHelper entry;
    for (int cluster_count=0; cluster_count < 10; ++cluster_count) {
        // Add chain of size 50
        lasttx = m_coinbase_txns[cluster_count];
        for (auto i{0}; i < 50; ++i) {
            const auto tx = make_tx({COutPoint{lasttx->GetHash(), 0}}, /*num_outputs=*/1);
            AddToMempool(pool, entry.Fee(CENT).FromTx(tx));
            chain_txids.push_back(tx->GetHash());
            lasttx = tx;
        }
        last_txs.emplace_back(lasttx->GetHash());
    }
    const auto cluster_500tx = pool.GatherClusters(last_txs);
    CTxMemPool::setEntries cluster_500tx_set{cluster_500tx.begin(), cluster_500tx.end()};
    BOOST_CHECK_EQUAL(cluster_500tx.size(), cluster_500tx_set.size());
    const auto vec_iters_500 = pool.GetIterVec(chain_txids);
    for (const auto& iter : vec_iters_500) BOOST_CHECK(cluster_500tx_set.count(iter));

    // GatherClusters stops at 500 transactions.
    const auto tx_501 = make_tx({COutPoint{lasttx->GetHash(), 0}}, /*num_outputs=*/1);
    AddToMempool(pool, entry.Fee(CENT).FromTx(tx_501));
    const auto cluster_501 = pool.GatherClusters(last_txs);
    BOOST_CHECK_EQUAL(cluster_501.size(), 0);

    /* Zig Zag cluster:
     * txp0     txp1     txp2    ...  txp30  txp31
     *    \    /    \   /   \            \   /
     *     txc0     txc1    txc2  ...    txc30
     * Note that each transaction's ancestor size is 1 or 3, and each descendant size is 1, 2 or 3.
     * However, all of these transactions are in the same cluster. */
    std::vector<Txid> zigzag_txids;
    for (auto p{0}; p < 32; ++p) {
        const auto txp = make_tx({COutPoint{Txid::FromUint256(GetRandHash()), 0}}, /*num_outputs=*/2);
        AddToMempool(pool, entry.Fee(CENT).FromTx(txp));
        zigzag_txids.push_back(txp->GetHash());
    }
    for (auto c{0}; c < 31; ++c) {
        const auto txc = make_tx({COutPoint{zigzag_txids[c], 1}, COutPoint{zigzag_txids[c+1], 0}}, /*num_outputs=*/1);
        AddToMempool(pool, entry.Fee(CENT).FromTx(txc));
        zigzag_txids.push_back(txc->GetHash());
    }
    const auto vec_iters_zigzag = pool.GetIterVec(zigzag_txids);
    // It doesn't matter which tx we calculate cluster for, everybody is in it.
    const std::vector<size_t> indices{0, 22, 52, zigzag_txids.size() - 1};
    for (const auto index : indices) {
        const auto cluster = pool.GatherClusters({zigzag_txids[index]});
        BOOST_CHECK_EQUAL(cluster.size(), zigzag_txids.size());
        CTxMemPool::setEntries clusterset{cluster.begin(), cluster.end()};
        BOOST_CHECK_EQUAL(cluster.size(), clusterset.size());
        for (const auto& iter : vec_iters_zigzag) BOOST_CHECK(clusterset.count(iter));
    }
}

BOOST_FIXTURE_TEST_CASE(manual_ctor, TestChain100Setup)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    LOCK2(cs_main, pool.cs);
    {
        // 3 pairs of grandparent + fee-bumping parent, plus 1 low-feerate child.
        // 0 fee + high fee
        auto grandparent_zero_fee = make_tx({{m_coinbase_txns.at(0)->GetHash(), 0}}, 1);
        auto parent_high_feerate = make_tx({{grandparent_zero_fee->GetHash(), 0}}, 1);
        // double low fee + med fee
        auto grandparent_double_low_feerate = make_tx({{m_coinbase_txns.at(2)->GetHash(), 0}}, 1);
        auto parent_med_feerate = make_tx({{grandparent_double_low_feerate->GetHash(), 0}}, 1);
        // low fee + double low fee
        auto grandparent_low_feerate = make_tx({{m_coinbase_txns.at(1)->GetHash(), 0}}, 1);
        auto parent_double_low_feerate = make_tx({{grandparent_low_feerate->GetHash(), 0}}, 1);
        // child is below the cpfp package feerates because it is larger than everything else
        auto child = make_tx({{parent_high_feerate->GetHash(), 0}, {parent_double_low_feerate->GetHash(), 0}, {parent_med_feerate->GetHash(), 0}}, 1);

        // We artificially record each transaction (except the child) with a uniform vsize of 100vB.
        const int64_t tx_vsize{100};
        const int64_t child_vsize{1000};

        std::vector<node::MiniMinerMempoolEntry> miniminer_info;
        miniminer_info.emplace_back(grandparent_zero_fee,          /*vsize_self=*/tx_vsize,/*vsize_ancestor=*/tx_vsize, /*fee_self=*/0,/*fee_ancestor=*/0);
        miniminer_info.emplace_back(parent_high_feerate,                          tx_vsize,                 2*tx_vsize, high_fee,      high_fee);
        miniminer_info.emplace_back(grandparent_double_low_feerate,               tx_vsize,                   tx_vsize, 2*low_fee,     2*low_fee);
        miniminer_info.emplace_back(parent_med_feerate,                           tx_vsize,                 2*tx_vsize, med_fee,       2*low_fee+med_fee);
        miniminer_info.emplace_back(grandparent_low_feerate,                      tx_vsize,                   tx_vsize, low_fee,       low_fee);
        miniminer_info.emplace_back(parent_double_low_feerate,                    tx_vsize,                 2*tx_vsize, 2*low_fee,     3*low_fee);
        miniminer_info.emplace_back(child,                                     child_vsize,     6*tx_vsize+child_vsize, low_fee,       high_fee+med_fee+6*low_fee);
        std::map<Txid, std::set<Txid>> descendant_caches;
        descendant_caches.emplace(grandparent_zero_fee->GetHash(), std::set<Txid>{grandparent_zero_fee->GetHash(), parent_high_feerate->GetHash(), child->GetHash()});
        descendant_caches.emplace(grandparent_low_feerate->GetHash(), std::set<Txid>{grandparent_low_feerate->GetHash(), parent_double_low_feerate->GetHash(), child->GetHash()});
        descendant_caches.emplace(grandparent_double_low_feerate->GetHash(), std::set<Txid>{grandparent_double_low_feerate->GetHash(), parent_med_feerate->GetHash(), child->GetHash()});
        descendant_caches.emplace(parent_high_feerate->GetHash(), std::set<Txid>{parent_high_feerate->GetHash(), child->GetHash()});
        descendant_caches.emplace(parent_med_feerate->GetHash(), std::set<Txid>{parent_med_feerate->GetHash(), child->GetHash()});
        descendant_caches.emplace(parent_double_low_feerate->GetHash(), std::set<Txid>{parent_double_low_feerate->GetHash(), child->GetHash()});
        descendant_caches.emplace(child->GetHash(), std::set<Txid>{child->GetHash()});

        node::MiniMiner miniminer_manual(miniminer_info, descendant_caches);
        BOOST_CHECK(miniminer_manual.IsReadyToCalculate());
        const auto sequences{miniminer_manual.Linearize()};

        // CPFP zero + high
        BOOST_CHECK_EQUAL(sequences.at(grandparent_zero_fee->GetHash()), 0);
        BOOST_CHECK_EQUAL(sequences.at(parent_high_feerate->GetHash()), 0);

        // CPFP double low + med
        BOOST_CHECK_EQUAL(sequences.at(grandparent_double_low_feerate->GetHash()), 1);
        BOOST_CHECK_EQUAL(sequences.at(parent_med_feerate->GetHash()), 1);

        // CPFP low + double low
        BOOST_CHECK_EQUAL(sequences.at(grandparent_low_feerate->GetHash()), 2);
        BOOST_CHECK_EQUAL(sequences.at(parent_double_low_feerate->GetHash()), 2);

        // Child at the end
        BOOST_CHECK_EQUAL(sequences.at(child->GetHash()), 3);
    }
}

BOOST_AUTO_TEST_SUITE_END()
