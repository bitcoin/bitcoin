// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/issuer_selected_policy.h>

#include <coins.h>
#include <consensus/amount.h>
#include <logging.h>
#include <tinyformat.h>
#include <util/check.h>

#include <algorithm>
#include <numeric>
#include <vector>

// If this flag is set, CTxIn::nSequence of V3 transaction is not interpreted as a relative lock-time.
static const uint32_t SEQUENCE_ISSUER_SELECTED_LIMITS_DISABLE_FLAG = (1U << 30);

// This mask is applied to extract ancestor limit from the sequence field.
static const uint32_t SEQUENCE_ISSUER_SELECTED_ANCESTOR_MASK = 0x00ff0000;

// This mask is applied to extract descendant limit from the sequence field.
static const uint32_t SEQUENCE_ISSUER_SELECTED_DESCENDANT_MASK = 0x0000ff00;

// This mask is applied to extract package-level vbytes limit from the sequence field.
static const uint32_t SEQUENCE_ISSUER_SELECTED_WEIGHT_MASK = 0x000000ff;

// Granularity-level: 1024 vbytes
static const uint32_t SEQUENCE_ISSUER_SELECTED_WEIGHT_GRANULARITY = 10;

struct IssuerSelectedLimits {
    uint32_t m_ancestor_limit;
    /** Wtxid used for debug string */
    uint32_t m_descendant_limit;
    /** nVersion used to check inheritance of v3 and non-v3 */
    int64_t m_package_max_size;

    IssuerSelectedLimits() = delete;
    IssuerSelectedLimits(uint32_t ancestor_limit, uint32_t descendant_limit , int64_t package_max_size) :
        m_ancestor_limit{ancestor_limit}, m_descendant_limit{descendant_limit}, m_package_max_size{package_max_size}
    {}
};

std::optional<IssuerSelectedLimits> ParsePackageTopologicalLimits(const CTransactionRef& ptx)
{
    uint32_t sequence_field = ptx->vin[0].nSequence;
 
    // Mark incompatibility between V3+ tagged input and relative-lock time (bip68) due to the 32-bits nSequence
    // field constraints.This is not an issue for current deployed LN pre-signed transactions using relative-lock time,
    // as only malleable second-stage transactions (`option_anchors_zero_fee_htlc_tx`) are encumbered by a CSV 1.
    // LN transactions issuers can generate their second-stage transactions by placing a dummy input with the
    // issuer-selected topological and size limits, without channel-type level upgrades.
    // There are few "cold custody" / multi-sig use-cases relying on relative-locktime usage, however there is
    // no ecosystem-level documentation informing on the exact uses of relative lock time, neither available 
    // on-chain economic metrics as in the optimistic case there are never published.
    if (sequence_field & CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG) {
        return std::nullopt;
    }

    // Mark v3+ semantics as transaction / package issuer opt-in only.
    if (sequence_field & SEQUENCE_ISSUER_SELECTED_LIMITS_DISABLE_FLAG) {
        return std::nullopt;
    }

    const uint32_t ancestor_limit = sequence_field & SEQUENCE_ISSUER_SELECTED_ANCESTOR_MASK;
    const uint32_t descendant_limit = sequence_field & SEQUENCE_ISSUER_SELECTED_DESCENDANT_MASK;
    const uint32_t weight_limit = (uint32_t)(sequence_field & SEQUENCE_ISSUER_SELECTED_WEIGHT_MASK) << SEQUENCE_ISSUER_SELECTED_WEIGHT_GRANULARITY;

    // This parsing logic can be adapted for forward-compatibility in matters of issuer-selected policy limits:
    //      - encodes accumulated package SigOpCost
    //      - commit to package-level dynamic DUST_RELAY_TX_FEE or DEFAULT_INCREMENTAL_RELAY_FEE
    //      - opt-in if limits are strict (at the pckg-level) or composable (at the tx-level) to allow non-interactive composability among N chain of transaction issuers.

    IssuerSelectedLimits tx_issuer_selected_limits(ancestor_limit, descendant_limit, weight_limit);

    return tx_issuer_selected_limits;
}

