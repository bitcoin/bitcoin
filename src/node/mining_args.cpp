// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mining_args.h>

#include <common/args.h>
#include <common/messages.h>
#include <consensus/consensus.h>
#include <node/mining.h>
#include <tinyformat.h>
#include <util/moneystr.h>
#include <util/translation.h>

using common::AmountErrMsg;

namespace node {

util::Result<void> ApplyArgsManOptions(const ArgsManager& args)
{
    if (const auto arg{args.GetArg("-blockmintxfee")}) {
        if (!ParseMoney(*arg)) {
            return util::Error{AmountErrMsg("blockmintxfee", *arg)};
        }
    }

    const auto max_block_weight = args.GetIntArg("-blockmaxweight", DEFAULT_BLOCK_MAX_WEIGHT);
    if (max_block_weight > MAX_BLOCK_WEIGHT) {
        return util::Error{strprintf(_("Specified -blockmaxweight (%d) exceeds consensus maximum block weight (%d)"), max_block_weight, MAX_BLOCK_WEIGHT)};
    }

    const auto block_reserved_weight = args.GetIntArg("-blockreservedweight", DEFAULT_BLOCK_RESERVED_WEIGHT);
    if (block_reserved_weight > MAX_BLOCK_WEIGHT) {
        return util::Error{strprintf(_("Specified -blockreservedweight (%d) exceeds consensus maximum block weight (%d)"), block_reserved_weight, MAX_BLOCK_WEIGHT)};
    }
    if (block_reserved_weight < MINIMUM_BLOCK_RESERVED_WEIGHT) {
        return util::Error{strprintf(_("Specified -blockreservedweight (%d) is lower than minimum safety value of (%d)"), block_reserved_weight, MINIMUM_BLOCK_RESERVED_WEIGHT)};
    }

    return {};
}

} // namespace node
