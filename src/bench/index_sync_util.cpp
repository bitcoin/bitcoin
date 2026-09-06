// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/index_sync_util.h>

#include <addresstype.h>
#include <chain.h>
#include <consensus/amount.h>
#include <key.h>
#include <node/blockstorage.h>
#include <policy/feerate.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <validation.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

int ExtendChainWithSpends(TestChain100Setup& test_setup, uint32_t num_blocks, uint32_t num_txs_per_block)
{
    Assert(num_txs_per_block > 0);

    // Block filter elements are deduplicated, so only distinct scripts add to a
    // filter. Keys are reused across blocks, distinct within one.
    const size_t num_keys{2 * num_txs_per_block};
    std::vector<CKey> keys;
    std::vector<CTxOut> outs;
    keys.reserve(num_keys);
    outs.reserve(num_keys);
    for (size_t i{0}; i < num_keys; ++i) {
        keys.push_back(GenerateRandomKey());
        outs.emplace_back(COIN, GetScriptForDestination(WitnessV0KeyHash{keys.back().GetPubKey()}));
    }

    const CScript coinbase_spk{GetScriptForDestination(WitnessV0KeyHash{test_setup.coinbaseKey.GetPubKey()})};
    const int start_height{WITH_LOCK(::cs_main, return test_setup.m_node.chainman->ActiveHeight())};

    // The coinbase of block 1 matures exactly at the first height added here.
    auto& coinbase_to_spend{test_setup.m_coinbase_txns[0]};
    size_t k0{0};
    size_t k1{1};
    const CMutableTransaction first_tx{
        test_setup.CreateValidTransaction(
            /*input_transactions=*/{coinbase_to_spend},
            /*inputs=*/{COutPoint(coinbase_to_spend->GetHash(), 0)},
            /*input_height=*/1,
            /*input_signing_keys=*/{test_setup.coinbaseKey},
            /*outputs=*/{outs[k0], outs[k1]},
            /*feerate=*/{},
            /*fee_output=*/{})
            .first};
    test_setup.CreateAndProcessBlock({first_tx}, coinbase_spk);
    test_setup.m_clock += 1s;

    // first_tx went into start_height + 1, so the loop starts one above that.
    int next_height{start_height + 2};
    CTransactionRef tx_to_spend{MakeTransactionRef(first_tx)};
    size_t next_key{2 % num_keys};
    for (uint32_t b{0}; b < num_blocks; ++b) {
        std::vector<CMutableTransaction> txs;
        txs.reserve(num_txs_per_block);
        for (uint32_t i{0}; i < num_txs_per_block; ++i) {
            const std::vector<COutPoint> inputs{
                COutPoint(tx_to_spend->GetHash(), 0),
                COutPoint(tx_to_spend->GetHash(), 1),
            };
            const std::vector<CKey> signing_keys{keys[k0], keys[k1]};
            // The first transaction of a block spends the last one of the
            // previous block; the rest spend within this one.
            const int input_height{i == 0 ? next_height - 1 : next_height};
            k0 = next_key;
            next_key = (next_key + 1) % num_keys;
            k1 = next_key;
            next_key = (next_key + 1) % num_keys;
            const CMutableTransaction tx{
                test_setup.CreateValidTransaction(
                    /*input_transactions=*/{tx_to_spend},
                    /*inputs=*/inputs,
                    /*input_height=*/input_height,
                    /*input_signing_keys=*/signing_keys,
                    /*outputs=*/{outs[k0], outs[k1]},
                    /*feerate=*/{},
                    /*fee_output=*/{})
                    .first};
            txs.emplace_back(tx);
            tx_to_spend = MakeTransactionRef(tx);
        }
        test_setup.CreateAndProcessBlock(txs, coinbase_spk);
        // UpdateTime() clamps a block's time to at least MTP+1, so a frozen
        // clock drifts a second per block and eventually trips
        // MAX_FUTURE_BLOCK_TIME.
        test_setup.m_clock += 1s;
        ++next_height;
    }

    // CreateAndProcessBlock ignores the result of ProcessNewBlock.
    Assert(WITH_LOCK(::cs_main, return test_setup.m_node.chainman->ActiveHeight()) == next_height - 1);

    // BaseIndex::Commit() writes nothing until the index tip is an ancestor of
    // the last flushed block, and a test setup never flushes on its own.
    test_setup.m_node.chainman->ActiveChainstate().ForceFlushStateToDisk();

    return start_height + 1;
}

std::vector<Txid> CollectChainTxids(TestChain100Setup& test_setup, int from_height)
{
    Assert(from_height >= 0);
    std::vector<Txid> txids;
    LOCK(::cs_main);
    const CChain& chain{test_setup.m_node.chainman->ActiveChain()};
    for (int h{from_height}; h <= chain.Height(); ++h) {
        CBlock block;
        Assert(test_setup.m_node.chainman->m_blockman.ReadBlock(block, *chain[h]));
        for (const auto& tx : block.vtx) txids.push_back(tx->GetHash());
    }
    return txids;
}

std::vector<COutPoint> CollectChainSpentOutpoints(TestChain100Setup& test_setup, int from_height)
{
    Assert(from_height >= 0);
    std::vector<COutPoint> outpoints;
    LOCK(::cs_main);
    const CChain& chain{test_setup.m_node.chainman->ActiveChain()};
    for (int h{from_height}; h <= chain.Height(); ++h) {
        CBlock block;
        Assert(test_setup.m_node.chainman->m_blockman.ReadBlock(block, *chain[h]));
        for (const auto& tx : block.vtx) {
            if (tx->IsCoinBase()) continue;
            for (const auto& in : tx->vin) outpoints.push_back(in.prevout);
        }
    }
    return outpoints;
}
