// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_KERNEL_MEMPOOL_REMOVAL_REASON_H
#define BITCOIN_KERNEL_MEMPOOL_REMOVAL_REASON_H

#include <primitives/transaction.h>

#include <string>
#include <variant>

/** Reasons why a transaction was removed from the mempool,
 * this is passed to the notification signal.
 */

struct ExpiryReason {
    std::string toString() const noexcept {
        return "expiry";
    }
};

struct SizeLimitReason {
    std::string toString() const noexcept {
        return "sizelimit";
    }
};

struct ReorgReason {
    std::string toString() const noexcept {
        return "reorg";
    }
};

struct BlockReason {
    std::string toString() const noexcept {
        return "block";
    }
};

struct ConflictReason {
    std::string toString() const noexcept {
        return "conflict";
    }
};

struct ReplacedReason {
    /**
    * The double-spending tx
    */
    CTransactionRef conflicting_tx;

    explicit ReplacedReason(const CTransactionRef conflicting_tx) : conflicting_tx(conflicting_tx) {}
    std::string toString() const noexcept {
        return "replaced";
    }
};

using MemPoolRemovalReason = std::variant<ExpiryReason, SizeLimitReason, ReorgReason, BlockReason, ConflictReason, ReplacedReason>;

std::string RemovalReasonToString(const MemPoolRemovalReason& r) noexcept;

#endif // BITCOIN_KERNEL_MEMPOOL_REMOVAL_REASON_H
