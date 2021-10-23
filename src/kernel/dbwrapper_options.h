// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_DBWRAPPER_OPTIONS_H
#define BITCOIN_KERNEL_DBWRAPPER_OPTIONS_H

#include <fs.h>

#include <cstddef>

namespace kernel {

struct DBWrapperOpts {
    //! Location in the filesystem where leveldb data will be stored.
    fs::path db_path;
    //! Configures various leveldb cache settings.
    size_t cache_size;
    //! If true, use leveldb's memory environment.
    bool in_memory = false;
    //! If true, remove all existing data.
    bool wipe_existing = false;
    //! If true, store data obfuscated via simple XOR. If false, XOR with a zero'd byte array.
    bool obfuscate_data = false;
    bool do_compact = false;
};

} // namespace kernel

#endif // BITCOIN_KERNEL_DBWRAPPER_OPTIONS_H
