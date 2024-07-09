// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_NODE_CACHES_H
#define SYSCOIN_NODE_CACHES_H

#include <cstddef>
#include <cstdint>

class ArgsManager;

namespace node {
struct CacheSizes {
    int64_t block_tree_db;
    int64_t coins_db;
    int64_t coins;
    int64_t tx_index;
    int64_t filter_index;
    // SYSCOIN
    int64_t evo_dmn_db;
    int64_t evo_qc_db;
    int64_t evo_qvvecs_db;
    int64_t evo_qsk_db;
    int64_t evo_poda_db;
};
CacheSizes CalculateCacheSizes(const ArgsManager& args, size_t n_indexes = 0);
} // namespace node

#endif // SYSCOIN_NODE_CACHES_H
