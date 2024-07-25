// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/messages.h>
#include <core_io.h>
#include <node/context.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <txmempool.h>
#include <univalue.h>
#include <validationinterface.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

using common::FeeModeFromString;
using common::FeeModes;
using common::InvalidEstimateModeErrorMessage;
using node::NodeContext;

static RPCHelpMan estimatesmartfee()
{
    return RPCHelpMan{"estimatesmartfee",
        "\nEstimates the approximate fee per kilobyte needed for a transaction to begin\n"
        "confirmation within conf_target blocks if possible and return the number of blocks\n"
        "for which the estimate is valid. Uses virtual transaction size as defined\n"
        "in BIP 141 (witness data is discounted).\n",
        {
            {"conf_target", RPCArg::Type::NUM, RPCArg::Optional::NO, "Confirmation target in blocks (1 - 1008)"},
            {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"economical"}, "The fee estimate mode.\n"
            "Whether to return a more conservative estimate which also satisfies\n"
            "a longer history. A conservative estimate potentially returns a\n"
            "higher feerate and is more likely to be sufficient for the desired\n"
            "target, but is not as responsive to short term drops in the\n"
            "prevailing fee market. Must be one of (case insensitive):\n"
             "\"" + FeeModes("\"\n\"") + "\""},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "feerate", /*optional=*/true, "estimate fee rate in " + CURRENCY_UNIT + "/kvB (only present if no errors were encountered)"},
                {RPCResult::Type::ARR, "errors", /*optional=*/true, "Errors encountered during processing (if there are any)",
                    {
                        {RPCResult::Type::STR, "", "error"},
                    }},
                {RPCResult::Type::NUM, "blocks", "block number where estimate was found\n"
                "The request target will be clamped between 2 and the highest target\n"
                "fee estimation is able to return based on how long it has been running.\n"
                "An error is returned if not enough transactions and blocks\n"
                "have been observed to make an estimate for any number of blocks."},
        }},
        RPCExamples{
            HelpExampleCli("estimatesmartfee", "6") +
            HelpExampleRpc("estimatesmartfee", "6")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            CBlockPolicyEstimator& fee_estimator = EnsureAnyFeeEstimator(request.context);
            const NodeContext& node = EnsureAnyNodeContext(request.context);
            const CTxMemPool& mempool = EnsureMemPool(node);

            CHECK_NONFATAL(mempool.m_opts.signals)->SyncWithValidationInterfaceQueue();
            unsigned int max_target = fee_estimator.HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE);
            unsigned int conf_target = ParseConfirmTarget(request.params[0], max_target);
            bool conservative = false;
            if (!request.params[1].isNull()) {
                FeeEstimateMode fee_mode;
                if (!FeeModeFromString(request.params[1].get_str(), fee_mode)) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, InvalidEstimateModeErrorMessage());
                }
                if (fee_mode == FeeEstimateMode::CONSERVATIVE) conservative = true;
            }

            UniValue result(UniValue::VOBJ);
            UniValue errors(UniValue::VARR);
            FeeCalculation feeCalc;
            CFeeRate feeRate{fee_estimator.estimateSmartFee(conf_target, &feeCalc, conservative)};
            if (feeRate != CFeeRate(0)) {
                CFeeRate min_mempool_feerate{mempool.GetMinFee()};
                CFeeRate min_relay_feerate{mempool.m_opts.min_relay_feerate};
                feeRate = std::max({feeRate, min_mempool_feerate, min_relay_feerate});
                result.pushKV("feerate", ValueFromAmount(feeRate.GetFeePerK()));
            } else {
                errors.push_back("Insufficient data or no feerate found");
                result.pushKV("errors", std::move(errors));
            }
            result.pushKV("blocks", feeCalc.returnedTarget);
            return result;
        },
    };
}

