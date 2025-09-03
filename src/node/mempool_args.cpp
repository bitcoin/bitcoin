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
#include <node/interface_ui.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <tinyformat.h>
#include <util/moneystr.h>
#include <util/string.h>
#include <util/translation.h>

#include <chrono>
#include <memory>

using common::AmountErrMsg;
using kernel::MemPoolLimits;
using kernel::MemPoolOptions;

//! Maximum mempool size on 32-bit systems.
static constexpr int MAX_32BIT_MEMPOOL_MB{500};

namespace {
void ApplyArgsManOptions(const ArgsManager& argsman, MemPoolLimits& mempool_limits)
{
    mempool_limits.ancestor_count = argsman.GetIntArg("-limitancestorcount", mempool_limits.ancestor_count);

    if (auto vkb = argsman.GetIntArg("-limitancestorsize")) mempool_limits.ancestor_size_vbytes = *vkb * 1'000;

    mempool_limits.descendant_count = argsman.GetIntArg("-limitdescendantcount", mempool_limits.descendant_count);

    if (auto vkb = argsman.GetIntArg("-limitdescendantsize")) mempool_limits.descendant_size_vbytes = *vkb * 1'000;
}
}

util::Result<void> ApplyArgsManOptions(const ArgsManager& argsman, const CChainParams& chainparams, MemPoolOptions& mempool_opts)
{
    mempool_opts.check_ratio = argsman.GetIntArg("-checkmempool", mempool_opts.check_ratio);

    if (auto mb = argsman.GetIntArg("-maxmempool")) {
        constexpr bool is_32bit{sizeof(void*) == 4};
        if (is_32bit && *mb > MAX_32BIT_MEMPOOL_MB) {
            return util::Error{Untranslated(strprintf("-maxmempool is set to %i but can't be over %i MB on 32-bit systems", *mb, MAX_32BIT_MEMPOOL_MB))};
        }
        mempool_opts.max_size_bytes = *mb * 1'000'000;
    }

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

    static_assert(DEFAULT_MIN_RELAY_TX_FEE == DEFAULT_INCREMENTAL_RELAY_FEE);
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

    mempool_opts.acceptunknownwitness = argsman.GetBoolArg("-acceptunknownwitness", mempool_opts.acceptunknownwitness);

    if (argsman.IsArgSet("-mempoolreplacement") || argsman.IsArgSet("-mempoolfullrbf")) {
        // Generally, mempoolreplacement overrides mempoolfullrbf, but the latter is used to infer intent in some cases
        std::optional<bool> optin_flag;
        bool fee_flag{false};
        if (argsman.GetBoolArg("-mempoolreplacement", false)) {
            fee_flag = true;
        } else {
            for (auto& opt : util::SplitString(argsman.GetArg("-mempoolreplacement", ""), ",+")) {
                if (opt == "optin") {
                    optin_flag = true;
                } else if (opt == "-optin") {
                    optin_flag = false;
                } else if (opt == "fee") {
                    fee_flag = true;
                }
            }
        }
        if (optin_flag.value_or(false)) {
            // "optin" is explicitly specified
            mempool_opts.rbf_policy = RBFPolicy::OptIn;
        } else if (argsman.GetBoolArg("-mempoolfullrbf", false)) {
            const bool mempoolreplacement_false{argsman.IsArgSet("-mempoolreplacement") && !(fee_flag || optin_flag.has_value())};
            if (mempoolreplacement_false) {
                // This is a contradiction, but override rather than error
                InitWarning(_("False mempoolreplacement option contradicts true mempoolfullrbf; disallowing all RBF"));
                mempool_opts.rbf_policy = RBFPolicy::Never;
            } else {
                mempool_opts.rbf_policy = RBFPolicy::Always;
            }
        } else if (!optin_flag.value_or(true)) {
            // "-optin" is explicitly specified
            mempool_opts.rbf_policy = fee_flag ? RBFPolicy::Always : RBFPolicy::Never;
        } else if (fee_flag) {
            // Just "fee" by itself
            if (!argsman.GetBoolArg("-mempoolfullrbf", true)) {
                mempool_opts.rbf_policy = RBFPolicy::OptIn;
            } else {
                // Fallback to default, unless it's been changed to Never
                if (mempool_opts.rbf_policy == RBFPolicy::Never) {
                    mempool_opts.rbf_policy = RBFPolicy::Always;
                }
            }
        } else if (!argsman.IsArgSet("-mempoolreplacement")) {
            // mempoolfullrbf is always explicitly false here
            // Fallback to default, as long as it isn't Always
            if (mempool_opts.rbf_policy == RBFPolicy::Always) {
                mempool_opts.rbf_policy = RBFPolicy::OptIn;
            }
        } else {
            // mempoolreplacement is explicitly false here
            mempool_opts.rbf_policy = RBFPolicy::Never;
        }
    }

    mempool_opts.persist_v1_dat = argsman.GetBoolArg("-persistmempoolv1", mempool_opts.persist_v1_dat);

    ApplyArgsManOptions(argsman, mempool_opts.limits);

    return {};
}
