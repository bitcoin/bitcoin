// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_MEMPOOL_PERSIST_H
#define BITCOIN_NODE_MEMPOOL_PERSIST_H

#include <util/expected.h>
#include <util/fs.h>

#include <cstdint>
#include <string_view>

class Chainstate;
class CTxMemPool;

namespace node {

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

enum class MempoolLoadError {
    NO_LOAD_PATH,
    FILE_OPEN_FAILED,
    UNSUPPORTED_VERSION,
    INTERRUPTED,
    DESERIALIZATION_FAILED,
};

/** Return a string representation for diagnostics. */
std::string_view MempoolLoadErrorString(MempoolLoadError error);

//! On success, contains the total weight of transactions in the persisted snapshot.
using MempoolLoadResult = util::Expected<uint64_t, MempoolLoadError>;

/** Import the file and attempt to add its contents to the mempool. */
MempoolLoadResult LoadMempool(CTxMemPool& pool, const fs::path& load_path,
                              Chainstate& active_chainstate,
                              ImportMempoolOptions&& opts);

} // namespace node


#endif // BITCOIN_NODE_MEMPOOL_PERSIST_H
