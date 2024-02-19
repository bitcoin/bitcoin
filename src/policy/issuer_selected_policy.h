// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_V3PLUS_POLICY_H
#define BITCOIN_POLICY_V3PLUS_POLICY_H

#include <consensus/amount.h>
#include <consensus/validation.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <txmempool.h>
#include <util/result.h>

#include <set>
#include <string>

std::optional<std::string> SingleIssuerSelectedChecks(const CTransactionRef& ptx,
                                          const CTxMemPool::setEntries& mempool_ancestors,
                                          const std::set<Txid>& direct_conflicts,
                                          int64_t vsize);

std::optional<std::string> PackageIssuerSelectedChecks(const CTransactionRef& ptx, int64_t vsize,
                                           const Package& package,
                                           const CTxMemPool::setEntries& mempool_ancestors);

#endif //BITCOIN_POLICY_V3PLUS_POLICY_H
