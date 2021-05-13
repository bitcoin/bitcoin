// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXREBROADCAST_H
#define BITCOIN_TXREBROADCAST_H

#include <policy/feerate.h>
#include <txmempool.h>

struct TxIds {
    TxIds(uint256 txid, uint256 wtxid) : m_txid(txid), m_wtxid(wtxid) {}

    const uint256 m_txid;
    const uint256 m_wtxid;
};

class TxRebroadcastHandler
{
public:
    TxRebroadcastHandler(const CTxMemPool& mempool, const ChainstateManager& chainman, const CChainParams& chainparams)
        : m_mempool(mempool),
          m_chainman(chainman),
          m_chainparams(chainparams){};

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

private:
    const CTxMemPool& m_mempool;
    const ChainstateManager& m_chainman;
    const CChainParams& m_chainparams;
};

#endif // BITCOIN_TXREBROADCAST_H
