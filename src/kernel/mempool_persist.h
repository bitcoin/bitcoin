// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_MEMPOOL_PERSIST_H
#define BITCOIN_KERNEL_MEMPOOL_PERSIST_H

#include <node/blockstorage.h>
#include <kernel/result.h>
#include <util/fs.h>

class Chainstate;
class CTxMemPool;

namespace kernel {

/** Dump the mempool to a file. */
bool DumpMempool(const CTxMemPool& pool, const fs::path& dump_path,
                 fsbridge::FopenFn mockable_fopen_function = fsbridge::fopen,
                 bool skip_file_commit = false);

struct ImportMempoolOptions {
    fsbridge::FopenFn mockable_fopen_function{fsbridge::fopen};
    bool use_current_time{false};
    bool apply_fee_delta_priority{true};
    bool apply_unbroadcast_set{true};
};
/** Import the file and attempt to add its contents to the mempool. */
FlushResult<InterruptResult> LoadMempool(CTxMemPool& pool, const fs::path& load_path,
                 Chainstate& active_chainstate,
                 ImportMempoolOptions&& opts);

} // namespace kernel


#endif // BITCOIN_KERNEL_MEMPOOL_PERSIST_H
