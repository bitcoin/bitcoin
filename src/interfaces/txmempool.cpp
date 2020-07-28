// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/txmempool.h>

#include <primitives/transaction.h>
#include <sync.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/memory.h>

#include <memory>
#include <vector>

namespace interfaces {
namespace {

class TxMempoolImpl final : public TxMempool
{
public:
    explicit TxMempoolImpl(CTxMemPool& txmempool)
        : TxMempool{&txmempool.cs},
          m_txmempool(txmempool) {}

    void transactionUpdated() override
    {
        m_txmempool.AddTransactionsUpdated(1);
    }

    void check(const CCoinsViewCache& coins) override
    {
        m_txmempool.check(&coins);
    }

    void removeForBlock(const std::vector<CTransactionRef>& vtx, unsigned int block_height) override
    {
        AssertLockHeld(m_txmempool.cs);
        m_txmempool.removeForBlock(vtx, block_height);
    }

    CTxMemPool* txmempool() override { return &m_txmempool; }

    CTxMemPool& m_txmempool;
};

class NoTxMempool final : public TxMempool
{
public:
    explicit NoTxMempool()
        : TxMempool{nullptr} {}

    void transactionUpdated() override { invalidCall(); }
    void check(const CCoinsViewCache& coins) override { invalidCall(); }
    void removeForBlock(const std::vector<CTransactionRef>& vtx, unsigned int block_height) override { invalidCall(); }

private:
    void invalidCall() { Assert(false); }
};

} // namespace

std::unique_ptr<TxMempool> MakeTxMempool(CTxMemPool* txpool)
{
    if (txpool) {
        return MakeUnique<TxMempoolImpl>(*txpool);
    } else {
        return MakeUnique<NoTxMempool>();
    }
}

} // namespace interfaces
