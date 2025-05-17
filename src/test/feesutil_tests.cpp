// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/mempool_entry.h>
#include <policy/fees_util.h>
#include <test/util/random.h>
#include <test/util/txmempool.h>
#include <txmempool.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(feesutil_tests, BasicTestingSetup)

static inline CTransactionRef make_tx(const std::vector<COutPoint>& outpoints, int num_outputs = 1)
{
    CMutableTransaction tx;
    tx.vin.reserve(outpoints.size());
    tx.vout.resize(num_outputs);
    for (const auto& outpoint : outpoints) {
        tx.vin.emplace_back(outpoint);
    }
    for (int i = 0; i < num_outputs; ++i) {
        auto scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        tx.vout.emplace_back(COIN, scriptPubKey);
    }
    return MakeTransactionRef(tx);
}

void verifyTxsInclusion(const std::vector<Txid>& txs_vec, const std::set<Txid>& txs_set)
{
    BOOST_CHECK(txs_vec.size() == txs_set.size());
    for (const auto txid : txs_vec) {
        BOOST_CHECK(txs_set.find(txid) != txs_set.end());
    }
}

void validateTxRelation(const Txid& txid, const std::vector<Txid>& ancestors, const std::vector<Txid>& descendants, const TxAncestorsAndDescendants& txAncestorsAndDescendants)
{
    const auto iter = txAncestorsAndDescendants.find(txid);
    BOOST_CHECK(iter != txAncestorsAndDescendants.end());
    const auto calc_ancestors = std::get<0>(iter->second);
    verifyTxsInclusion(ancestors, calc_ancestors);
    const auto calc_descendants = std::get<1>(iter->second);
    verifyTxsInclusion(descendants, calc_descendants);
}

