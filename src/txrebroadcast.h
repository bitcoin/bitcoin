// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXREBROADCAST_H
#define BITCOIN_TXREBROADCAST_H

#include <policy/feerate.h>
#include <txmempool.h>
#include <validation.h>

/** Frequency to run the fee rate cache. */
constexpr std::chrono::minutes REBROADCAST_CACHE_FREQUENCY{1};

struct TxIds {
    TxIds(uint256 txid, uint256 wtxid) : m_txid(txid), m_wtxid(wtxid) {}

    const uint256 m_txid;
    const uint256 m_wtxid;
};

class indexed_rebroadcast_set;

class TxRebroadcastHandler
{
public:
    TxRebroadcastHandler(const CTxMemPool& mempool, const ChainstateManager& chainman, const CChainParams& chainparams);
    ~TxRebroadcastHandler();

    TxRebroadcastHandler(const TxRebroadcastHandler& other) = delete;
    TxRebroadcastHandler& operator=(const TxRebroadcastHandler& other) = delete;

    /**
     * Identify transaction candidates to be rebroadcast.
     * Calculates the top of the mempool by fee rate, limits the size based on
     * recent block information passed in, rate limits candidates and enforces
     * a maximum number of rebroadcast attempts per transaction.
     *
     * @param[in]  recent_block        Optionally provide a reference to skip a disk read
     * @param[in]  recent_block_index  An index to the recent block, used to
     *                                 calculate weight of rebroadcast candidates
     * @return     std::vector<TxIds>  Returns transaction ids of rebroadcast candidates
     * */
    std::vector<TxIds> GetRebroadcastTransactions(const std::shared_ptr<const CBlock>& recent_block, const CBlockIndex& recent_block_index);

    /** Assemble a block from the highest fee rate packages in the local
     *  mempool. Update the cache with the minimum fee rate for a package to be
     *  included.
     * */
    void CacheMinRebroadcastFee();

private:
    const CTxMemPool& m_mempool;
    const ChainstateManager& m_chainman;
    const CChainParams& m_chainparams;

    /** Protects internal data members */
    Mutex m_rebroadcast_mutex;

    /** Block at time of cache */
    CBlockIndex* m_tip_at_cache_time GUARDED_BY(m_rebroadcast_mutex){nullptr};

    /** Minimum fee rate for package to be included in block */
    CFeeRate m_cached_fee_rate GUARDED_BY(m_rebroadcast_mutex);
    CFeeRate m_previous_cached_fee_rate GUARDED_BY(m_rebroadcast_mutex);

    /** Keep track of previous rebroadcast attempts.
     *
     *  There are circumstances where our mempool might know about transactions
     *  that will never be mined. Two examples:
     *  1. A software upgrade tightens policy, but the node has not been
     *  upgraded and thus is accepting transactions that other nodes on the
     *  network now reject.
     *  2. An attacker targets the network by sending conflicting transactions
     *  to nodes.
     *
     *  Under such circumstances, we want to avoid wasting a significant amount
     *  of network bandwidth. Also we want to let transactions genuinely expire
     *  from the majority of mempools, unless the source wallet decides to
     *  rebroadcast the transaction.
     *
     *  So, we use this tracker to limit the frequency and the maximum number
     *  of times we will attempt to rebroadcast a transaction.
     * */
    std::unique_ptr<indexed_rebroadcast_set> m_attempt_tracker GUARDED_BY(m_rebroadcast_mutex);
};

#endif // BITCOIN_TXREBROADCAST_H
