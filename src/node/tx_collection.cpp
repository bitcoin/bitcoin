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
} // namespace node