std::optional<std::string> PackageIssuerSelectedChecks(const CTransactionRef& ptx, int64_t vsize,
                                           const Package& package,
                                           const CTxMemPool::setEntries& mempool_ancestors)
{

    // We fetch the issuer-selected from the reference_tx. Lack of issuer-selected opt-in by reference tx should be treated as an error.
    std::optional<IssuerSelectedLimits> issuer_selected_limits_ret = ParsePackageTopologicalLimits(ptx);
    if (!issuer_selected_limits_ret.has_value()) return std::nullopt;
    uint32_t reference_tx_ancestor_limit = (*issuer_selected_limits_ret).m_ancestor_limit;
    uint32_t reference_tx_descendant_limit = (*issuer_selected_limits_ret).m_descendant_limit;
    int32_t reference_tx_package_max_size = (*issuer_selected_limits_ret).m_package_max_size;

    // Check that "issuer selected" package limits are within mempool default limits.
    if ((reference_tx_ancestor_limit < 1 || reference_tx_ancestor_limit > DEFAULT_ANCESTOR_LIMIT)
        || (reference_tx_descendant_limit < 1 || reference_tx_descendant_limit > DEFAULT_DESCENDANT_LIMIT)
        || (reference_tx_package_max_size < ((int32_t) MIN_STANDARD_TX_NONWITNESS_SIZE) || reference_tx_package_max_size > MAX_STANDARD_TX_WEIGHT)) {
            return strprintf("issuer-selected policy: issuer-selected limits should be in bounds of hard tx-relay policy limits");
    }

    int32_t accumulated_package_weight = GetTransactionWeight(*ptx);

    if (mempool_ancestors.size() > reference_tx_ancestor_limit) {
            return strprintf("issuer-selected policy: issuer-selected limits should be in bounds of hard tx-relay policy limits");
    }
    
    // We verify that all the transactions in the package have equivalent issuer-selected limits.
    for (const auto& package_tx : package) {

        // All issuer-selected policy packages txn must be nversion=3
        if (package_tx->nVersion != 3) {
            return strprintf("v3 tx %s (wtxid=%s) cannot spend from non-v3",
                             package_tx->GetHash().ToString(), package_tx->GetWitnessHash().ToString());
        }
        std::optional<IssuerSelectedLimits> issuer_selected_limits_ret = ParsePackageTopologicalLimits(package_tx);
        if (!issuer_selected_limits_ret.has_value()) return strprintf("strict issuer-selected policy: all txns should have opted-in in issuer-selected limits");
        uint32_t check_tx_ancestor_limit = (*issuer_selected_limits_ret).m_ancestor_limit;
        uint32_t check_tx_descendant_limit = (*issuer_selected_limits_ret).m_descendant_limit;
        int32_t check_tx_package_max_size = (*issuer_selected_limits_ret).m_package_max_size;

        // We enforce strict v3+ policy, all the issuer-selected limits should be equivalent
        if ((reference_tx_ancestor_limit != check_tx_ancestor_limit)
                || (reference_tx_descendant_limit != check_tx_descendant_limit)
                || (reference_tx_package_max_size != check_tx_package_max_size)) {
            return strprintf("strict issuer-selected policy: all txns should have equivalent issuer-selected limits");
        }

        accumulated_package_weight += GetTransactionWeight(*package_tx);
    }

    // Be more conservative than in-mempool ancestor / descendant accumulated size limit.
    if (accumulated_package_weight > MAX_STANDARD_TX_WEIGHT) {
            return strprintf("issuer-selected policy: issuer-selected limits should be in bounds of hard tx-relay policy limits");
    }

    // We start at 0 to account for the transaction itself.
    uint32_t accumulated_descendant = 0;
    for (const auto& mempool_ancestor: mempool_ancestors) {
            // We subtract the child from any children of all of its mempool ancestor
            const auto& children = mempool_ancestor->GetMemPoolChildrenConst();
            accumulated_descendant += (children.size() - 1);
    }

    // We previously verified that issuer-selected descendant limit where equal to MAX_PACKAGE_COUNT
    if ((accumulated_descendant > reference_tx_descendant_limit) || (package.size() > reference_tx_descendant_limit)) {
        return strprintf("issuer-selected policy: issuer-selected limits should be in bounds of hard tx-relay policy limits");
    }

    return std::nullopt;
}

