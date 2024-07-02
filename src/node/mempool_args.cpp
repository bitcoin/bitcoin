// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mempool_args.h>

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
    mempool_limits.cluster_count = argsman.GetIntArg("-limitclustercount", mempool_limits.cluster_count);

    if (auto vkb = argsman.GetIntArg("-limitclustersize")) mempool_limits.cluster_size_vbytes = *vkb * 1'000;

    mempool_limits.ancestor_count = argsman.GetIntArg("-limitancestorcount", mempool_limits.ancestor_count);

    if (auto vkb = argsman.GetIntArg("-limitancestorsize")) mempool_limits.ancestor_size_vbytes = *vkb * 1'000;

    mempool_limits.descendant_count = argsman.GetIntArg("-limitdescendantcount", mempool_limits.descendant_count);

    if (auto vkb = argsman.GetIntArg("-limitdescendantsize")) mempool_limits.descendant_size_vbytes = *vkb * 1'000;
}
}

util::Result<void> ApplyArgsManOptions(const ArgsManager& argsman, const CChainParams& chainparams, MemPoolOptions& mempool_opts)
{
    mempool_opts.check_ratio = argsman.GetIntArg("-checkmempool", mempool_opts.check_ratio);

    if (auto mb = argsman.GetIntArg("-maxmempool")) mempool_opts.max_size_bytes = *mb * 1'000'000;

    if (auto hours = argsman.GetIntArg("-mempoolexpiry")) mempool_opts.expiry = std::chrono::hours{*hours};

    // incremental relay fee sets the minimum feerate increase necessary for replacement in the mempool
    // and the amount the mempool min fee increases above the feerate of txs evicted due to mempool limiting.
    if (argsman.IsArgSet("-incrementalrelayfee")) {
        if (std::optional<CAmount> inc_relay_fee = ParseMoney(argsman.GetArg("-incrementalrelayfee", ""))) {
            mempool_opts.incremental_relay_feerate = CFeeRate{inc_relay_fee.value()};
        } else {
            return util::Error{AmountErrMsg("incrementalrelayfee", argsman.GetArg("-incrementalrelayfee", ""))};
        }
    }

    if (argsman.IsArgSet("-minrelaytxfee")) {
        if (std::optional<CAmount> min_relay_feerate = ParseMoney(argsman.GetArg("-minrelaytxfee", ""))) {
            // High fee check is done afterward in CWallet::Create()
            mempool_opts.min_relay_feerate = CFeeRate{min_relay_feerate.value()};
        } else {
            return util::Error{AmountErrMsg("minrelaytxfee", argsman.GetArg("-minrelaytxfee", ""))};
        }
    } else if (mempool_opts.incremental_relay_feerate > mempool_opts.min_relay_feerate) {
        // Allow only setting incremental fee to control both
        mempool_opts.min_relay_feerate = mempool_opts.incremental_relay_feerate;
        LogPrintf("Increasing minrelaytxfee to %s to match incrementalrelayfee\n", mempool_opts.min_relay_feerate.ToString());
    }

    // Feerate used to define dust.  Shouldn't be changed lightly as old
    // implementations may inadvertently create non-standard transactions
    if (argsman.IsArgSet("-dustrelayfee")) {
        if (std::optional<CAmount> parsed = ParseMoney(argsman.GetArg("-dustrelayfee", ""))) {
            mempool_opts.dust_relay_feerate = CFeeRate{parsed.value()};
        } else {
            return util::Error{AmountErrMsg("dustrelayfee", argsman.GetArg("-dustrelayfee", ""))};
        }
    }

    mempool_opts.permit_bare_multisig = argsman.GetBoolArg("-permitbaremultisig", DEFAULT_PERMIT_BAREMULTISIG);

    if (argsman.GetBoolArg("-datacarrier", DEFAULT_ACCEPT_DATACARRIER)) {
        mempool_opts.max_datacarrier_bytes = argsman.GetIntArg("-datacarriersize", MAX_OP_RETURN_RELAY);
    } else {
        mempool_opts.max_datacarrier_bytes = std::nullopt;
    }

    mempool_opts.require_standard = !argsman.GetBoolArg("-acceptnonstdtxn", DEFAULT_ACCEPT_NON_STD_TXN);
    if (!chainparams.IsTestChain() && !mempool_opts.require_standard) {
        return util::Error{strprintf(Untranslated("acceptnonstdtxn is not currently supported for %s chain"), chainparams.GetChainTypeString())};
    }

    mempool_opts.full_rbf = argsman.GetBoolArg("-mempoolfullrbf", mempool_opts.full_rbf);

    mempool_opts.persist_v1_dat = argsman.GetBoolArg("-persistmempoolv1", mempool_opts.persist_v1_dat);

    ApplyArgsManOptions(argsman, mempool_opts.limits);

    return {};
}
