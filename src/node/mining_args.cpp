// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mining_args.h>

#include <common/args.h>
#include <common/messages.h>
#include <consensus/consensus.h>
#include <node/mining_types.h>
#include <tinyformat.h>
#include <util/moneystr.h>
#include <util/translation.h>

#include <cstdint>

using common::AmountErrMsg;
using util::Error;
using util::Result;

namespace node {

Result<void> ReadMiningArgs(const ArgsManager& args)
{
    if (const auto arg{args.GetArg("-blockmintxfee")}) {
        if (!ParseMoney(*arg)) {
            return Error{AmountErrMsg("blockmintxfee", *arg)};
        }
    }

    const uint64_t max_block_weight{args.GetArg<uint64_t>("-blockmaxweight", DEFAULT_BLOCK_MAX_WEIGHT)};
    if (max_block_weight > MAX_BLOCK_WEIGHT) {
        return Error{strprintf(_("Specified -blockmaxweight (%d) exceeds consensus maximum block weight (%d)"), max_block_weight, MAX_BLOCK_WEIGHT)};
    }

    const uint64_t block_reserved_weight{args.GetArg<uint64_t>("-blockreservedweight", DEFAULT_BLOCK_RESERVED_WEIGHT)};
    if (block_reserved_weight > MAX_BLOCK_WEIGHT) {
        return Error{strprintf(_("Specified -blockreservedweight (%d) exceeds consensus maximum block weight (%d)"), block_reserved_weight, MAX_BLOCK_WEIGHT)};
    }
    if (block_reserved_weight < MINIMUM_BLOCK_RESERVED_WEIGHT) {
        return Error{strprintf(_("Specified -blockreservedweight (%d) is lower than minimum safety value of (%d)"), block_reserved_weight, MINIMUM_BLOCK_RESERVED_WEIGHT)};
    }

    return {};
}

} // namespace node