std::optional<std::string> SingleIssuerSelectedChecks(const CTransactionRef& ptx,
                                          const CTxMemPool::setEntries& mempool_ancestors,
                                          const std::set<Txid>& direct_conflicts,
                                          int64_t vsize)
{
    // Check v3+ and non-v3+ inheritance.
    for (const auto& entry : mempool_ancestors) {
        if (ptx->nVersion != 3 && entry->GetTx().nVersion == 3) {
            return strprintf("non-v3 tx %s (wtxid=%s) cannot spend from v3 tx %s (wtxid=%s)",
                             ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString(),
                             entry->GetSharedTx()->GetHash().ToString(), entry->GetSharedTx()->GetWitnessHash().ToString());
        } else if (ptx->nVersion == 3 && entry->GetTx().nVersion != 3) {
            return strprintf("v3 tx %s (wtxid=%s) cannot spend from non-v3 tx %s (wtxid=%s)",
                             ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString(),
                             entry->GetSharedTx()->GetHash().ToString(), entry->GetSharedTx()->GetWitnessHash().ToString());
        }
    }

    // The rest of the rules only apply to transactions with nVersion=3.
    if (ptx->nVersion != 3) return std::nullopt;

    std::optional<IssuerSelectedLimits> issuer_selected_limits_ret = ParsePackageTopologicalLimits(ptx);
    if (!issuer_selected_limits_ret.has_value()) return std::nullopt;
    uint32_t tx_ancestor_limit = (*issuer_selected_limits_ret).m_ancestor_limit;
    uint32_t tx_descendant_limit = (*issuer_selected_limits_ret).m_descendant_limit;
    int32_t tx_package_max_size = (*issuer_selected_limits_ret).m_package_max_size;

    // Check that "issuer selected" package limits are within mempool default limits.
    if ((tx_ancestor_limit < 1 || tx_ancestor_limit > DEFAULT_ANCESTOR_LIMIT)
        || (tx_descendant_limit < 1 || tx_descendant_limit > DEFAULT_DESCENDANT_LIMIT)
        || (tx_package_max_size < ((int32_t) MIN_STANDARD_TX_NONWITNESS_SIZE) || tx_package_max_size > MAX_STANDARD_TX_WEIGHT)) {
            return strprintf("issuer-selected policy: issuer-selected limits should be in bounds of hard tx-relay policy limits");
    }

    // Check that "transaction-issuer selected" package limits are respected.
    if (mempool_ancestors.size() + 1 > tx_ancestor_limit) {
        return strprintf("tx %s (wtxid=%s) would have too many ancestors",
                         ptx->GetHash().ToString(), ptx->GetWitnessHash().ToString());
    }

    // Check the descendant counts of in-mempool ancestors.
    const auto& parent_entry = *mempool_ancestors.begin();

    // We allow one more descendant if this single issuer-selected transaction replaces a subpackage.
    if (parent_entry->GetCountWithDescendants() + 1 > tx_descendant_limit) {
        return strprintf("tx %u (wtxid=%s) would exceed descendant count limit",
                         parent_entry->GetSharedTx()->GetHash().ToString(),
                         parent_entry->GetSharedTx()->GetWitnessHash().ToString());
    }

    return std::nullopt;
}
