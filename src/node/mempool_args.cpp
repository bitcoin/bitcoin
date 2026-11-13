// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mempool_args.h>

#include <kernel/mempool_limits.h>
#include <kernel/mempool_options.h>

#include <common/args.h>
#include <common/messages.h>
#include <consensus/amount.h>
#include <kernel/chainparams.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <tinyformat.h>
#include <txgraph.h>
#include <util/log.h>
#include <util/moneystr.h>
#include <util/translation.h>

#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>

using common::AmountErrMsg;
using kernel::MemPoolLimits;
using kernel::MemPoolOptions;

//! Maximum mempool size on 32-bit systems.
static constexpr int MAX_32BIT_MEMPOOL_MB{500};

namespace {
util::Result<void> ApplyArgsManOptions(const ArgsManager& argsman, MemPoolLimits& mempool_limits)
{
    if (auto value{argsman.GetIntArg("-limitclustercount")}) {
        // cluster_count is unsigned and TxGraph requires at least one transaction
        // per cluster, so check the range before narrowing.
        if (*value < 1 || *value > MAX_CLUSTER_COUNT_LIMIT) {
            return util::Error{Untranslated(strprintf("limitclustercount must be between 1 and %u (got %d)", MAX_CLUSTER_COUNT_LIMIT, *value))};
        }
        mempool_limits.cluster_count = static_cast<unsigned>(*value);
    }

    if (auto vkb = argsman.GetIntArg("-limitclustersize")) {
        // The value is multiplied by 1000 below and later passed to MakeTxGraph,
        // which takes a uint64_t, so a negative value would wrap to a huge limit
        // and effectively disable the cluster size limit. A zero limit is also
        // rejected: it would make every transaction exceed the cluster size and
        // prevent any normal mempool admission. Values too large to scale to
        // bytes without overflowing are rejected as well.
        constexpr int64_t MAX_CLUSTER_SIZE_KVB{std::numeric_limits<int64_t>::max() / 1'000};
        if (*vkb < 1 || *vkb > MAX_CLUSTER_SIZE_KVB) {
            return util::Error{Untranslated(strprintf("limitclustersize must be between 1 and %d kilobytes (got %d)", MAX_CLUSTER_SIZE_KVB, *vkb))};
        }
        mempool_limits.cluster_size_vbytes = *vkb * 1'000;
    }

    if (auto value{argsman.GetIntArg("-limitancestorcount")}) {
        if (*value < 0) {
            return util::Error{Untranslated(strprintf("limitancestorcount must be non-negative (got %d)", *value))};
        }
        mempool_limits.ancestor_count = *value;
    }

    if (auto value{argsman.GetIntArg("-limitdescendantcount")}) {
        if (*value < 0) {
            return util::Error{Untranslated(strprintf("limitdescendantcount must be non-negative (got %d)", *value))};
        }
        mempool_limits.descendant_count = *value;
    }
    return {};
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

    if (auto hours = argsman.GetIntArg("-mempoolexpiry")) {
        // The expiry is subtracted from the current time in validation.cpp, so a
        // negative value would move the expiration cutoff into the future and
        // stop anything from ever expiring. The value is also converted from
        // hours to seconds, which is stored in a signed 64-bit count, so reject
        // values that would overflow that conversion.
        constexpr int64_t MAX_EXPIRY_HOURS{std::chrono::duration_cast<std::chrono::hours>(std::chrono::seconds::max()).count()};
        if (*hours < 0 || *hours > MAX_EXPIRY_HOURS) {
            return util::Error{Untranslated(strprintf("-mempoolexpiry must be between 0 and %d hours (got %d).", MAX_EXPIRY_HOURS, *hours))};
        }
        mempool_opts.expiry = std::chrono::hours{*hours};
    }

    // incremental relay fee sets the minimum feerate increase necessary for replacement in the mempool
    // and the amount the mempool min fee increases above the feerate of txs evicted due to mempool limiting.
    if (const auto arg{argsman.GetArg("-incrementalrelayfee")}) {
        if (std::optional<CAmount> inc_relay_fee = ParseMoney(*arg)) {
            mempool_opts.incremental_relay_feerate = CFeeRate{inc_relay_fee.value()};
        } else {
            return util::Error{AmountErrMsg("incrementalrelayfee", *arg)};
        }
    }

    static_assert(DEFAULT_MIN_RELAY_TX_FEE == DEFAULT_INCREMENTAL_RELAY_FEE);
    if (const auto arg{argsman.GetArg("-minrelaytxfee")}) {
        if (std::optional<CAmount> min_relay_feerate = ParseMoney(*arg)) {
            // High fee check is done afterward in CWallet::Create()
            mempool_opts.min_relay_feerate = CFeeRate{min_relay_feerate.value()};
        } else {
            return util::Error{AmountErrMsg("minrelaytxfee", *arg)};
        }
    } else if (mempool_opts.incremental_relay_feerate > mempool_opts.min_relay_feerate) {
        // Allow only setting incremental fee to control both
        mempool_opts.min_relay_feerate = mempool_opts.incremental_relay_feerate;
        LogInfo("Increasing minrelaytxfee to %s to match incrementalrelayfee", mempool_opts.min_relay_feerate.ToString());
    }

    // Feerate used to define dust.  Shouldn't be changed lightly as old
    // implementations may inadvertently create non-standard transactions
    if (const auto arg{argsman.GetArg("-dustrelayfee")}) {
        if (std::optional<CAmount> parsed = ParseMoney(*arg)) {
            mempool_opts.dust_relay_feerate = CFeeRate{parsed.value()};
        } else {
            return util::Error{AmountErrMsg("dustrelayfee", *arg)};
        }
    }

    mempool_opts.permit_bare_multisig = argsman.GetBoolArg("-permitbaremultisig", DEFAULT_PERMIT_BAREMULTISIG);

    if (argsman.GetBoolArg("-datacarrier", DEFAULT_ACCEPT_DATACARRIER)) {
        if (auto bytes{argsman.GetIntArg("-datacarriersize")}) {
            // max_datacarrier_bytes is unsigned, so an out-of-range value would
            // wrap: negative to a huge budget that accepts any amount of
            // OP_RETURN data, and above UINT_MAX to a smaller one.
            if (*bytes < 0 || static_cast<uint64_t>(*bytes) > std::numeric_limits<unsigned>::max()) {
                return util::Error{Untranslated(strprintf("-datacarriersize must be between 0 and %u (got %d).", std::numeric_limits<unsigned>::max(), *bytes))};
            }
            mempool_opts.max_datacarrier_bytes = static_cast<unsigned>(*bytes);
        } else {
            mempool_opts.max_datacarrier_bytes = MAX_OP_RETURN_RELAY;
        }
    } else {
        mempool_opts.max_datacarrier_bytes = std::nullopt;
    }

    mempool_opts.require_standard = !argsman.GetBoolArg("-acceptnonstdtxn", DEFAULT_ACCEPT_NON_STD_TXN);
    if (!chainparams.IsTestChain() && !mempool_opts.require_standard) {
        return util::Error{Untranslated(strprintf("acceptnonstdtxn is not currently supported for %s chain", chainparams.GetChainTypeString()))};
    }

    if (auto result{ApplyArgsManOptions(argsman, mempool_opts.limits)}; !result) return util::Error{util::ErrorString(result)};

    return {};
}
