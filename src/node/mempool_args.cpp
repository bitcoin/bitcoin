// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mempool_args.h>

#include <kernel/mempool_limits.h>
#include <kernel/mempool_options.h>

#include <common/args.h>
#include <common/messages.h>
#include <common/settings.h>
#include <consensus/amount.h>
#include <kernel/chainparams.h>
#include <logging.h>
#include <node/interface_ui.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/moneystr.h>
#include <util/result.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/translation.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>

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

util::Result<std::pair<int32_t, int>> ParseDustDynamicOpt(std::string_view optstr, const unsigned int max_fee_estimate_blocks)
{
    if (optstr == "0" || optstr == "off") {
        return std::pair<int32_t, int>(0, DEFAULT_DUST_RELAY_MULTIPLIER);
    }

    int multiplier{DEFAULT_DUST_RELAY_MULTIPLIER};
    if (auto pos = optstr.find('*'); pos != optstr.npos) {
        int64_t parsed;
        if ((!ParseFixedPoint(optstr.substr(0, pos), 3, &parsed)) || parsed > std::numeric_limits<int>::max() || parsed < 1) {
            return util::Error{_("failed to parse multiplier")};
        }
        multiplier = parsed;
        optstr.remove_prefix(pos + 1);
    }

    if (optstr.rfind("target:", 0) == 0) {
        const auto val = ToIntegral<uint16_t>(optstr.substr(7));
        if (!val) {
            return util::Error{_("failed to parse target block count")};
        }
        if (*val < 2) {
            return util::Error{_("target must be at least 2 blocks")};
        }
        if (Assume(max_fee_estimate_blocks) && *val > max_fee_estimate_blocks) {
            return util::Error{strprintf(_("target can only be at most %s blocks"), max_fee_estimate_blocks)};
        }
        return std::pair<int32_t, int>(-*val, multiplier);
    } else if (optstr.rfind("mempool:", 0) == 0) {
        const auto val = ToIntegral<int32_t>(optstr.substr(8));
        if (!val) {
            return util::Error{_("failed to parse mempool position")};
        }
        if (*val < 1) {
            return util::Error{_("mempool position must be at least 1 kB")};
        }
        return std::pair<int32_t, int>(*val, multiplier);
    } else {
        return util::Error{strprintf(_("\"%s\""), optstr)};
    }
}

void ApplyPermitEphemeralOption(const common::SettingsValue& value, MemPoolOptions& mempool_opts)
{
    std::optional<bool> flag_anchor, flag_send, flag_dust;
    if (SettingToBool(value, false)) {
        flag_anchor = flag_send = flag_dust = true;
    }
    for (auto& opt : util::SplitString(SettingToString(value).value_or(""), ",+")) {
        bool v{true};
        if (opt.size() && opt[0] == '-') {
            opt.erase(opt.begin());
            v = false;
        }
        if (opt == "anchor") {
            flag_anchor = v;
        } else if (opt == "dust") {
            flag_dust = v;
        } else if (opt == "send") {
            flag_send = v;
        } else if (opt == "reject" || opt == "0") {
            flag_anchor = flag_send = flag_dust = !v;
        }
    }

    if (!flag_send) {
        flag_send = flag_dust.value_or(false) && !flag_anchor.value_or(false);
    }
    if (!flag_dust) {
        flag_dust = flag_send;
    }
    if (!flag_anchor) {
        flag_anchor = true;
    }

    mempool_opts.permitephemeral_dust = *flag_dust;
    mempool_opts.permitephemeral_anchor = *flag_anchor;
    mempool_opts.permitephemeral_send = *flag_send;
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
    if (argsman.IsArgSet("-dustdynamic")) {
        const auto optstr = argsman.GetArg("-dustdynamic", DEFAULT_DUST_DYNAMIC);
        // TODO: Should probably reject target-based dustdynamic if there's no estimator, but currently we're checking parameters long before we have the fee estimator initialised
        const auto max_fee_estimate_blocks = mempool_opts.estimator ? mempool_opts.estimator->HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE) : (CBlockPolicyEstimator::LONG_BLOCK_PERIODS * CBlockPolicyEstimator::LONG_SCALE);
        const auto parsed = ParseDustDynamicOpt(optstr, max_fee_estimate_blocks);
        if (!parsed) {
            return util::Error{strprintf(_("Invalid mode for dustdynamic: %s"), util::ErrorString(parsed))};
        }
        mempool_opts.dust_relay_target = parsed->first;
        mempool_opts.dust_relay_multiplier = parsed->second;
    }

    mempool_opts.permitbareanchor = argsman.GetBoolArg("-permitbareanchor", mempool_opts.permitbareanchor);

    mempool_opts.permit_bare_pubkey = argsman.GetBoolArg("-permitbarepubkey", DEFAULT_PERMIT_BAREPUBKEY);

    mempool_opts.permit_bare_multisig = argsman.GetBoolArg("-permitbaremultisig", DEFAULT_PERMIT_BAREMULTISIG);

    if (argsman.GetBoolArg("-datacarrier", DEFAULT_ACCEPT_DATACARRIER)) {
        mempool_opts.max_datacarrier_bytes = argsman.GetIntArg("-datacarriersize", MAX_OP_RETURN_RELAY);
    } else {
        mempool_opts.max_datacarrier_bytes = std::nullopt;
    }
    mempool_opts.datacarrier_fullcount = argsman.GetBoolArg("-datacarrierfullcount", DEFAULT_DATACARRIER_FULLCOUNT);

    mempool_opts.permitbaredatacarrier = argsman.GetBoolArg("-permitbaredatacarrier", mempool_opts.permitbaredatacarrier);

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

    if (argsman.IsArgSet("-mempooltruc")) {
        std::optional<bool> accept_flag, enforce_flag;
        if (argsman.GetBoolArg("-mempooltruc", false)) {
            enforce_flag = true;
        }
        for (auto& opt : util::SplitString(argsman.GetArg("-mempooltruc", ""), ",+")) {
            if (opt == "optin" || opt == "enforce") {
                enforce_flag = true;
            } else if (opt == "-optin" || opt == "-enforce") {
                enforce_flag = false;
            } else if (opt == "accept") {
                accept_flag = true;
            } else if (opt == "reject" || opt == "0") {
                accept_flag = false;
            }
        }

        if (accept_flag && !*accept_flag) {  // reject
            mempool_opts.truc_policy = TRUCPolicy::Reject;
        } else if (enforce_flag && *enforce_flag) {  // enforce
            mempool_opts.truc_policy = TRUCPolicy::Enforce;
        } else if ((!accept_flag) && !enforce_flag) {
            // nothing specified, leave at default
        } else {  // accept or -enforce
            mempool_opts.truc_policy = TRUCPolicy::Accept;
        }
    }

    if (argsman.IsArgSet("-permitephemeral")) {
        ApplyPermitEphemeralOption(argsman.GetSetting("-permitephemeral"), mempool_opts);
    }

    mempool_opts.persist_v1_dat = argsman.GetBoolArg("-persistmempoolv1", mempool_opts.persist_v1_dat);

    ApplyArgsManOptions(argsman, mempool_opts.limits);

    return {};
}
