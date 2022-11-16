// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txmempool_entry.h>

#include <consensus/amount.h>
#include <consensus/validation.h>
#include <core_memusage.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <primitives/transaction.h>
#include <util/overflow.h>

CTxMemPoolEntry::CTxMemPoolEntry(const CTransactionRef& tx, CAmount fee,
                                 int64_t time, unsigned int entry_height,
                                 bool spends_coinbase, int64_t sigops_cost, LockPoints lp)
    : tx{tx},
      nFee{fee},
      nTxWeight(GetTransactionWeight(*tx)),
      nUsageSize{RecursiveDynamicUsage(tx)},
      nTime{time},
      entryHeight{entry_height},
      spendsCoinbase{spends_coinbase},
      sigOpCost{sigops_cost},
      m_modified_fee{nFee},
      lockPoints{lp},
      nSizeWithDescendants{GetTxSize()},
      nModFeesWithDescendants{nFee},
      nSizeWithAncestors{GetTxSize()},
      nModFeesWithAncestors{nFee},
      nSigOpCostWithAncestors{sigOpCost} {}

void CTxMemPoolEntry::UpdateModifiedFee(CAmount fee_diff)
{
    nModFeesWithDescendants = SaturatingAdd(nModFeesWithDescendants, fee_diff);
    nModFeesWithAncestors = SaturatingAdd(nModFeesWithAncestors, fee_diff);
    m_modified_fee = SaturatingAdd(m_modified_fee, fee_diff);
}

void CTxMemPoolEntry::UpdateLockPoints(const LockPoints& lp)
{
    lockPoints = lp;
}

size_t CTxMemPoolEntry::GetTxSize() const
{
    return GetVirtualTransactionSize(nTxWeight, sigOpCost, ::nBytesPerSigOp);
}
