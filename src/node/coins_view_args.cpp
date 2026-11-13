// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/coins_view_args.h>

#include <common/args.h>
#include <tinyformat.h>
#include <txdb.h>
#include <util/result.h>
#include <util/strencodings.h>
#include <util/translation.h>

#include <cstdint>
#include <limits>

namespace node {
util::Result<void> ReadCoinsViewArgs(const ArgsManager& args, CoinsViewOptions& options)
{
    if (const auto arg{args.GetArg("-dbbatchsize")}) {
        // Parse into the destination's width. A negative value would wrap to a huge
        // batch size and disable flushing entirely, so it must be rejected; ToIntegral
        // is used rather than GetArg<uint64_t>() because the typed getter saturates
        // values that fail to parse, so "-dbbatchsize=-1" would silently become 0.
        const auto bytes{ToIntegral<uint64_t>(*arg)};
        if (!bytes) {
            return util::Error{Untranslated(strprintf("-dbbatchsize must be a non-negative number of bytes (got %s).", *arg))};
        }
        options.batch_write_bytes = *bytes;
    }
    if (const auto arg{args.GetArg("-dbcrashratio")}) {
        // simulate_crash_ratio is uint32_t, so parse straight into that width. A
        // negative value would otherwise become a huge ratio (or trip the positivity
        // assumption inside FastRandomContext::randrange), and a value above the
        // destination's range would narrow to a different ratio than requested.
        const auto ratio{ToIntegral<uint32_t>(*arg)};
        if (!ratio) {
            return util::Error{Untranslated(strprintf("-dbcrashratio must be between 0 and %u (got %s).", std::numeric_limits<uint32_t>::max(), *arg))};
        }
        options.simulate_crash_ratio = *ratio;
    }
    return {};
}
} // namespace node
