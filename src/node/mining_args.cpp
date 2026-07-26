// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mining_args.h>

#include <common/args.h>
#include <common/messages.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <node/mining_types.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <tinyformat.h>
#include <util/moneystr.h>
#include <util/result.h>
#include <util/translation.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

using common::AmountErrMsg;
using util::Error;
using util::Result;

namespace node {

Result<void> CheckMiningOptions(BlockCreateOptions options, bool use_argnames)
{
    options = FlattenMiningOptions(std::move(options));
    if (*options.block_reserved_weight < MINIMUM_BLOCK_RESERVED_WEIGHT) {
        return Error{Untranslated(strprintf("%s (%d) is lower than minimum safety value of (%d)",
                                            use_argnames ? "-blockreservedweight" : "block_reserved_weight",
                                            *options.block_reserved_weight, MINIMUM_BLOCK_RESERVED_WEIGHT))};
    }
    if (*options.block_reserved_weight > MAX_BLOCK_WEIGHT) {
        return Error{Untranslated(strprintf("%s (%d) exceeds consensus maximum block weight (%d)",
                                            use_argnames ? "-blockreservedweight" : "block_reserved_weight",
                                            *options.block_reserved_weight, MAX_BLOCK_WEIGHT))};
    }
    if (*options.block_max_weight > MAX_BLOCK_WEIGHT) {
        return Error{Untranslated(strprintf("%s (%d) exceeds consensus maximum block weight (%d)",
                                            use_argnames ? "-blockmaxweight" : "block_max_weight",
                                            *options.block_max_weight, MAX_BLOCK_WEIGHT))};
    }
    if (*options.block_reserved_weight > *options.block_max_weight) {
        return Error{Untranslated(strprintf("%s (%d) exceeds %s (%d)",
                                            use_argnames ? "-blockreservedweight" : "block_reserved_weight",
                                            *options.block_reserved_weight,
                                            use_argnames ? "-blockmaxweight" : "block_max_weight",
                                            *options.block_max_weight))};
    }
    if (options.coinbase_output_max_additional_sigops > MAX_BLOCK_SIGOPS_COST) {
        return Error{Untranslated(strprintf("%s (%zu) exceeds consensus maximum block sigops cost (%d)",
                                            "coinbase_output_max_additional_sigops",
                                            options.coinbase_output_max_additional_sigops, MAX_BLOCK_SIGOPS_COST))};
    }
    return {};
}

Result<BlockCreateOptions> ReadMiningArgs(const ArgsManager& args)
{
    BlockCreateOptions options;
    if (const auto arg{args.GetArg("-blockmintxfee")}) {
        std::optional<CAmount> block_min_tx_fee{ParseMoney(*arg)};
        if (!block_min_tx_fee) return Error{AmountErrMsg("blockmintxfee", *arg)};
        options.block_min_fee_rate = CFeeRate{*block_min_tx_fee};
    }

    if (const auto arg{args.GetBoolArg("-printpriority")}) options.print_modified_fee = *arg;

    options.block_reserved_weight = args.GetArg<uint64_t>("-blockreservedweight");
    options.block_max_weight = args.GetArg<uint64_t>("-blockmaxweight");

    if (auto result{CheckMiningOptions(options, /*use_argnames=*/true)}; !result) return Error{util::ErrorString(result)};
    return options;
}

BlockCreateOptions FlattenMiningOptions(BlockCreateOptions options)
{
    if (!options.block_min_fee_rate) options.block_min_fee_rate = CFeeRate{DEFAULT_BLOCK_MIN_TX_FEE};
    if (!options.print_modified_fee) options.print_modified_fee = DEFAULT_PRINT_MODIFIED_FEE;
    if (!options.block_reserved_weight) options.block_reserved_weight = DEFAULT_BLOCK_RESERVED_WEIGHT;
    if (!options.block_max_weight) options.block_max_weight = DEFAULT_BLOCK_MAX_WEIGHT;
    return options;
}

BlockCreateOptions MergeMiningOptions(BlockCreateOptions x, const BlockCreateOptions& y)
{
    if (!x.block_min_fee_rate) x.block_min_fee_rate = y.block_min_fee_rate;
    if (!x.print_modified_fee) x.print_modified_fee = y.print_modified_fee;
    if (!x.block_reserved_weight) x.block_reserved_weight = y.block_reserved_weight;
    if (!x.block_max_weight) x.block_max_weight = y.block_max_weight;
    return x;
}

} // namespace node
