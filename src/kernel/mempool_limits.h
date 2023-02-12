// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KERNEL_MEMPOOL_LIMITS_H
#define BITCOIN_KERNEL_MEMPOOL_LIMITS_H

#include <policy/policy.h>

#include <cstdint>

namespace kernel {
/**
 * Options struct containing limit options for a CTxMemPool. Default constructor
 * populates the struct with sane default values which can be modified.
 *
 * Most of the time, this struct should be referenced as CTxMemPool::Limits.
 */
struct MemPoolLimits {
    //! The maximum allowed number of transactions in a package including the entry and its ancestors.
    int64_t ancestor_count{DEFAULT_ANCESTOR_LIMIT};
    //! The maximum allowed size in virtual bytes of an entry and its ancestors within a package.
    int64_t ancestor_size_vbytes{DEFAULT_ANCESTOR_SIZE_LIMIT_KVB * 1'000};
    //! The maximum allowed number of transactions in a package including the entry and its descendants.
    int64_t descendant_count{DEFAULT_DESCENDANT_LIMIT};
    //! The maximum allowed size in virtual bytes of an entry and its descendants within a package.
    int64_t descendant_size_vbytes{DEFAULT_DESCENDANT_SIZE_LIMIT_KVB * 1'000};

    /**
     * @return MemPoolLimits with all the limits set to the maximum
     */
    static constexpr MemPoolLimits NoLimits()
    {
        int64_t no_limit{std::numeric_limits<int64_t>::max()};
        return {no_limit, no_limit, no_limit, no_limit};
    }
};
} // namespace kernel

#endif // BITCOIN_KERNEL_MEMPOOL_LIMITS_H
