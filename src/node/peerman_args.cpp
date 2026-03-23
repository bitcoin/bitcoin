// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <node/peerman_args.h>

#include <common/args.h>
#include <logging.h>
#include <net_processing.h>
#include <node/stdio_bus_hooks.h>
#include <util/translation.h>

#include <algorithm>
#include <limits>

namespace node {

void ApplyArgsManOptions(const ArgsManager& argsman, PeerManager::Options& options)
{
    if (auto value{argsman.GetBoolArg("-txreconciliation")}) options.reconcile_txs = *value;

    if (auto value{argsman.GetIntArg("-blockreconstructionextratxn")}) {
        options.max_extra_txs = uint32_t((std::clamp<int64_t>(*value, 0, std::numeric_limits<uint32_t>::max())));
    }

    if (auto value{argsman.GetBoolArg("-capturemessages")}) options.capture_messages = *value;

    if (auto value{argsman.GetBoolArg("-blocksonly")}) options.ignore_incoming_txs = *value;

    if (auto value{argsman.GetBoolArg("-privatebroadcast")}) options.private_broadcast = *value;

    // Parse -stdiobus mode
    if (auto value{argsman.GetArg("-stdiobus")}) {
        const std::string& mode_str = *value;
        if (mode_str == "off") {
            options.stdio_bus_mode = StdioBusMode::Off;
        } else if (mode_str == "shadow") {
            options.stdio_bus_mode = StdioBusMode::Shadow;
            LogInfo("stdio_bus enabled in shadow mode (observe only)");
        } else if (mode_str == "active") {
            options.stdio_bus_mode = StdioBusMode::Active;
            LogInfo("stdio_bus enabled in active mode");
        } else {
            // Fail-fast on unknown value - will be caught by InitError in caller
            LogError("Unknown -stdiobus mode: %s (valid: off, shadow, active)", mode_str);
        }
    }
}

} // namespace node

