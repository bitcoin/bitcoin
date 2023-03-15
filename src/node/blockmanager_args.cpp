// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/blockmanager_args.h>

#include <util/system.h>
#include <validation.h>

namespace node {
std::optional<bilingual_str> ApplyArgsManOptions(const ArgsManager& args, BlockManager::Options& opts)
{
    // block pruning; get the amount of disk space (in MiB) to allot for block & undo files
    int64_t nPruneArg{args.GetIntArg("-prune", opts.prune_target)};
    if (nPruneArg < 0) {
        return _("Prune cannot be configured with a negative value.");
    }
    uint64_t nPruneTarget{uint64_t(nPruneArg) * 1024 * 1024};
    if (nPruneArg == 1) { // manual pruning: -prune=1
        nPruneTarget = BlockManager::PRUNE_TARGET_MANUAL;
    } else if (nPruneTarget) {
        if (nPruneTarget < MIN_DISK_SPACE_FOR_BLOCK_FILES) {
            return strprintf(_("Prune configured below the minimum of %d MiB.  Please use a higher number."), MIN_DISK_SPACE_FOR_BLOCK_FILES / 1024 / 1024);
        }
    }
    opts.prune_target = nPruneTarget;

    return std::nullopt;
}
} // namespace node
