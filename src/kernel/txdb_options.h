// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_TXDB_OPTIONS_H
#define BITCOIN_KERNEL_TXDB_OPTIONS_H

#include <kernel/dbwrapper_options.h>

#include <cstdint>
#include <functional>

class CChainParams;

namespace kernel {

/**
 * An options struct for `CBlockTreeDB`, more ergonomically referred to as
 * `CBlockTreeDB::Options` due to the using-declaration in
 * `CBlockTreeDB`.
 */
struct BlockTreeDBOpts {
    fs::path db_path;
    size_t cache_size;
    bool in_memory = false;
    bool wipe_existing = false;
    bool do_compact = false;

    DBWrapperOpts ToDBWrapperOptions() const
    {
        return DBWrapperOpts{
            .db_path = db_path,
            .cache_size = cache_size,
            .in_memory = in_memory,
            .wipe_existing = wipe_existing,
            .obfuscate_data = false,
            .do_compact = do_compact,
        };
    }
};

} // namespace kernel

#endif // BITCOIN_KERNEL_TXDB_OPTIONS_H
