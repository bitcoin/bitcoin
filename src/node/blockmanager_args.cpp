// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/blockmanager_args.h>

#include <common/args.h>
#include <node/blockstorage.h>
#include <node/database_args.h>
#include <tinyformat.h>
#include <util/result.h>
#include <util/translation.h>
#include <validation.h>

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace node {
util::Result<uint64_t> ParsePruneOption(const int64_t nPruneArg, const std::string_view opt_name)
{
    // block pruning; get the amount of disk space (in MiB) to allot for block & undo files
    if (nPruneArg < 0) {
        return util::Error{strprintf(_("%s cannot be configured with a negative value."), opt_name)};
    } else if (nPruneArg == 0) { // pruning disabled
        return 0;
    } else if (nPruneArg == 1) { // manual pruning: -prune=1
        return BlockManager::PRUNE_TARGET_MANUAL;
    }
    const uint64_t nPruneTarget{uint64_t(nPruneArg) * 1024 * 1024};
    if (nPruneTarget < MIN_DISK_SPACE_FOR_BLOCK_FILES) {
        return util::Error{strprintf(_("%s configured below the minimum of %d MiB.  Please use a higher number."), opt_name, MIN_DISK_SPACE_FOR_BLOCK_FILES / 1024 / 1024)};
    }
    return nPruneTarget;
}

util::Result<void> ApplyArgsManOptions(const ArgsManager& args, BlockManager::Options& opts)
{
    if (auto value{args.GetBoolArg("-blocksxor")}) opts.use_xor = *value;

    // block pruning; get the amount of disk space (in MiB) to allot for block & undo files
    int64_t nPruneArg{args.GetIntArg("-prune", opts.prune_target)};
    if (const auto prune_parsed = ParsePruneOption(nPruneArg, "Prune")) {
        opts.prune_target = *prune_parsed;
    } else {
        return util::Error{util::ErrorString(prune_parsed)};
    }

    if (const auto prune_during_init{args.GetIntArg("-pruneduringinit")}) {
        if (*prune_during_init == -1) {
            opts.prune_target_during_init = -1;
        } else if (const auto prune_parsed = ParsePruneOption(*prune_during_init, "-pruneduringinit")) {
            // NOTE: PRUNE_TARGET_MANUAL is >int64 max
            opts.prune_target_during_init = std::min(std::numeric_limits<int64_t>::max(), (int64_t)*prune_parsed);
        } else {
            return util::Error{util::ErrorString(prune_parsed)};
        }
    }

    if (auto value{args.GetBoolArg("-fastprune")}) opts.fast_prune = *value;

    ReadDatabaseArgs(args, opts.block_tree_db_params.options);

    return {};
}
} // namespace node
