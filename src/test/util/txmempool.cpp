// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/txmempool.h>

#include <chainparams.h>
#include <node/context.h>
#include <node/mempool_args.h>
#include <policy/rbf.h>
#include <policy/v3_policy.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>

using node::NodeContext;

CTxMemPool::Options MemPoolOptionsForTest(const NodeContext& node)
{
    CTxMemPool::Options mempool_opts{
        // Default to always checking mempool regardless of
        // chainparams.DefaultConsistencyChecks for tests
        .check_ratio = 1,
        .signals = node.validation_signals.get(),
    };
    const auto result{ApplyArgsManOptions(*node.args, ::Params(), mempool_opts)};
    Assert(result);
    return mempool_opts;
}

CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CMutableTransaction& tx) const
{
    return FromTx(MakeTransactionRef(tx));
}

CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CTransactionRef& tx) const
{
    return CTxMemPoolEntry{tx, nFee, TicksSinceEpoch<std::chrono::seconds>(time), nHeight, m_sequence, spendsCoinbase, sigOpCost, lp};
}

std::optional<std::string> CheckPackageMempoolAcceptResult(const Package& txns,
                                                           const PackageMempoolAcceptResult& result,
                                                           bool expect_valid,
                                                           const CTxMemPool* mempool)
{
    if (expect_valid) {
        if (result.m_state.IsInvalid()) {
            return strprintf("Package validation unexpectedly failed: %s", result.m_state.ToString());
        }
    } else {
        if (result.m_state.IsValid()) {
            return strprintf("Package validation unexpectedly succeeded. %s", result.m_state.ToString());
        }
    }
    if (result.m_state.GetResult() != PackageValidationResult::PCKG_POLICY && txns.size() != result.m_tx_results.size()) {
        return strprintf("txns size %u does not match tx results size %u", txns.size(), result.m_tx_results.size());
    }
    for (const auto& tx : txns) {
        const auto& wtxid = tx->GetWitnessHash();
        if (result.m_tx_results.count(wtxid) == 0) {
            return strprintf("result not found for tx %s", wtxid.ToString());
        }

        const auto& atmp_result = result.m_tx_results.at(wtxid);
        const bool valid{atmp_result.m_result_type == MempoolAcceptResult::ResultType::VALID};
        if (expect_valid && atmp_result.m_state.IsInvalid()) {
            return strprintf("tx %s unexpectedly failed: %s", wtxid.ToString(), atmp_result.m_state.ToString());
        }

        // Each subpackage is allowed MAX_REPLACEMENT_CANDIDATES replacements (only checking individually here)
        if (atmp_result.m_replaced_transactions.size() > MAX_REPLACEMENT_CANDIDATES) {
            return strprintf("tx %s result replaced too many transactions",
                                wtxid.ToString());
        }

        // Replacements can't happen for subpackages larger than 2
        if (!atmp_result.m_replaced_transactions.empty() &&
            atmp_result.m_wtxids_fee_calculations.has_value() && atmp_result.m_wtxids_fee_calculations.value().size() > 2) {
             return strprintf("tx %s was part of a too-large package RBF subpackage",
                                wtxid.ToString());
        }

        if (!atmp_result.m_replaced_transactions.empty() && mempool) {
            LOCK(mempool->cs);
            // If replacements occurred and it used 2 transactions, this is a package RBF and should result in a cluster of size 2
            if (atmp_result.m_wtxids_fee_calculations.has_value() && atmp_result.m_wtxids_fee_calculations.value().size() == 2) {
                const auto cluster = mempool->GatherClusters({tx->GetHash()});
                if (cluster.size() != 2) return strprintf("tx %s has too many ancestors or descendants for a package rbf", wtxid.ToString());
            }
        }

        // m_vsize and m_base_fees should exist iff the result was VALID or MEMPOOL_ENTRY
        const bool mempool_entry{atmp_result.m_result_type == MempoolAcceptResult::ResultType::MEMPOOL_ENTRY};
        if (atmp_result.m_base_fees.has_value() != (valid || mempool_entry)) {
            return strprintf("tx %s result should %shave m_base_fees", wtxid.ToString(), valid || mempool_entry ? "" : "not ");
        }
        if (atmp_result.m_vsize.has_value() != (valid || mempool_entry)) {
            return strprintf("tx %s result should %shave m_vsize", wtxid.ToString(), valid || mempool_entry ? "" : "not ");
        }

        // m_other_wtxid should exist iff the result was DIFFERENT_WITNESS
        const bool diff_witness{atmp_result.m_result_type == MempoolAcceptResult::ResultType::DIFFERENT_WITNESS};
        if (atmp_result.m_other_wtxid.has_value() != diff_witness) {
            return strprintf("tx %s result should %shave m_other_wtxid", wtxid.ToString(), diff_witness ? "" : "not ");
        }

        // m_effective_feerate and m_wtxids_fee_calculations should exist iff the result was valid
        // or if the failure was TX_RECONSIDERABLE
        const bool valid_or_reconsiderable{atmp_result.m_result_type == MempoolAcceptResult::ResultType::VALID ||
                    atmp_result.m_state.GetResult() == TxValidationResult::TX_RECONSIDERABLE};
        if (atmp_result.m_effective_feerate.has_value() != valid_or_reconsiderable) {
            return strprintf("tx %s result should %shave m_effective_feerate",
                                    wtxid.ToString(), valid ? "" : "not ");
        }
        if (atmp_result.m_wtxids_fee_calculations.has_value() != valid_or_reconsiderable) {
            return strprintf("tx %s result should %shave m_effective_feerate",
                                    wtxid.ToString(), valid ? "" : "not ");
        }

        if (mempool) {
            // The tx by txid should be in the mempool iff the result was not INVALID.
            const bool txid_in_mempool{atmp_result.m_result_type != MempoolAcceptResult::ResultType::INVALID};
            if (mempool->exists(GenTxid::Txid(tx->GetHash())) != txid_in_mempool) {
                return strprintf("tx %s should %sbe in mempool", wtxid.ToString(), txid_in_mempool ? "" : "not ");
            }
            // Additionally, if the result was DIFFERENT_WITNESS, we shouldn't be able to find the tx in mempool by wtxid.
            if (tx->HasWitness() && atmp_result.m_result_type == MempoolAcceptResult::ResultType::DIFFERENT_WITNESS) {
                if (mempool->exists(GenTxid::Wtxid(wtxid))) {
                    return strprintf("wtxid %s should not be in mempool", wtxid.ToString());
                }
            }
            for (const auto& tx_ref : atmp_result.m_replaced_transactions) {
                if (mempool->exists(GenTxid::Txid(tx_ref->GetHash()))) {
                    return strprintf("tx %s should not be in mempool as it was replaced", tx_ref->GetWitnessHash().ToString());
                }
            }
        }
    }
    return std::nullopt;
}

