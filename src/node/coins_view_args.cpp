// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/coins_view_args.h>

#include <common/args.h>
#include <tinyformat.h>
#include <txdb.h>
#include <util/result.h>
#include <util/translation.h>

#include <cstdint>

namespace node {
util::Result<void> ReadCoinsViewArgs(const ArgsManager& args, CoinsViewOptions& options)
{
    // Both options are stored in types narrower than the int64_t they are read
    // as (uint64_t and int respectively), so a negative value would otherwise
    // wrap to a huge positive one: a negative -dbbatchsize would disable batch
    // flushing entirely, and a negative -dbcrashratio would behave unpredictably.
    if (auto value{args.GetIntArg("-dbbatchsize")}) {
        if (*value < 0) {
            return util::Error{Untranslated(strprintf("-dbbatchsize must be non-negative (got %d).", *value))};
        }
        options.batch_write_bytes = static_cast<uint64_t>(*value);
    }
    if (auto value{args.GetIntArg("-dbcrashratio")}) {
        if (*value < 0) {
            return util::Error{Untranslated(strprintf("-dbcrashratio must be non-negative (got %d).", *value))};
        }
        options.simulate_crash_ratio = static_cast<int>(*value);
    }
    return {};
}
} // namespace node
