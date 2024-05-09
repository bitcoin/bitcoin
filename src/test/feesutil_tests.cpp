// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <kernel/mempool_entry.h>
#include <policy/fees_util.h>
#include <test/util/random.h>
#include <test/util/txmempool.h>
#include <txmempool.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(feesutil_tests, ChainTestingSetup)

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

BOOST_AUTO_TEST_CASE(ComputingTxAncestorsAndDescendants)
{
    TestMemPoolEntryHelper entry;
    // Test 20 unique transactions
    {
        std::vector<RemovedMempoolTransactionInfo> transactions;
        transactions.reserve(20);

        for (auto i = 0; i < 20; ++i) {
            const std::vector<COutPoint> outpoints{COutPoint(Txid::FromUint256(InsecureRand256()), 0)};
            const CTransactionRef tx = make_tx(outpoints);
            transactions.emplace_back(entry.FromTx(tx));
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
            const std::vector<COutPoint> outpoints{COutPoint(Txid::FromUint256(InsecureRand256()), 0)};
            const CTransactionRef tx = make_tx(outpoints);
            transactions.emplace_back(entry.FromTx(tx));
        }

        // Create cluster A children ---> E->F->G
        std::vector<COutPoint> outpoints{COutPoint(transactions[0].info.m_tx->GetHash(), 0)};
        for (auto i = 0; i < 3; ++i) {
            const CTransactionRef tx = make_tx(outpoints);
            transactions.emplace_back(entry.FromTx(tx));
            outpoints = {COutPoint(tx->GetHash(), 0)};
        }

        // Create cluster B children ---> H->I
        outpoints = {COutPoint(transactions[1].info.m_tx->GetHash(), 0)};
        for (size_t i = 0; i < 2; ++i) {
            const CTransactionRef tx = make_tx(outpoints);
            transactions.emplace_back(entry.FromTx(tx));
            outpoints = {COutPoint(tx->GetHash(), 0)};
        }

        // Create cluster C child ---> J
        outpoints = {COutPoint(transactions[2].info.m_tx->GetHash(), 0)};
        const CTransactionRef txJ = make_tx(outpoints);
        transactions.emplace_back(entry.FromTx(txJ));

        // Create cluster B child ---> K
        outpoints = {COutPoint(transactions[3].info.m_tx->GetHash(), 0)};
        const CTransactionRef txK = make_tx(outpoints);
        transactions.emplace_back(entry.FromTx(txK));
        const auto txAncestorsAndDescendants = GetTxAncestorsAndDescendants(transactions);
        BOOST_CHECK_EQUAL(txAncestorsAndDescendants.size(), transactions.size());
        // Check tx A topology;
        {
            const Txid txA_Id = transactions[0].info.m_tx->GetHash();
            const auto txA_Iter = txAncestorsAndDescendants.find(txA_Id);
            BOOST_CHECK(txA_Iter != txAncestorsAndDescendants.end());
            const auto ancestors = std::get<0>(txA_Iter->second);
            BOOST_CHECK(ancestors.size() == 1); // A
            BOOST_CHECK(ancestors.find(txA_Id) != ancestors.end());
            const auto descendants = std::get<1>(txA_Iter->second);
            BOOST_CHECK(descendants.size() == 4); // A, E, F, G
            BOOST_CHECK(descendants.find(txA_Id) != descendants.end());
            for (auto i = 4; i <= 6; i++) {
                auto curr_tx = transactions[i];
                BOOST_CHECK(descendants.find(curr_tx.info.m_tx->GetHash()) != descendants.end());
            }
        }
        // Check tx G topology;
        {
            const Txid txG_Id = transactions[6].info.m_tx->GetHash();
            const auto txG_Iter = txAncestorsAndDescendants.find(txG_Id);
            BOOST_CHECK(txG_Iter != txAncestorsAndDescendants.end());
            const auto ancestors = std::get<0>(txG_Iter->second);
            BOOST_CHECK(ancestors.size() == 4); // G, A, E, F
            BOOST_CHECK(ancestors.find(txG_Id) != ancestors.end());
            auto txA_Id = transactions[0].info.m_tx->GetHash();
            BOOST_CHECK(ancestors.find(txA_Id) != ancestors.end());
            for (auto i = 4; i <= 6; i++) {
                auto curr_tx = transactions[i];
                BOOST_CHECK(ancestors.find(curr_tx.info.m_tx->GetHash()) != ancestors.end());
            }
            const auto descendants = std::get<1>(txG_Iter->second);
            BOOST_CHECK(descendants.size() == 1); // G
            BOOST_CHECK(descendants.find(txG_Id) != descendants.end());
        }
        // Check tx B topology;
        {
            const Txid txB_Id = transactions[1].info.m_tx->GetHash();
            const auto txB_Iter = txAncestorsAndDescendants.find(txB_Id);
            BOOST_CHECK(txB_Iter != txAncestorsAndDescendants.end());

            const auto ancestors = std::get<0>(txB_Iter->second);
            BOOST_CHECK(ancestors.size() == 1); // B
            BOOST_CHECK(ancestors.find(txB_Id) != ancestors.end());

            const auto descendants = std::get<1>(txB_Iter->second);
            BOOST_CHECK(descendants.size() == 3); // B, H, I
            BOOST_CHECK(descendants.find(txB_Id) != descendants.end());
            for (auto i = 7; i <= 8; i++) {
                auto curr_tx = transactions[i];
                BOOST_CHECK(descendants.find(curr_tx.info.m_tx->GetHash()) != descendants.end());
            }
        }
        // Check tx H topology;
        {
            const Txid txH_Id = transactions[7].info.m_tx->GetHash();
            const auto txH_Iter = txAncestorsAndDescendants.find(txH_Id);
            BOOST_CHECK(txH_Iter != txAncestorsAndDescendants.end());
            const auto ancestors = std::get<0>(txH_Iter->second);
            BOOST_CHECK(ancestors.size() == 2); // H, B
            BOOST_CHECK(ancestors.find(txH_Id) != ancestors.end());
            BOOST_CHECK(ancestors.find(transactions[1].info.m_tx->GetHash()) != ancestors.end());
            const auto descendants = std::get<1>(txH_Iter->second);
            BOOST_CHECK(descendants.size() == 2); // H, I
            BOOST_CHECK(descendants.find(txH_Id) != descendants.end());
            BOOST_CHECK(descendants.find(transactions[8].info.m_tx->GetHash()) != descendants.end());
        }
        // Check tx C topology;
        {
            const Txid txC_Id = transactions[2].info.m_tx->GetHash();
            const auto txC_Iter = txAncestorsAndDescendants.find(txC_Id);
            BOOST_CHECK(txC_Iter != txAncestorsAndDescendants.end());

            const auto ancestors = std::get<0>(txC_Iter->second);
            BOOST_CHECK(ancestors.size() == 1); // C
            BOOST_CHECK(ancestors.find(txC_Id) != ancestors.end());

            const auto descendants = std::get<1>(txC_Iter->second);
            BOOST_CHECK(descendants.size() == 2); // C, J
            BOOST_CHECK(descendants.find(txC_Id) != descendants.end());
            BOOST_CHECK(descendants.find(transactions[9].info.m_tx->GetHash()) != descendants.end());
        }
        // Check tx D topology;
        {
            const Txid txD_Id = transactions[3].info.m_tx->GetHash();
            const auto txD_Iter = txAncestorsAndDescendants.find(txD_Id);
            BOOST_CHECK(txD_Iter != txAncestorsAndDescendants.end());
            const auto ancestors = std::get<0>(txD_Iter->second);
            BOOST_CHECK(ancestors.size() == 1); // D
            BOOST_CHECK(ancestors.find(txD_Id) != ancestors.end());

            const auto descendants = std::get<1>(txD_Iter->second);
            BOOST_CHECK(descendants.size() == 2); // D, K
            BOOST_CHECK(ancestors.find(txD_Id) != ancestors.end());
            BOOST_CHECK(descendants.find(transactions[10].info.m_tx->GetHash()) != descendants.end());
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
        // Create transaction A B
        for (auto i = 0; i < 2; ++i) {
            const std::vector<COutPoint> outpoints{COutPoint(Txid::FromUint256(InsecureRand256()), 0)};
            const CTransactionRef tx = make_tx(outpoints, /*num_outputs=*/2);
            transactions.emplace_back(entry.FromTx(tx));
        }
        // Cluster 1 Topology
        // Create a child for A ---> C
        std::vector<COutPoint> outpoints{COutPoint(transactions[0].info.m_tx->GetHash(), 0)};
        const CTransactionRef tx_C = make_tx(outpoints, /*num_outputs=*/2);
        transactions.emplace_back(entry.FromTx(tx_C));

        // Create a child for A ---> D
        outpoints = {COutPoint(transactions[0].info.m_tx->GetHash(), 1)};
        const CTransactionRef tx_D = make_tx(outpoints);
        transactions.emplace_back(entry.FromTx(tx_D));

        // Create a child for C ---> E
        outpoints = {COutPoint(tx_C->GetHash(), 0)};
        const CTransactionRef tx_E = make_tx(outpoints);
        transactions.emplace_back(entry.FromTx(tx_E));

        // Create a child for C ---> F
        outpoints = {COutPoint(tx_C->GetHash(), 1)};
        const CTransactionRef tx_F = make_tx(outpoints);
        transactions.emplace_back(entry.FromTx(tx_F));

        // Create a child for E and F  ---> G
        outpoints = {COutPoint(tx_E->GetHash(), 0), COutPoint(tx_F->GetHash(), 0)};
        transactions.emplace_back(entry.FromTx(make_tx(outpoints)));

        // Create a child for D ---> H
        outpoints = {COutPoint(tx_D->GetHash(), 0)};
        transactions.emplace_back(entry.FromTx(make_tx(outpoints)));


        // Cluster 2
        // Create a child for B ---> I
        outpoints = {COutPoint(transactions[1].info.m_tx->GetHash(), 0)};
        transactions.emplace_back(entry.FromTx(make_tx(outpoints)));

        // Create a child for B ---> J
        outpoints = {COutPoint(transactions[1].info.m_tx->GetHash(), 1)};
        const CTransactionRef tx_J = make_tx(outpoints);
        transactions.emplace_back(entry.FromTx(tx_J));

        // Create a child for J ---> K
        outpoints = {COutPoint(tx_J->GetHash(), 0)};
        transactions.emplace_back(entry.FromTx(make_tx(outpoints)));
        const auto txAncestorsAndDescendants = GetTxAncestorsAndDescendants(transactions);
        BOOST_CHECK_EQUAL(txAncestorsAndDescendants.size(), transactions.size());
        // Check tx A topology;
        {
            const Txid txA_Id = transactions[0].info.m_tx->GetHash();
            const auto txA_Iter = txAncestorsAndDescendants.find(txA_Id);
            BOOST_CHECK(txA_Iter != txAncestorsAndDescendants.end());

            const auto ancestors = std::get<0>(txA_Iter->second);
            const auto descendants = std::get<1>(txA_Iter->second);
            BOOST_CHECK(ancestors.size() == 1); // A
            BOOST_CHECK(ancestors.find(txA_Id) != ancestors.end());

            BOOST_CHECK(descendants.size() == 7); // A, C, D, E, F, G, H
            BOOST_CHECK(descendants.find(txA_Id) != descendants.end());
            for (auto i = 2; i <= 7; i++) {
                BOOST_CHECK(descendants.find(transactions[i].info.m_tx->GetHash()) != descendants.end());
            }
        }
        // Check tx C topology;
        {
            const Txid txC_Id = transactions[2].info.m_tx->GetHash();
            const auto txC_Iter = txAncestorsAndDescendants.find(txC_Id);
            BOOST_CHECK(txC_Iter != txAncestorsAndDescendants.end());

            const auto ancestors = std::get<0>(txC_Iter->second);
            const auto descendants = std::get<1>(txC_Iter->second);
            BOOST_CHECK(ancestors.size() == 2); // C, A
            BOOST_CHECK(ancestors.find(txC_Id) != ancestors.end());
            BOOST_CHECK(ancestors.find(transactions[0].info.m_tx->GetHash()) != ancestors.end());

            BOOST_CHECK(descendants.size() == 4); // C, E, F, G
            BOOST_CHECK(descendants.find(txC_Id) != descendants.end());
            for (auto i = 4; i <= 6; i++) {
                BOOST_CHECK(descendants.find(transactions[i].info.m_tx->GetHash()) != descendants.end());
            }
        }
        // Check tx B topology;
        {
            const Txid txB_Id = transactions[1].info.m_tx->GetHash();
            const auto txB_Iter = txAncestorsAndDescendants.find(txB_Id);
            BOOST_CHECK(txB_Iter != txAncestorsAndDescendants.end());

            const auto ancestors = std::get<0>(txB_Iter->second);
            BOOST_CHECK(ancestors.size() == 1); // B
            BOOST_CHECK(ancestors.find(txB_Id) != ancestors.end());

            const auto descendants = std::get<1>(txB_Iter->second);
            BOOST_CHECK(descendants.size() == 4); // B, I, J, K
            BOOST_CHECK(descendants.find(txB_Id) != descendants.end());

            for (auto i = 8; i <= 10; i++) {
                BOOST_CHECK(descendants.find(transactions[i].info.m_tx->GetHash()) != descendants.end());
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
