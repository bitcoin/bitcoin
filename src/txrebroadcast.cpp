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

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

/** We rebroadcast upto 3/4 of max block weight to reduce noise due to
 * circumstances such as miners mining priority transactions. */
static constexpr float REBROADCAST_WEIGHT_RATIO{0.75};

/** Default minimum age for a transaction to be rebroadcast */
static constexpr std::chrono::minutes REBROADCAST_MIN_TX_AGE{30min};

/** Maximum number of times we will rebroadcast a transaction */
static constexpr int MAX_REBROADCAST_COUNT{6};

/** Minimum amount of time between returning the same transaction for
 * rebroadcast */
static constexpr std::chrono::hours MIN_REATTEMPT_INTERVAL{4h};

struct RebroadcastEntry {
    RebroadcastEntry(std::chrono::microseconds now_time, uint256 wtxid)
        : m_last_attempt(now_time),
          m_wtxid(wtxid) {}

    std::chrono::microseconds m_last_attempt;
    const uint256 m_wtxid;
    int m_count{1};
};

/** Used for multi_index tag  */
struct index_by_last_attempt {};

class indexed_rebroadcast_set : public
boost::multi_index_container<
    RebroadcastEntry,
    boost::multi_index::indexed_by<
        // sorted by wtxid
        boost::multi_index::hashed_unique<
            boost::multi_index::tag<index_by_wtxid>,
            boost::multi_index::member<RebroadcastEntry, const uint256, &RebroadcastEntry::m_wtxid>,
            SaltedTxidHasher
        >,
        // sorted by last rebroadcast time
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<index_by_last_attempt>,
            boost::multi_index::member<RebroadcastEntry, std::chrono::microseconds, &RebroadcastEntry::m_last_attempt>
        >
    >
>{};

TxRebroadcastHandler::~TxRebroadcastHandler() = default;

TxRebroadcastHandler::TxRebroadcastHandler(const CTxMemPool& mempool, const ChainstateManager& chainman, const CChainParams& chainparams)
    : m_mempool{mempool},
      m_chainman{chainman},
      m_chainparams(chainparams),
      m_attempt_tracker{std::make_unique<indexed_rebroadcast_set>()}{}

std::vector<TxIds> TxRebroadcastHandler::GetRebroadcastTransactions(const std::shared_ptr<const CBlock>& recent_block, const CBlockIndex& recent_block_index)
{
    auto start_time = GetTime<std::chrono::microseconds>();

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
    options.m_skip_inclusion_until = start_time - REBROADCAST_MIN_TX_AGE;
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

    LOCK(m_rebroadcast_mutex);
    for (const CTransactionRef& tx : block_template->block.vtx) {
        if (tx->IsCoinBase()) continue;

        const uint256& txid = tx->GetHash();
        const uint256& wtxid = tx->GetWitnessHash();

        // Check if we have previously rebroadcasted, decide if we will this
        // round, and if so, record the attempt.
        auto entry_it = m_attempt_tracker->find(wtxid);

        if (entry_it == m_attempt_tracker->end()) {
            // No existing entry, we will rebroadcast, so create a new one
            RebroadcastEntry entry(start_time, wtxid);
            m_attempt_tracker->insert(entry);
        } else if (entry_it->m_count >= MAX_REBROADCAST_COUNT) {
            // We have already rebroadcast this transaction the maximum number
            // of times permitted, so skip rebroadcasting.
            continue;
        } else if (entry_it->m_last_attempt > start_time - MIN_REATTEMPT_INTERVAL) {
            // We already rebroadcasted this in the past 4 hours. Even if we
            // added it to the set, it would probably not get INVed to most
            // peers due to filterInventoryKnown.
            continue;
        } else {
            // We have rebroadcasted this transaction before, but will try
            // again now. Record the attempt.
            auto UpdateRebroadcastEntry = [start_time](RebroadcastEntry& rebroadcast_entry) {
                rebroadcast_entry.m_last_attempt = start_time;
                ++rebroadcast_entry.m_count;
            };

            m_attempt_tracker->modify(entry_it, UpdateRebroadcastEntry);
        }

        // Add to set of rebroadcast candidates
        rebroadcast_txs.push_back(TxIds(txid, wtxid));
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
