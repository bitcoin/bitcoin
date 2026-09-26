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

#include <vector>

namespace wallet {

struct TipBlock
{
    uint256 prev_block_hash{};
    int64_t prev_block_time{0};
    int tip_height{0};
};

inline TipBlock GetTip(const CChainParams& params, const node::NodeContext& context)
{
    {
        LOCK(::cs_main);
        if (const auto* tip{context.chainman->ActiveTip()}) {
            return {tip->GetBlockHash(), tip->GetBlockTime(), tip->nHeight};
        }
    }
    return {params.GenesisBlock().GetHash(), params.GenesisBlock().GetBlockTime(), 0};
}

/**
 * Generate a fake block with customizable coinbase outputs and optional extra transactions.
 * This is useful for benchmarks that need to quickly build wallet state without full validation.
 */
inline void GenerateFakeBlock(const CChainParams& params,
                              const node::NodeContext& context,
                              CWallet& wallet,
                              const std::vector<CTxOut>& coinbase_outputs,
                              const std::vector<CTransactionRef>& extra_txs = {})
{
    TipBlock tip{GetTip(params, context)};

    CBlock block;
    CMutableTransaction coinbase_tx;
    coinbase_tx.vin.resize(1);
    coinbase_tx.vin[0].prevout.SetNull();
    coinbase_tx.vin[0].scriptSig = CScript() << ++tip.tip_height << OP_0;
    coinbase_tx.vout = coinbase_outputs;

    block.vtx = {MakeTransactionRef(std::move(coinbase_tx))};
    block.vtx.insert(block.vtx.end(), extra_txs.begin(), extra_txs.end());

    block.nVersion = VERSIONBITS_LAST_OLD_BLOCK_VERSION;
    block.hashPrevBlock = tip.prev_block_hash;
    block.hashMerkleRoot = BlockMerkleRoot(block);
    block.nTime = ++tip.prev_block_time;
    block.nBits = params.GenesisBlock().nBits;
    block.nNonce = 0;

    CBlockIndex* pindex{nullptr};
    {
        LOCK(::cs_main);
        pindex = context.chainman->m_blockman.AddToBlockIndex(block, context.chainman->m_best_header);
        context.chainman->ActiveChain().SetTip(*pindex);
    }

    wallet.blockConnected(kernel::ChainstateRole{}, kernel::MakeBlockInfo(pindex, &block));
}

inline void GenerateFakeBlock(const CChainParams& params,
                              const node::NodeContext& context,
                              CWallet& wallet,
                              const CScript& coinbase_script,
                              int num_coinbase_outputs,
                              CAmount total_coinbase_value = 50 * COIN,
                              const std::vector<CTransactionRef>& extra_txs = {})
{
    std::vector<CTxOut> coinbase_outputs;
    coinbase_outputs.reserve(num_coinbase_outputs);
    const CAmount per_output{total_coinbase_value / num_coinbase_outputs};
    for (int i{0}; i < num_coinbase_outputs; ++i) {
        coinbase_outputs.emplace_back(per_output, coinbase_script);
    }
    GenerateFakeBlock(params, context, wallet, coinbase_outputs, extra_txs);
}

} // namespace wallet

#endif // BITCOIN_BENCH_WALLET_BENCH_UTIL_H
