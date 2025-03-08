// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mempool_args.h>

#include <init_settings.h>
#include <kernel/mempool_limits.h>
#include <kernel/mempool_options.h>

#include <common/args.h>
#include <common/messages.h>
#include <consensus/amount.h>
#include <kernel/chainparams.h>
#include <logging.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <tinyformat.h>
#include <util/moneystr.h>
#include <util/translation.h>

#include <chrono>
#include <memory>

using common::AmountErrMsg;
using kernel::MemPoolLimits;
using kernel::MemPoolOptions;

namespace {
void ApplyArgsManOptions(const ArgsManager& argsman, MemPoolLimits& mempool_limits)
{
    mempool_limits.ancestor_count = LimitAncestorCountSetting::Get(argsman, mempool_limits.ancestor_count);

    if (auto vkb = LimitAncestorSizeSetting::Get(argsman)) mempool_limits.ancestor_size_vbytes = *vkb * 1'000;

    mempool_limits.descendant_count = LimitDescendantCountSetting::Get(argsman, mempool_limits.descendant_count);

    if (auto vkb = LimitDescendantSizeSetting::Get(argsman)) mempool_limits.descendant_size_vbytes = *vkb * 1'000;
}
}

util::Result<void> ApplyArgsManOptions(const ArgsManager& argsman, const CChainParams& chainparams, MemPoolOptions& mempool_opts)
{
    mempool_opts.check_ratio = CheckMempoolSetting::Get(argsman, mempool_opts.check_ratio);

    if (auto mb = MaxMemPoolSetting::Get(argsman)) mempool_opts.max_size_bytes = *mb * 1'000'000;

    if (auto hours = MempoolExpirySetting::Get(argsman)) mempool_opts.expiry = std::chrono::hours{*hours};

    // incremental relay fee sets the minimum feerate increase necessary for replacement in the mempool
    // and the amount the mempool min fee increases above the feerate of txs evicted due to mempool limiting.
    if (!IncrementalRelayFeeSetting::Value(argsman).isNull()) {
        if (std::optional<CAmount> inc_relay_fee = ParseMoney(IncrementalRelayFeeSetting::Get(argsman))) {
            mempool_opts.incremental_relay_feerate = CFeeRate{inc_relay_fee.value()};
        } else {
            return util::Error{AmountErrMsg("incrementalrelayfee", IncrementalRelayFeeSetting::Get(argsman))};
        }
    }

    if (!MinRelayTxFeeSetting::Value(argsman).isNull()) {
        if (std::optional<CAmount> min_relay_feerate = ParseMoney(MinRelayTxFeeSetting::Get(argsman))) {
            // High fee check is done afterward in CWallet::Create()
            mempool_opts.min_relay_feerate = CFeeRate{min_relay_feerate.value()};
        } else {
            return util::Error{AmountErrMsg("minrelaytxfee", MinRelayTxFeeSetting::Get(argsman))};
        }
    } else if (mempool_opts.incremental_relay_feerate > mempool_opts.min_relay_feerate) {
        // Allow only setting incremental fee to control both
        mempool_opts.min_relay_feerate = mempool_opts.incremental_relay_feerate;
        LogPrintf("Increasing minrelaytxfee to %s to match incrementalrelayfee\n", mempool_opts.min_relay_feerate.ToString());
    }

    // Feerate used to define dust.  Shouldn't be changed lightly as old
    // implementations may inadvertently create non-standard transactions
    if (!DustRelayFeeSetting::Value(argsman).isNull()) {
        if (std::optional<CAmount> parsed = ParseMoney(DustRelayFeeSetting::Get(argsman))) {
            mempool_opts.dust_relay_feerate = CFeeRate{parsed.value()};
        } else {
            return util::Error{AmountErrMsg("dustrelayfee", DustRelayFeeSetting::Get(argsman))};
        }
    }

    mempool_opts.permit_bare_multisig = PermitBareMultiSigSetting::Get(argsman);

    if (DataCarrierSetting::Get(argsman)) {
        mempool_opts.max_datacarrier_bytes = DataCarrierSizeSetting::Get(argsman);
    } else {
        mempool_opts.max_datacarrier_bytes = std::nullopt;
    }

    mempool_opts.require_standard = !AcceptNonstdTxnSetting::Get(argsman);
    if (!chainparams.IsTestChain() && !mempool_opts.require_standard) {
        return util::Error{Untranslated(strprintf("acceptnonstdtxn is not currently supported for %s chain", chainparams.GetChainTypeString()))};
    }

    mempool_opts.persist_v1_dat = PersistMempoolV1Setting::Get(argsman, mempool_opts.persist_v1_dat);

    ApplyArgsManOptions(argsman, mempool_opts.limits);

    return {};
}
