// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/consensus.h>
#include <miner.h>
#include <node/blockstorage.h>
#include <script/script.h>
#include <txrebroadcast.h>
#include <util/time.h>

/** We rebroadcast upto 3/4 of max block weight to reduce noise due to
 * circumstances such as miners mining priority transactions. */
static constexpr float REBROADCAST_WEIGHT_RATIO{0.75};

/** Default minimum age for a transaction to be rebroadcast */
static constexpr std::chrono::minutes REBROADCAST_MIN_TX_AGE{30min};

std::vector<TxIds> TxRebroadcastHandler::GetRebroadcastTransactions(const std::shared_ptr<const CBlock>& recent_block, const CBlockIndex& recent_block_index)
{

    // Calculate how many transactions to rebroadcast based on the size of the
    // incoming block.
    float rebroadcast_block_weight = REBROADCAST_WEIGHT_RATIO * MAX_BLOCK_WEIGHT;
    if (recent_block) {
        // If the passed in block is populated, use to avoid a disk read.
        rebroadcast_block_weight = REBROADCAST_WEIGHT_RATIO * GetBlockWeight(*recent_block.get());
    } else {
        // Otherwise, use the block index to retrieve the relevant block.
        const Consensus::Params& consensus_params = m_chainparams.GetConsensus();
        CBlock block;

        if (ReadBlockFromDisk(block, &recent_block_index, consensus_params)) {
            rebroadcast_block_weight = REBROADCAST_WEIGHT_RATIO * GetBlockWeight(block);
        }
    }

    BlockAssembler::Options options;
    options.nBlockMaxWeight = rebroadcast_block_weight;
    options.m_skip_inclusion_until = GetTime<std::chrono::microseconds>() - REBROADCAST_MIN_TX_AGE;
    options.m_check_block_validity = false;

    // The fee rate condition only filters out transactions if it runs before
    // we process the recently mined block. If the cache has since been
    // updated, used the value from the previous run to filter transactions.
    // Increment the cached value by 1 satoshi / 1000 bytes to avoid
    // rebroadcasting a long-tail of transactions at a fee rate boundary.
    {
        const CBlockIndex* tip = m_chainman.ActiveTip();
        LOCK(m_rebroadcast_mutex);
        if (m_tip_at_cache_time == tip) {
            m_previous_cached_fee_rate += CFeeRate(1);
            options.blockMinFeeRate = m_previous_cached_fee_rate;
        } else {
            m_cached_fee_rate += CFeeRate(1);
            options.blockMinFeeRate = m_cached_fee_rate;
        }
    }

    // Skip if the fee rate cache has not yet run, which could happen once on
    // startup
    std::vector<TxIds> rebroadcast_txs;
    if (options.blockMinFeeRate.GetFeePerK() == CAmount(0)) return rebroadcast_txs;

    // Use CreateNewBlock to identify rebroadcast candidates
    auto block_template = BlockAssembler(m_chainman.ActiveChainstate(), m_mempool, m_chainparams, options)
                              .CreateNewBlock(CScript());
    rebroadcast_txs.reserve(block_template->block.vtx.size());

    for (const CTransactionRef& tx : block_template->block.vtx) {
        if (tx->IsCoinBase()) continue;

        rebroadcast_txs.push_back(TxIds(tx->GetHash(), tx->GetWitnessHash()));
    }

    return rebroadcast_txs;
};

void TxRebroadcastHandler::CacheMinRebroadcastFee()
{
    CChainState& chain_state = m_chainman.ActiveChainstate();
    if (chain_state.IsInitialBlockDownload() || !m_mempool.IsLoaded()) return;

    // Calculate a new fee rate
    BlockAssembler::Options options;
    options.nBlockMaxWeight = REBROADCAST_WEIGHT_RATIO * MAX_BLOCK_WEIGHT;

    CBlockIndex* tip;
    CFeeRate current_fee_rate;
    {
        LOCK(cs_main);
        tip = m_chainman.ActiveTip();
        current_fee_rate = BlockAssembler(chain_state, m_mempool, m_chainparams, options).MinTxFeeRate();
    }

    // Update stored information
    LOCK(m_rebroadcast_mutex);
    m_tip_at_cache_time = tip;
    m_previous_cached_fee_rate = m_cached_fee_rate;
    m_cached_fee_rate = current_fee_rate;
};
