// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <node/tx_collection.h>

#include <consensus/consensus.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <tinyformat.h>
#include <txmempool.h>
#include <util/hasher.h>

#include <boost/multi_index/detail/hash_index_iterator.hpp>

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>

namespace node {

TxCollection::TxCollection(std::vector<Wtxid> wtxids, CTxMemPool& mempool)
    : m_wtxids(std::move(wtxids)),
      m_mempool(mempool)
{
    // Check for excessively high numbers of transactions.
    if (m_wtxids.size() > MAX_BLOCK_WEIGHT / MIN_TRANSACTION_WEIGHT) {
        throw std::runtime_error(strprintf("too many wtxids (%d > %d)", m_wtxids.size(), MAX_BLOCK_WEIGHT / MIN_TRANSACTION_WEIGHT));
    }
    m_transactions.reserve(m_wtxids.size());
    for (const auto& wtxid : m_wtxids) {
        if (!m_transactions.emplace(wtxid, nullptr).second) {
            throw std::runtime_error(strprintf("duplicate wtxid %s", wtxid.ToString()));
        }
    }
    LOCK(m_mempool.cs);
    for (auto& [wtxid, tx] : m_transactions) {
        if (const auto it{m_mempool.GetIter(wtxid)}) {
            tx = (*it)->GetSharedTx();
        }
    }
}

std::vector<uint32_t> TxCollection::UnknownTxPos() const
{
    std::vector<uint32_t> result;
    for (size_t i{0}; i < m_wtxids.size(); ++i) {
        // Every requested wtxid is a key (added in the constructor), so at()
        // is safe; a null value means the transaction is still missing.
        if (!m_transactions.at(m_wtxids[i])) result.push_back(static_cast<uint32_t>(i));
    }
    return result;
}
} // namespace node
