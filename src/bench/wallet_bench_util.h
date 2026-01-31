// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BENCH_WALLET_BENCH_UTIL_H
#define BITCOIN_BENCH_WALLET_BENCH_UTIL_H

#include <consensus/merkle.h>
#include <kernel/chain.h>
#include <kernel/chainparams.h>
#include <kernel/types.h>
#include <node/blockstorage.h>
#include <node/context.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <uint256.h>
#include <validation.h>
#include <versionbits.h>
#include <wallet/wallet.h>

namespace wallet {

struct TipBlock
{
    uint256 prev_block_hash;
    int64_t prev_block_time;
    int tip_height;
};

inline TipBlock GetTip(const CChainParams& params, const node::NodeContext& context)
{
    auto tip = WITH_LOCK(::cs_main, return context.chainman->ActiveTip());
    return (tip) ? TipBlock{tip->GetBlockHash(), tip->GetBlockTime(), tip->nHeight} :
           TipBlock{params.GenesisBlock().GetHash(), params.GenesisBlock().GetBlockTime(), 0};
}

/**
 * Generate a fake block with customizable coinbase outputs and optional extra transactions.
 * This is useful for benchmarks that need to quickly build wallet state without full validation.
 */
inline void GenerateFakeBlock(const CChainParams& params,
                              const node::NodeContext& context,
                              CWallet& wallet,
                              const CScript& coinbase_script,
                              int num_coinbase_outputs,
                              CAmount total_coinbase_value = 50 * COIN,
                              const std::vector<CTransactionRef>& extra_txs = {})
{
    TipBlock tip{GetTip(params, context)};

    CBlock block;
    CMutableTransaction coinbase_tx;
    coinbase_tx.vin.resize(1);
    coinbase_tx.vin[0].prevout.SetNull();
    coinbase_tx.vin[0].scriptSig = CScript() << ++tip.tip_height << OP_0;

    // Create coinbase outputs
    CAmount per_output = total_coinbase_value / num_coinbase_outputs;
    for (int i = 0; i < num_coinbase_outputs; ++i) {
        coinbase_tx.vout.emplace_back(per_output, coinbase_script);
    }

    block.vtx = {MakeTransactionRef(std::move(coinbase_tx))};

    // Add any extra transactions
    for (const auto& tx : extra_txs) {
        block.vtx.push_back(tx);
    }

    block.nVersion = VERSIONBITS_LAST_OLD_BLOCK_VERSION;
    block.hashPrevBlock = tip.prev_block_hash;
    block.hashMerkleRoot = BlockMerkleRoot(block);
    block.nTime = ++tip.prev_block_time;
    block.nBits = params.GenesisBlock().nBits;
    block.nNonce = 0;

    {
        LOCK(::cs_main);
        CBlockIndex* pindex{context.chainman->m_blockman.AddToBlockIndex(block, context.chainman->m_best_header)};
        context.chainman->ActiveChain().SetTip(*pindex);
    }

    const auto* pindex = WITH_LOCK(::cs_main, return context.chainman->ActiveChain().Tip());
    wallet.blockConnected(kernel::ChainstateRole{}, kernel::MakeBlockInfo(pindex, &block));
}

} // namespace wallet

#endif // BITCOIN_BENCH_WALLET_BENCH_UTIL_H
