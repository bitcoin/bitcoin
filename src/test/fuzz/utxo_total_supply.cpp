// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <kernel/coinstats.h>
#include <node/miner.h>
#include <script/interpreter.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>
#include <validation.h>

FUZZ_TARGET(utxo_total_supply)
{
    /** The testing setup that creates a chainman only (no chainstate) */
    ChainTestingSetup test_setup{
        ChainType::REGTEST,
        {
            "-testactivationheight=bip34@2",
        },
    };
    // Create chainstate
    test_setup.LoadVerifyActivateChainstate();
    auto& node{test_setup.m_node};
    auto& chainman{*Assert(test_setup.m_node.chainman)};
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const auto ActiveHeight = [&]() {
        LOCK(chainman.GetMutex());
        return chainman.ActiveHeight();
    };
    const auto PrepareNextBlock = [&]() {
        // Use OP_FALSE to avoid BIP30 check from hitting early
        auto block = PrepareBlock(node, CScript{} << OP_FALSE);
        // Replace OP_FALSE with OP_TRUE
        {
            CMutableTransaction tx{*block->vtx.back()};
            tx.vout.at(0).scriptPubKey = CScript{} << OP_TRUE;
            block->vtx.back() = MakeTransactionRef(tx);
        }
        return block;
    };

    /** The block template this fuzzer is working on */
    auto current_block = PrepareNextBlock();
    /** Append-only set of tx outpoints, entries are not removed when spent */
    std::vector<std::pair<COutPoint, CTxOut>> txos;
    /** The utxo stats at the chain tip */
    kernel::CCoinsStats utxo_stats;
    /** The total amount of coins in the utxo set */
    CAmount circulation{0};


    // Store the tx out in the txo map
    const auto StoreLastTxo = [&]() {
        // get last tx
        const CTransaction& tx = *current_block->vtx.back();
        // get last out
        const uint32_t i = tx.vout.size() - 1;
        // store it
        txos.emplace_back(COutPoint{tx.GetHash(), i}, tx.vout.at(i));
        if (current_block->vtx.size() == 1 && tx.vout.at(i).scriptPubKey[0] == OP_RETURN) {
            // also store coinbase
            const uint32_t i = tx.vout.size() - 2;
            txos.emplace_back(COutPoint{tx.GetHash(), i}, tx.vout.at(i));
        }
    };
    const auto AppendRandomTxo = [&](CMutableTransaction& tx) {
        const auto& txo = txos.at(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, txos.size() - 1));
        tx.vin.emplace_back(txo.first);
        tx.vout.emplace_back(txo.second.nValue, txo.second.scriptPubKey); // "Forward" coin with no fee
    };
    const auto UpdateUtxoStats = [&]() {
        LOCK(chainman.GetMutex());
        Assert(chainman.ActiveChainstate().ForceFlushStateToDisk());
        utxo_stats = std::move(
            *Assert(kernel::ComputeUTXOStats(kernel::CoinStatsHashType::NONE, &chainman.ActiveChainstate().CoinsDB(), chainman.m_blockman, {})));
        // Check that miner can't print more money than they are allowed to
        assert(circulation == utxo_stats.total_amount);
    };


    // Update internal state to chain tip
    StoreLastTxo();
    UpdateUtxoStats();
    assert(ActiveHeight() == 0);
    // Get at which height we duplicate the coinbase
    // Assuming that the fuzzer will mine relatively short chains (less than 200 blocks), we want the duplicate coinbase to be not too high.
    // Up to 300 seems reasonable.
    int64_t duplicate_coinbase_height = fuzzed_data_provider.ConsumeIntegralInRange(0, 300);
    // Always pad with OP_0 at the end to avoid bad-cb-length error
    const CScript duplicate_coinbase_script = CScript() << duplicate_coinbase_height << OP_0;
    // Mine the first block with this duplicate
    current_block = PrepareNextBlock();
    StoreLastTxo();

    {
        // Create duplicate (CScript should match exact format as in CreateNewBlock)
        CMutableTransaction tx{*current_block->vtx.front()};
        tx.vin.at(0).scriptSig = duplicate_coinbase_script;

        // Mine block and create next block template
        current_block->vtx.front() = MakeTransactionRef(tx);
    }
    current_block->hashMerkleRoot = BlockMerkleRoot(*current_block);
    assert(!MineBlock(node, current_block).IsNull());
    circulation += GetBlockSubsidy(ActiveHeight(), Params().GetConsensus());

    assert(ActiveHeight() == 1);
    UpdateUtxoStats();
    current_block = PrepareNextBlock();
    StoreLastTxo();

    // Limit to avoid timeout, but enough to cover duplicate_coinbase_height
    // and CVE-2018-17144.
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes(), 2'00)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                // Append an input-output pair to the last tx in the current block
                CMutableTransaction tx{*current_block->vtx.back()};
                AppendRandomTxo(tx);
                current_block->vtx.back() = MakeTransactionRef(tx);
                StoreLastTxo();
            },
            [&] {
                // Append a tx to the list of txs in the current block
                CMutableTransaction tx{};
                AppendRandomTxo(tx);
                current_block->vtx.push_back(MakeTransactionRef(tx));
                StoreLastTxo();
            },
            [&] {
                // Append the current block to the active chain
                node::RegenerateCommitments(*current_block, chainman);
                const bool was_valid = !MineBlock(node, current_block).IsNull();

                const auto prev_utxo_stats = utxo_stats;
                if (was_valid) {
                    if (duplicate_coinbase_height == ActiveHeight()) {
                        // we mined the duplicate coinbase
                        assert(current_block->vtx.at(0)->vin.at(0).scriptSig == duplicate_coinbase_script);
                    }

                    circulation += GetBlockSubsidy(ActiveHeight(), Params().GetConsensus());
                }

                UpdateUtxoStats();

                if (!was_valid) {
                    // utxo stats must not change
                    assert(prev_utxo_stats.hashSerialized == utxo_stats.hashSerialized);
                }

                current_block = PrepareNextBlock();
                StoreLastTxo();
            });
    }
}
