// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#ifndef BITCOIN_NODE_PEERMAN_ARGS_H
#define BITCOIN_NODE_PEERMAN_ARGS_H

#include <net_processing.h>
#include <util/result.h>

#include <optional>
#include <string>
#include <string_view>

class ArgsManager;

namespace node {
std::string_view StaleTipRelayModeToString(StaleTipRelayMode mode);
std::optional<StaleTipRelayMode> StaleTipRelayModeFromString(std::string_view name);
/** Comma-separated list of the -staletips modes. */
std::string ListStaleTipRelayModes();

util::Result<void> ApplyArgsManOptions(const ArgsManager& argsman, PeerManager::Options& options);
} // namespace node

#endif // BITCOIN_NODE_PEERMAN_ARGS_H
