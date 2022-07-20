// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mempool_args.h>

#include <kernel/mempool_limits.h>
#include <kernel/mempool_options.h>

#include <chainparams.h>
#include <tinyformat.h>
#include <util/system.h>
#include <util/translation.h>

#include <chrono>
#include <memory>

using kernel::MemPoolLimits;
using kernel::MemPoolOptions;

namespace {
void ApplyArgsManOptions(const ArgsManager& argsman, MemPoolLimits& mempool_limits)
{
    mempool_limits.ancestor_count = argsman.GetIntArg("-limitancestorcount", mempool_limits.ancestor_count);

    if (auto vkb = argsman.GetIntArg("-limitancestorsize")) mempool_limits.ancestor_size_vbytes = *vkb * 1'000;

    mempool_limits.descendant_count = argsman.GetIntArg("-limitdescendantcount", mempool_limits.descendant_count);

    if (auto vkb = argsman.GetIntArg("-limitdescendantsize")) mempool_limits.descendant_size_vbytes = *vkb * 1'000;
}
}

std::optional<bilingual_str> ApplyArgsManOptions(const ArgsManager& argsman, const CChainParams& chainparams, MemPoolOptions& mempool_opts)
{
    mempool_opts.check_ratio = argsman.GetIntArg("-checkmempool", mempool_opts.check_ratio);

    if (auto mb = argsman.GetIntArg("-maxmempool")) mempool_opts.max_size_bytes = *mb * 1'000'000;

    if (auto hours = argsman.GetIntArg("-mempoolexpiry")) mempool_opts.expiry = std::chrono::hours{*hours};

    mempool_opts.full_rbf = argsman.GetBoolArg("-mempoolfullrbf", mempool_opts.full_rbf);

    ApplyArgsManOptions(argsman, mempool_opts.limits);

    return std::nullopt;
}
