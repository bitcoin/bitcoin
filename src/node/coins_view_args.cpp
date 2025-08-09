// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/caches.h>

#include <node/coins_view_args.h>

#include <common/args.h>
#include <txdb.h>

namespace node {
void ReadCoinsViewArgs(const ArgsManager& args, CoinsViewOptions& options)
{
    if (const auto value{args.GetIntArg("-dbbatchsize")}) {
        options.batch_write_bytes = *value;
    } else {
        options.batch_write_bytes = GetDbBatchSize(args.GetIntArg("-dbcache", DEFAULT_KERNEL_CACHE));
    }

    if (const auto value{args.GetIntArg("-dbcrashratio")}) {
        options.simulate_crash_ratio = *value;
    }
}
} // namespace node
