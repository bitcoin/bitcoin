// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CHAINSTATE_H
#define BITCOIN_NODE_CHAINSTATE_H

#include <kernel/notifications_interface.h>
#include <util/result.h>
#include <util/translation.h>
#include <validation.h>

#include <cstdint>
#include <functional>
#include <tuple>

class CTxMemPool;

namespace node {

struct CacheSizes;

struct ChainstateLoadOptions {
    CTxMemPool* mempool{nullptr};
    bool block_tree_db_in_memory{false};
    bool coins_db_in_memory{false};
    bool reindex{false};
    bool reindex_chainstate{false};
    bool prune{false};
    //! Setting require_full_verification to true will require all checks at
    //! check_level (below) to succeed for loading to succeed. Setting it to
    //! false will skip checks if cache is not big enough to run them, so may be
    //! helpful for running with a small cache.
    bool require_full_verification{true};
    int64_t check_blocks{DEFAULT_CHECKBLOCKS};
    int64_t check_level{DEFAULT_CHECKLEVEL};
    std::function<void()> coins_error_cb;
};

//! Chainstate load status. Simple applications can just check for the success
//! case, and treat other cases as errors. More complex applications may want to
//! try reindexing in the generic failure case, and pass an interrupt callback
//! and exit cleanly in the interrupted case.
enum class ChainstateLoadError {
    FAILURE, //!< Generic failure which reindexing may fix
    FAILURE_FATAL, //!< Fatal error which should not prompt to reindex
    FAILURE_INCOMPATIBLE_DB,
    FAILURE_INSUFFICIENT_DBCACHE,
};

kernel::FlushResult<kernel::InterruptResult, ChainstateLoadError> LoadChainstate(ChainstateManager& chainman, const CacheSizes& cache_sizes,
                                                                                 const ChainstateLoadOptions& options);
kernel::FlushResult<kernel::InterruptResult, ChainstateLoadError> VerifyLoadedChainstate(ChainstateManager& chainman, const ChainstateLoadOptions& options);
} // namespace node

#endif // BITCOIN_NODE_CHAINSTATE_H
