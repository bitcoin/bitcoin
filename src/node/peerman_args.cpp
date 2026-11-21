// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <node/peerman_args.h>

#include <common/args.h>
#include <net_processing.h>
#include <tinyformat.h>
#include <util/result.h>
#include <util/string.h>
#include <util/translation.h>

#include <algorithm>
#include <cassert>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace node {

std::string_view StaleTipRelayModeToString(StaleTipRelayMode mode)
{
    switch (mode) {
    case StaleTipRelayMode::NONE: return "none";
    case StaleTipRelayMode::HEADERS: return "headers";
    case StaleTipRelayMode::BLOCKS: return "blocks";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

std::optional<StaleTipRelayMode> StaleTipRelayModeFromString(std::string_view name)
{
    for (const auto mode : ALL_STALETIP_RELAY_MODES) {
        if (StaleTipRelayModeToString(mode) == name) return mode;
    }
    return std::nullopt;
}

std::string ListStaleTipRelayModes()
{
    return util::Join(ALL_STALETIP_RELAY_MODES, ", ", [](StaleTipRelayMode mode) { return std::string{StaleTipRelayModeToString(mode)}; });
}

util::Result<void> ApplyArgsManOptions(const ArgsManager& argsman, PeerManager::Options& options)
{
    if (auto value{argsman.GetBoolArg("-txreconciliation")}) options.reconcile_txs = *value;

    if (argsman.IsArgNegated("-staletips")) {
        options.stale_tip_relay_mode = StaleTipRelayMode::NONE;
    } else if (auto value{argsman.GetArg("-staletips")}) {
        const auto mode{StaleTipRelayModeFromString(*value)};
        if (!mode) {
            return util::Error{strprintf(_("Invalid value for -staletips=<mode>: '%s'. Expected one of: %s."), *value, ListStaleTipRelayModes())};
        }
        options.stale_tip_relay_mode = *mode;
    }

    if (auto value{argsman.GetIntArg("-blockreconstructionextratxn")}) {
        options.max_extra_txs = uint32_t((std::clamp<int64_t>(*value, 0, std::numeric_limits<uint32_t>::max())));
    }

    if (auto value{argsman.GetBoolArg("-capturemessages")}) options.capture_messages = *value;

    if (auto value{argsman.GetBoolArg("-blocksonly")}) options.ignore_incoming_txs = *value;

    if (auto value{argsman.GetIntArg("-txsendrate")}) {
        options.tx_send_rate = uint32_t(std::clamp<int64_t>(*value, 1, 1000));
    }

    if (auto value{argsman.GetBoolArg("-privatebroadcast")}) options.private_broadcast = *value;

    return {};
}

} // namespace node

