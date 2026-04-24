// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mining_args.h>

#include <common/args.h>
#include <common/messages.h>
#include <node/mining_types.h>
#include <util/moneystr.h>
#include <util/translation.h>

using common::AmountErrMsg;

namespace node {

util::Result<void> ReadMiningArgs(const ArgsManager& args, MiningArgs& mining_args)
{
    if (const auto arg{args.GetArg("-blockmintxfee")}) {
        std::optional<CAmount> block_min_tx_fee{ParseMoney(*arg)};
        if (!block_min_tx_fee) return util::Error{AmountErrMsg("blockmintxfee", *arg)};
        mining_args.blockMinFeeRate = CFeeRate{*block_min_tx_fee};
    }

    const size_t max_block_weight = args.GetIntArg("-blockmaxweight", DEFAULT_BLOCK_MAX_WEIGHT);
    if (auto result{CheckBlockMaxWeight(max_block_weight)}; !result) {
        return util::Error{Untranslated("Specified -blockmaxweight " + util::ErrorString(result).original)};
    }
    mining_args.default_block_max_weight = max_block_weight;

    const size_t block_reserved_weight = args.GetIntArg("-blockreservedweight", DEFAULT_BLOCK_RESERVED_WEIGHT);
    if (auto result{CheckBlockReservedWeight(block_reserved_weight)}; !result) {
        return util::Error{Untranslated("Specified -blockreservedweight " + util::ErrorString(result).original)};
    }
    mining_args.default_block_reserved_weight = block_reserved_weight;

    mining_args.print_modified_fee = args.GetBoolArg("-printpriority", mining_args.print_modified_fee);

    return {};
}

} // namespace node
