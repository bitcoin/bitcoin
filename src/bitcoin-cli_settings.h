// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BITCOIN_CLI_SETTINGS_H
#define BITCOIN_BITCOIN_CLI_SETTINGS_H

#include <cstdint>
#include <string>

static const char DEFAULT_RPCCONNECT[] = "127.0.0.1";
static const int DEFAULT_HTTP_CLIENT_TIMEOUT=900;
static constexpr int DEFAULT_WAIT_CLIENT_TIMEOUT = 0;
static const bool DEFAULT_NAMED=false;
static constexpr uint8_t NETINFO_MAX_LEVEL{4};

/** Default number of blocks to generate for RPC generatetoaddress. */
static constexpr auto DEFAULT_NBLOCKS = "1";

/** Default -color setting. */
static constexpr auto DEFAULT_COLOR_SETTING{"auto"};

#endif // BITCOIN_BITCOIN_CLI_SETTINGS_H
