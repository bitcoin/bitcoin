// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/txmempool.h>

#include <chainparams.h>
#include <node/context.h>
#include <node/mempool_args.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/time.h>
#include <util/translation.h>

using node::NodeContext;

CTxMemPool::Options MemPoolOptionsForTest(const NodeContext& node)
{
    CTxMemPool::Options mempool_opts{
        .estimator = node.fee_estimator.get(),
        // Default to always checking mempool regardless of
        // chainparams.DefaultConsistencyChecks for tests
        .check_ratio = 1,
    };
    const auto err{ApplyArgsManOptions(*node.args, ::Params(), mempool_opts)};
    Assert(!err);
    return mempool_opts;
}

CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CMutableTransaction& tx) const
{
    return FromTx(MakeTransactionRef(tx));
}

CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CTransactionRef& tx) const
{
    return CTxMemPoolEntry{tx, nFee, TicksSinceEpoch<std::chrono::seconds>(time), nHeight, spendsCoinbase, sigOpCost, lp};
}
