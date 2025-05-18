// Copyright (c) 2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mempool_persist_args.h>

#include <common/args.h>
#include <util/fs.h>
#include <validation.h>

namespace node {

bool ShouldPersistMempool(const ArgsManager& argsman)
{
    return argsman.GetBoolArg("-persistmempool", DEFAULT_PERSIST_MEMPOOL);
}

fs::path MempoolPath(const ArgsManager& argsman)
{
    return argsman.GetDataDirNet() / "mempool.dat";
}

} // namespace node
