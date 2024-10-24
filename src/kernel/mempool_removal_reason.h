// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_KERNEL_MEMPOOL_REMOVAL_REASON_H
#define BITCOIN_KERNEL_MEMPOOL_REMOVAL_REASON_H

#include <primitives/transaction.h>
#include <uint256.h>

#include <string>
#include <variant>

/** Reason why a transaction was removed from the mempool,
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
    uint256 conflicting_block_hash;
    unsigned int conflicting_block_height;

    explicit ConflictReason(const uint256& conflicting_block_hash, int conflicting_block_height) : conflicting_block_hash(conflicting_block_hash), conflicting_block_height(conflicting_block_height) {}
    ConflictReason(std::nullptr_t, int conflicting_block_height) = delete;

    std::string toString() const noexcept {
        return "conflict";
    }
};

struct ReplacedReason {
    CTransactionRef replacement_tx;

    explicit ReplacedReason(const CTransactionRef replacement_tx) : replacement_tx(replacement_tx) {}
    ReplacedReason(std::nullptr_t) = delete;

    std::string toString() const noexcept {
        return "replaced";
    }
};

using MemPoolRemovalReason = std::variant<ExpiryReason, SizeLimitReason, ReorgReason, BlockReason, ConflictReason, ReplacedReason>;

std::string RemovalReasonToString(const MemPoolRemovalReason& r) noexcept;

template <typename T>
bool IsReason(const MemPoolRemovalReason& reason)
{
    return std::get_if<T>(&reason) != nullptr;
}

#endif // BITCOIN_KERNEL_MEMPOOL_REMOVAL_REASON_H
