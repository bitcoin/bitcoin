// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_TXMEMPOOL_H
#define BITCOIN_INTERFACES_TXMEMPOOL_H

#include <primitives/transaction.h> // For CTransactionRef
#include <sync.h>                   // For RecursiveMutex

#include <memory>
#include <vector>

class CTxMemPool;
class CCoinsViewCache;

namespace interfaces {

//! Interface for accessing a txmempool.
class TxMempool
{
public:
    explicit TxMempool(RecursiveMutex* mutex)
        : m_mutex{mutex} {}
    virtual ~TxMempool() {}

    //! A transaction was updated
    virtual void transactionUpdated() = 0;

    //! Run consistency check
    virtual void check(const CCoinsViewCache& coins) = 0;

    //! Remove txs after block has been connected
    virtual void removeForBlock(const std::vector<CTransactionRef>& vtx, unsigned int block_height) EXCLUSIVE_LOCKS_REQUIRED(m_mutex) = 0;

    //! Alias for the txmempool mutex or nullptr if no txmempool was passed in
    RecursiveMutex* const m_mutex;

    //! Return pointer to internal txmempool class
    virtual CTxMemPool* txmempool() { return nullptr; }
};

//! Return implementation of txmempool interface.
std::unique_ptr<TxMempool> MakeTxMempool(CTxMemPool* txmempool);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_TXMEMPOOL_H