void CheckMempoolV3Invariants(const CTxMemPool& tx_pool)
{
    LOCK(tx_pool.cs);
    for (const auto& tx_info : tx_pool.infoAll()) {
        const auto& entry = *Assert(tx_pool.GetEntry(tx_info.tx->GetHash()));
        if (tx_info.tx->version == TRUC_VERSION) {
            // Check that special maximum virtual size is respected
            Assert(entry.GetTxSize() <= V3_MAX_VSIZE);

            // Check that special v3 ancestor/descendant limits and rules are always respected
            Assert(entry.GetCountWithDescendants() <= V3_DESCENDANT_LIMIT);
            Assert(entry.GetCountWithAncestors() <= V3_ANCESTOR_LIMIT);
            Assert(entry.GetSizeWithDescendants() <= V3_MAX_VSIZE + V3_CHILD_MAX_VSIZE);
            Assert(entry.GetSizeWithAncestors() <= V3_MAX_VSIZE + V3_CHILD_MAX_VSIZE);

            // If this transaction has at least 1 ancestor, it's a "child" and has restricted weight.
            if (entry.GetCountWithAncestors() > 1) {
                Assert(entry.GetTxSize() <= V3_CHILD_MAX_VSIZE);
                // All v3 transactions must only have v3 unconfirmed parents.
                const auto& parents = entry.GetMemPoolParentsConst();
                Assert(parents.begin()->get().GetSharedTx()->version == TRUC_VERSION);
            }
        } else if (entry.GetCountWithAncestors() > 1) {
            // All non-v3 transactions must only have non-v3 unconfirmed parents.
            for (const auto& parent : entry.GetMemPoolParentsConst()) {
                Assert(parent.get().GetSharedTx()->version != TRUC_VERSION);
            }
        }
    }
}