BOOST_AUTO_TEST_CASE(ComputingTxAncestorsAndDescendants)
{
    TestMemPoolEntryHelper entry;
    auto create_and_add_tx = [&](std::vector<RemovedMempoolTransactionInfo>& transactions, const std::vector<COutPoint>& outpoints, int num_outputs = 1) {
        const CTransactionRef tx = make_tx(outpoints, num_outputs);
        transactions.emplace_back(entry.FromTx(tx));
        return tx;
    };

    // Test 20 unique transactions
    {
        std::vector<RemovedMempoolTransactionInfo> transactions;
        transactions.reserve(20);

        for (auto i = 0; i < 20; ++i) {
            const std::vector<COutPoint> outpoints{COutPoint(Txid::FromUint256(m_rng.rand256()), 0)};
            create_and_add_tx(transactions, outpoints);
        }

        const auto txAncestorsAndDescendants = GetTxAncestorsAndDescendants(transactions);
        BOOST_CHECK_EQUAL(txAncestorsAndDescendants.size(), transactions.size());

        for (auto& tx : transactions) {
            const Txid txid = tx.info.m_tx->GetHash();
            const auto txiter = txAncestorsAndDescendants.find(txid);
            BOOST_CHECK(txiter != txAncestorsAndDescendants.end());
            const auto ancestors = std::get<0>(txiter->second);
            const auto descendants = std::get<1>(txiter->second);
            BOOST_CHECK(ancestors.size() == 1);
            BOOST_CHECK(descendants.size() == 1);
        }
    }
    // Test 3 Linear transactions clusters
    /*
            Linear Packages
            A     B     C    D
            |     |     |    |
            E     H     J    K
            |     |
            F     I
            |
            G
    */
    {
        std::vector<RemovedMempoolTransactionInfo> transactions;
        transactions.reserve(11);

        // Create transaction A B C D
        for (auto i = 0; i < 4; ++i) {
            const std::vector<COutPoint> outpoints{COutPoint(Txid::FromUint256(m_rng.rand256()), 0)};
            create_and_add_tx(transactions, outpoints);
        }

        // Create cluster A children ---> E->F->G
        std::vector<COutPoint> outpoints{COutPoint(transactions[0].info.m_tx->GetHash(), 0)};
        for (auto i = 0; i < 3; ++i) {
            const CTransactionRef tx = create_and_add_tx(transactions, outpoints);
            outpoints = {COutPoint(tx->GetHash(), 0)};
        }

        // Create cluster B children ---> H->I
        outpoints = {COutPoint(transactions[1].info.m_tx->GetHash(), 0)};
        for (size_t i = 0; i < 2; ++i) {
            const CTransactionRef tx = create_and_add_tx(transactions, outpoints);
            outpoints = {COutPoint(tx->GetHash(), 0)};
        }

        // Create cluster C child ---> J
        outpoints = {COutPoint(transactions[2].info.m_tx->GetHash(), 0)};
        create_and_add_tx(transactions, outpoints);

        // Create cluster D child ---> K
        outpoints = {COutPoint(transactions[3].info.m_tx->GetHash(), 0)};
        create_and_add_tx(transactions, outpoints);

        const auto txAncestorsAndDescendants = GetTxAncestorsAndDescendants(transactions);
        BOOST_CHECK_EQUAL(txAncestorsAndDescendants.size(), transactions.size());

        // Check tx A topology;
        {
            const Txid txA_Id = transactions[0].info.m_tx->GetHash();
            std::vector<Txid> descendants{txA_Id};
            for (auto i = 4; i <= 6; i++) { // Add E, F, G
                descendants.emplace_back(transactions[i].info.m_tx->GetHash());
            }
            validateTxRelation(txA_Id, std::vector<Txid>{txA_Id}, descendants, txAncestorsAndDescendants);
        }
        // Check tx G topology;
        {
            const Txid txG_Id = transactions[6].info.m_tx->GetHash();
            std::vector<Txid> ancestors{transactions[0].info.m_tx->GetHash()};
            for (auto i = 4; i <= 6; i++) { // Add E, F, G
                ancestors.emplace_back(transactions[i].info.m_tx->GetHash());
            }
            validateTxRelation(txG_Id, ancestors, std::vector<Txid>{txG_Id}, txAncestorsAndDescendants);
        }
        // Check tx B topology;
        {
            const Txid txB_Id = transactions[1].info.m_tx->GetHash();
            std::vector<Txid> descendants{txB_Id};
            for (auto i = 7; i <= 8; i++) { // Add H, I
                descendants.emplace_back(transactions[i].info.m_tx->GetHash());
            }
            validateTxRelation(txB_Id, std::vector<Txid>{txB_Id}, descendants, txAncestorsAndDescendants);
        }
        // Check tx H topology;
        {
            const Txid txH_Id = transactions[7].info.m_tx->GetHash();
            const std::vector<Txid> ancestors{txH_Id, transactions[1].info.m_tx->GetHash()};   // H, B
            const std::vector<Txid> descendants{txH_Id, transactions[8].info.m_tx->GetHash()}; // H, I
            validateTxRelation(txH_Id, ancestors, descendants, txAncestorsAndDescendants);
        }
        // Check tx C topology;
        {
            const Txid txC_Id = transactions[2].info.m_tx->GetHash();
            const std::vector<Txid> descendants{txC_Id, transactions[9].info.m_tx->GetHash()}; // C, J
            validateTxRelation(txC_Id, std::vector<Txid>{txC_Id}, descendants, txAncestorsAndDescendants);
        }
        // Check tx D topology;
        {
            const Txid txD_Id = transactions[3].info.m_tx->GetHash();
            const std::vector<Txid> descendants{txD_Id, transactions[10].info.m_tx->GetHash()}; // D, K
            validateTxRelation(txD_Id, std::vector<Txid>{txD_Id}, descendants, txAncestorsAndDescendants);
        }
    }

    /* Unique transactions with a cluster of transactions

           Cluster A                      Cluster B
              A                               B
            /   \                           /   \
           /     \                         /     \
          C       D                       I       J
        /   \     |                               |
       /     \    |                               |
      E       F   H                               K
      \       /
       \     /
          G
    */
    {
        std::vector<RemovedMempoolTransactionInfo> transactions;
        transactions.reserve(11);

        // Create transaction A and B
        for (auto i = 0; i < 2; ++i) {
            const std::vector<COutPoint> outpoints{COutPoint(Txid::FromUint256(m_rng.rand256()), 0)};
            create_and_add_tx(transactions, outpoints, /*num_outputs=*/2);
        }

        // Cluster 1 Topology
        // Create a child for A ---> C
        std::vector<COutPoint> outpoints{COutPoint(transactions[0].info.m_tx->GetHash(), 0)};
        const CTransactionRef tx_C = create_and_add_tx(transactions, outpoints, /*num_outputs=*/2);

        // Create a child for A ---> D
        outpoints = {COutPoint(transactions[0].info.m_tx->GetHash(), 1)};
        create_and_add_tx(transactions, outpoints);

        // Create a child for C ---> E
        outpoints = {COutPoint(tx_C->GetHash(), 0)};
        create_and_add_tx(transactions, outpoints);

        // Create a child for C ---> F
        outpoints = {COutPoint(tx_C->GetHash(), 1)};
        create_and_add_tx(transactions, outpoints);

        // Create a child for E and F  ---> G
        outpoints = {COutPoint(transactions[4].info.m_tx->GetHash(), 0), COutPoint(transactions[5].info.m_tx->GetHash(), 0)};
        create_and_add_tx(transactions, outpoints);

        // Create a child for D ---> H
        outpoints = {COutPoint(transactions[3].info.m_tx->GetHash(), 0)};
        create_and_add_tx(transactions, outpoints);

        // Cluster 2
        // Create a child for B ---> I
        outpoints = {COutPoint(transactions[1].info.m_tx->GetHash(), 0)};
        create_and_add_tx(transactions, outpoints);

        // Create a child for B ---> J
        outpoints = {COutPoint(transactions[1].info.m_tx->GetHash(), 1)};
        const CTransactionRef tx_J = create_and_add_tx(transactions, outpoints);

        // Create a child for J ---> K
        outpoints = {COutPoint(tx_J->GetHash(), 0)};
        create_and_add_tx(transactions, outpoints);

        const auto txAncestorsAndDescendants = GetTxAncestorsAndDescendants(transactions);
        BOOST_CHECK_EQUAL(txAncestorsAndDescendants.size(), transactions.size());

        // Check tx A topology;
        {
            const Txid txA_Id = transactions[0].info.m_tx->GetHash();
            std::vector<Txid> descendants{txA_Id};
            for (auto i = 2; i <= 7; i++) { // Add  C, D, E, F, G, H
                descendants.emplace_back(transactions[i].info.m_tx->GetHash());
            }
            validateTxRelation(txA_Id, std::vector<Txid>{txA_Id}, descendants, txAncestorsAndDescendants);
        }
        // Check tx C topology;
        {
            const Txid txC_Id = transactions[2].info.m_tx->GetHash();
            std::vector<Txid> ancestors{txC_Id, transactions[0].info.m_tx->GetHash()}; // C, A
            std::vector<Txid> descendants{txC_Id};
            for (auto i = 4; i <= 6; i++) { // Add E, F, G
                descendants.emplace_back(transactions[i].info.m_tx->GetHash());
            }
            validateTxRelation(txC_Id, ancestors, descendants, txAncestorsAndDescendants);
        }
        // Check tx B topology;
        {
            const Txid txB_Id = transactions[1].info.m_tx->GetHash();
            std::vector<Txid> descendants{txB_Id};
            for (auto i = 8; i <= 10; i++) { // Add I, J, K
                descendants.emplace_back(transactions[i].info.m_tx->GetHash());
            }
            validateTxRelation(txB_Id, std::vector<Txid>{txB_Id}, descendants, txAncestorsAndDescendants);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