static RPCHelpMan estimaterawfee()
{
    return RPCHelpMan{"estimaterawfee",
        "\nWARNING: This interface is unstable and may disappear or change!\n"
        "\nWARNING: This is an advanced API call that is tightly coupled to the specific\n"
        "implementation of fee estimation. The parameters it can be called with\n"
        "and the results it returns will change if the internal implementation changes.\n"
        "\nEstimates the approximate fee per kilobyte needed for a transaction to begin\n"
        "confirmation within conf_target blocks if possible. Uses virtual transaction size as\n"
        "defined in BIP 141 (witness data is discounted).\n",
        {
            {"conf_target", RPCArg::Type::NUM, RPCArg::Optional::NO, "Confirmation target in blocks (1 - 1008)"},
            {"threshold", RPCArg::Type::NUM, RPCArg::Default{0.95}, "The proportion of transactions in a given feerate range that must have been\n"
            "confirmed within conf_target in order to consider those feerates as high enough and proceed to check\n"
            "lower buckets."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "Results are returned for any horizon which tracks blocks up to the confirmation target",
            {
                {RPCResult::Type::OBJ, "short", /*optional=*/true, "estimate for short time horizon",
                    {
                        {RPCResult::Type::NUM, "feerate", /*optional=*/true, "estimate fee rate in " + CURRENCY_UNIT + "/kvB"},
                        {RPCResult::Type::NUM, "decay", "exponential decay (per block) for historical moving average of confirmation data"},
                        {RPCResult::Type::NUM, "scale", "The resolution of confirmation targets at this time horizon"},
                        {RPCResult::Type::OBJ, "pass", /*optional=*/true, "information about the lowest range of feerates to succeed in meeting the threshold",
                        {
                                {RPCResult::Type::NUM, "startrange", "start of feerate range"},
                                {RPCResult::Type::NUM, "endrange", "end of feerate range"},
                                {RPCResult::Type::NUM, "withintarget", "number of txs over history horizon in the feerate range that were confirmed within target"},
                                {RPCResult::Type::NUM, "totalconfirmed", "number of txs over history horizon in the feerate range that were confirmed at any point"},
                                {RPCResult::Type::NUM, "inmempool", "current number of txs in mempool in the feerate range unconfirmed for at least target blocks"},
                                {RPCResult::Type::NUM, "leftmempool", "number of txs over history horizon in the feerate range that left mempool unconfirmed after target"},
                        }},
                        {RPCResult::Type::OBJ, "fail", /*optional=*/true, "information about the highest range of feerates to fail to meet the threshold",
                        {
                            {RPCResult::Type::ELISION, "", ""},
                        }},
                        {RPCResult::Type::ARR, "errors", /*optional=*/true, "Errors encountered during processing (if there are any)",
                        {
                            {RPCResult::Type::STR, "error", ""},
                        }},
                }},
                {RPCResult::Type::OBJ, "medium", /*optional=*/true, "estimate for medium time horizon",
                {
                    {RPCResult::Type::ELISION, "", ""},
                }},
                {RPCResult::Type::OBJ, "long", /*optional=*/true, "estimate for long time horizon",
                {
                    {RPCResult::Type::ELISION, "", ""},
                }},
            }},
        RPCExamples{
            HelpExampleCli("estimaterawfee", "6 0.9")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            CBlockPolicyEstimator& fee_estimator = EnsureAnyFeeEstimator(request.context);
            const NodeContext& node = EnsureAnyNodeContext(request.context);

            CHECK_NONFATAL(node.validation_signals)->SyncWithValidationInterfaceQueue();
            unsigned int max_target = fee_estimator.HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE);
            unsigned int conf_target = ParseConfirmTarget(request.params[0], max_target);
            double threshold = 0.95;
            if (!request.params[1].isNull()) {
                threshold = request.params[1].get_real();
            }
            if (threshold < 0 || threshold > 1) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid threshold");
            }

            UniValue result(UniValue::VOBJ);

            for (const FeeEstimateHorizon horizon : ALL_FEE_ESTIMATE_HORIZONS) {
                CFeeRate feeRate;
                EstimationResult buckets;

                // Only output results for horizons which track the target
                if (conf_target > fee_estimator.HighestTargetTracked(horizon)) continue;

                feeRate = fee_estimator.estimateRawFee(conf_target, threshold, horizon, &buckets);
                UniValue horizon_result(UniValue::VOBJ);
                UniValue errors(UniValue::VARR);
                UniValue passbucket(UniValue::VOBJ);
                passbucket.pushKV("startrange", round(buckets.pass.start));
                passbucket.pushKV("endrange", round(buckets.pass.end));
                passbucket.pushKV("withintarget", round(buckets.pass.withinTarget * 100.0) / 100.0);
                passbucket.pushKV("totalconfirmed", round(buckets.pass.totalConfirmed * 100.0) / 100.0);
                passbucket.pushKV("inmempool", round(buckets.pass.inMempool * 100.0) / 100.0);
                passbucket.pushKV("leftmempool", round(buckets.pass.leftMempool * 100.0) / 100.0);
                UniValue failbucket(UniValue::VOBJ);
                failbucket.pushKV("startrange", round(buckets.fail.start));
                failbucket.pushKV("endrange", round(buckets.fail.end));
                failbucket.pushKV("withintarget", round(buckets.fail.withinTarget * 100.0) / 100.0);
                failbucket.pushKV("totalconfirmed", round(buckets.fail.totalConfirmed * 100.0) / 100.0);
                failbucket.pushKV("inmempool", round(buckets.fail.inMempool * 100.0) / 100.0);
                failbucket.pushKV("leftmempool", round(buckets.fail.leftMempool * 100.0) / 100.0);

                // CFeeRate(0) is used to indicate error as a return value from estimateRawFee
                if (feeRate != CFeeRate(0)) {
                    horizon_result.pushKV("feerate", ValueFromAmount(feeRate.GetFeePerK()));
                    horizon_result.pushKV("decay", buckets.decay);
                    horizon_result.pushKV("scale", (int)buckets.scale);
                    horizon_result.pushKV("pass", std::move(passbucket));
                    // buckets.fail.start == -1 indicates that all buckets passed, there is no fail bucket to output
                    if (buckets.fail.start != -1) horizon_result.pushKV("fail", std::move(failbucket));
                } else {
                    // Output only information that is still meaningful in the event of error
                    horizon_result.pushKV("decay", buckets.decay);
                    horizon_result.pushKV("scale", (int)buckets.scale);
                    horizon_result.pushKV("fail", std::move(failbucket));
                    errors.push_back("Insufficient data or no feerate found which meets threshold");
                    horizon_result.pushKV("errors", std::move(errors));
                }
                result.pushKV(StringForFeeEstimateHorizon(horizon), std::move(horizon_result));
            }
            return result;
        },
    };
}

void RegisterFeeRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"util", &estimatesmartfee},
        {"hidden", &estimaterawfee},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
