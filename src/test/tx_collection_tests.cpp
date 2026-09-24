// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <arith_uint256.h>
#include <consensus/consensus.h>
#include <node/block_template_manager.h>
#include <node/tx_collection.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <tinyformat.h>
#include <txmempool.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(tx_collection_tests, RegTestingSetup)

BOOST_AUTO_TEST_CASE(collection_limits)
{
    auto& manager{*m_node.block_template_manager};
    BOOST_CHECK(manager.CreateTxCollection({}));
    const auto wtxid{Wtxid::FromUint256(uint256{1})};
    BOOST_CHECK_EXCEPTION(manager.CreateTxCollection({wtxid, wtxid}), std::runtime_error,
                          HasReason{"duplicate wtxid " + wtxid.ToString()});

    std::vector<Wtxid> wtxids;
    const auto max_txs{MAX_BLOCK_WEIGHT / MIN_TRANSACTION_WEIGHT};
    for (uint32_t i{0}; i < max_txs; ++i) {
        wtxids.push_back(Wtxid::FromUint256(ArithToUint256(arith_uint256{i})));
    }
    // The maximum succeeds; one more request fails before any mempool lookup.
    BOOST_CHECK(manager.CreateTxCollection(wtxids));
    wtxids.push_back(Wtxid::FromUint256(ArithToUint256(arith_uint256{MAX_BLOCK_WEIGHT})));
    BOOST_CHECK_EXCEPTION(manager.CreateTxCollection(wtxids), std::runtime_error,
                          HasReason{strprintf("too many wtxids (%d > %d)", wtxids.size(), max_txs)});
}

BOOST_AUTO_TEST_CASE(unknown_tx_positions_ignore_mempool_changes)
{
    // The collection retains transactions found in the mempool at creation.
    // Missing entries are filled through addMissingTxs(); the collection is
    // independent of later mempool changes.
    CMutableTransaction tx;
    tx.vin.emplace_back(Txid::FromUint256(uint256{1}), 0);
    tx.vout.emplace_back(1, CScript() << OP_TRUE);
    const auto witness_a{MakeTransactionRef(tx)};
    tx.vin[0].scriptWitness.stack = {{1}};
    const auto witness_b{MakeTransactionRef(tx)};
    // Same txid, different wtxid: only the exact witness variant is present.
    BOOST_CHECK(witness_a->GetHash() == witness_b->GetHash());
    BOOST_CHECK(witness_a->GetWitnessHash() != witness_b->GetWitnessHash());

    auto& pool{*m_node.mempool};
    auto& manager{*m_node.block_template_manager};
    TryAddToMempool(pool, TestMemPoolEntryHelper{}.FromTx(witness_b));
    BOOST_REQUIRE(pool.exists(witness_b->GetWitnessHash()));
    auto collection{manager.CreateTxCollection({witness_a->GetWitnessHash(), witness_b->GetWitnessHash()})};
    BOOST_CHECK(collection->UnknownTxPos() == std::vector<uint32_t>{0});

    // The collection retains witness_b after it leaves the mempool.
    WITH_LOCK(pool.cs, pool.removeRecursive(*witness_b, MemPoolRemovalReason::EXPIRY));
    BOOST_REQUIRE(!pool.exists(witness_b->GetWitnessHash()));
    BOOST_CHECK(collection->UnknownTxPos() == std::vector<uint32_t>{0});

    // Missing entries are only filled through addMissingTxs(), not automatically
    // when transactions enter the mempool. This behavior is not essential.
    TryAddToMempool(pool, TestMemPoolEntryHelper{}.FromTx(witness_a));
    BOOST_REQUIRE(pool.exists(witness_a->GetWitnessHash()));
    BOOST_CHECK(collection->UnknownTxPos() == std::vector<uint32_t>{0});
    BOOST_CHECK(manager.CreateTxCollection({})->UnknownTxPos().empty());
}

BOOST_AUTO_TEST_SUITE_END()
