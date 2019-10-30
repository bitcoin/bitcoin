// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mempool_args.h>

#include <kernel/mempool_limits.h>
#include <kernel/mempool_options.h>

#include <node/interface_ui.h>
#include <util/system.h>
#include <util/translation.h>

using kernel::MemPoolLimits;
using kernel::MemPoolOptions;

namespace {
bool ApplyArgsManOptions(const ArgsManager& argsman, MemPoolLimits& mempool_limits)
{
    mempool_limits.ancestor_count = argsman.GetIntArg("-limitancestorcount", mempool_limits.ancestor_count);

    if (auto vkb = argsman.GetIntArg("-limitancestorsize")) mempool_limits.ancestor_size_vbytes = *vkb * 1'000;

    mempool_limits.descendant_count = argsman.GetIntArg("-limitdescendantcount", mempool_limits.descendant_count);

    if (auto vkb = argsman.GetIntArg("-limitdescendantsize")) mempool_limits.descendant_size_vbytes = *vkb * 1'000;

    return true;
}
}

bool ApplyArgsManOptions(const ArgsManager& argsman, MemPoolOptions& mempool_opts)
{
    mempool_opts.check_ratio = argsman.GetIntArg("-checkmempool", mempool_opts.check_ratio);

    if (auto mb = argsman.GetIntArg("-maxmempool")) mempool_opts.max_size_bytes = *mb * 1'000'000;

    if (auto hours = argsman.GetIntArg("-mempoolexpiry")) mempool_opts.expiry = std::chrono::hours{*hours};

    bool enable_replacement = argsman.GetBoolArg("-mempoolreplacement", /*fDefault=*/true);
    if ((!enable_replacement) && argsman.IsArgSet("-mempoolreplacement")) {
        // Minimal effort at forwards compatibility
        std::string replacement_opt = argsman.GetArg("-mempoolreplacement", "");  // default is impossible
        std::vector<std::string> replacement_modes = SplitString(replacement_opt, ",+");
        enable_replacement = (std::find(replacement_modes.begin(), replacement_modes.end(), "fee") != replacement_modes.end());
        if (enable_replacement) {
            mempool_opts.full_rbf = (std::find(replacement_modes.begin(), replacement_modes.end(), "-optin") != replacement_modes.end());
        }
    }
    if (!enable_replacement) {
        return InitError(strprintf(_("-mempoolreplacement=%s is not supported by %s"), argsman.GetArg("-mempoolreplacement", "0"), PACKAGE_NAME));
    }

    return ApplyArgsManOptions(argsman, mempool_opts.limits);
}
