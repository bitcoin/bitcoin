// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BITCOIN_CLI_SETTINGS_H
#define BITCOIN_BITCOIN_CLI_SETTINGS_H

#include <common/args.h>
#include <common/setting.h>
#include <rpc/mining.h>

#include <cstdint>
#include <string>
#include <vector>

static const char DEFAULT_RPCCONNECT[] = "127.0.0.1";
static const int DEFAULT_HTTP_CLIENT_TIMEOUT=900;
static constexpr int DEFAULT_WAIT_CLIENT_TIMEOUT = 0;
static const bool DEFAULT_NAMED=false;
static constexpr uint8_t NETINFO_MAX_LEVEL{4};

/** Default number of blocks to generate for RPC generatetoaddress. */
static constexpr auto DEFAULT_NBLOCKS = "1";

/** Default -color setting. */
static constexpr auto DEFAULT_COLOR_SETTING{"auto"};

using VersionSetting = common::Setting<
    "-version", bool, common::SettingOptions{.legacy = true},
    "Print version and exit">;

using ConfSetting = common::Setting<
    "-conf=<file>", common::Unset, common::SettingOptions{.legacy = true},
    "Specify configuration file. Relative paths will be prefixed by datadir location. (default: %s)">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, BITCOIN_CONF_FILENAME); }>;

using DatadirSetting = common::Setting<
    "-datadir=<dir>", std::string, common::SettingOptions{.legacy = true, .disallow_negation = true},
    "Specify data directory">;

using GenerateSetting = common::Setting<
    "-generate", bool, common::SettingOptions{.legacy = true},
    "Generate blocks, equivalent to RPC getnewaddress followed by RPC generatetoaddress. Optional positional integer "
                             "arguments are number of blocks to generate (default: %s) and maximum iterations to try (default: %s), equivalent to "
                             "RPC generatetoaddress nblocks and maxtries arguments. Example: bitcoin-cli -generate 4 1000">
    ::HelpFn<[](const auto& fmt) { return strprintf(fmt, DEFAULT_NBLOCKS, DEFAULT_MAX_TRIES); }>
    ::Category<OptionsCategory::CLI_COMMANDS>;

using AddrinfoSetting = common::Setting<
    "-addrinfo", bool, common::SettingOptions{.legacy = true},
    "Get the number of addresses known to the node, per network and total, after filtering for quality and recency. The total number of addresses known to the node may be higher.">
    ::Category<OptionsCategory::CLI_COMMANDS>;

using GetinfoSetting = common::Setting<
    "-getinfo", bool, common::SettingOptions{.legacy = true},
    "Get general information from the remote server. Note that unlike server-side RPC calls, the output of -getinfo is the result of multiple non-atomic requests. Some entries in the output may represent results from different states (e.g. wallet balance may be as of a different block from the chain state reported)">
    ::Category<OptionsCategory::CLI_COMMANDS>;

using NetinfoSetting = common::Setting<
    "-netinfo", bool, common::SettingOptions{.legacy = true},
    "Get network peer connection information from the remote server. An optional argument from 0 to %d can be passed for different peers listings (default: 0). If a non-zero value is passed, an additional \"outonly\" (or \"o\") argument can be passed to see outbound peers only. Pass \"help\" (or \"h\") for detailed help documentation.">
    ::HelpArgs<NETINFO_MAX_LEVEL>
    ::Category<OptionsCategory::CLI_COMMANDS>;

using ColorSetting = common::Setting<
    "-color=<when>", std::string, common::SettingOptions{.legacy = true, .disallow_negation = true},
    "Color setting for CLI output (default: %s). Valid values: always, auto (add color codes when standard output is connected to a terminal and OS is not WIN32), never. Only applies to the output of -getinfo.">
    ::DefaultFn<[] { return DEFAULT_COLOR_SETTING; }>;

using NamedSetting = common::Setting<
    "-named", bool, common::SettingOptions{.legacy = true},
    "Pass named instead of positional arguments (default: %s)">
    ::Default<DEFAULT_NAMED>;

using RpcclienttimeoutSetting = common::Setting<
    "-rpcclienttimeout=<n>", int64_t, common::SettingOptions{.legacy = true},
    "Timeout in seconds during HTTP requests, or 0 for no timeout. (default: %d)">
    ::Default<DEFAULT_HTTP_CLIENT_TIMEOUT>;

using RpcconnectSetting = common::Setting<
    "-rpcconnect=<ip>", std::string, common::SettingOptions{.legacy = true},
    "Send commands to node running on <ip> (default: %s)">
    ::Default<DEFAULT_RPCCONNECT>;

using RpccookiefileSetting = common::Setting<
    "-rpccookiefile=<loc>", common::Unset, common::SettingOptions{.legacy = true},
    "Location of the auth cookie. Relative paths will be prefixed by a net-specific datadir location. (default: data dir)">;

using RpcpasswordSetting = common::Setting<
    "-rpcpassword=<pw>", std::string, common::SettingOptions{.legacy = true},
    "Password for JSON-RPC connections">;

using RpcportSetting = common::Setting<
    "-rpcport=<port>", std::optional<std::string>, common::SettingOptions{.legacy = true, .network_only = true},
    "Connect to JSON-RPC on <port> (default: %u, testnet: %u, testnet4: %u, signet: %u, regtest: %u)">
    ::HelpFn<[](const auto& fmt, const auto& defaultBaseParams, const auto& testnetBaseParams, const auto& testnet4BaseParams, const auto& signetBaseParams, const auto& regtestBaseParams) { return strprintf(fmt, defaultBaseParams->RPCPort(), testnetBaseParams->RPCPort(), testnet4BaseParams->RPCPort(), signetBaseParams->RPCPort(), regtestBaseParams->RPCPort()); }>;

using RpcuserSetting = common::Setting<
    "-rpcuser=<user>", std::string, common::SettingOptions{.legacy = true},
    "Username for JSON-RPC connections">;

using RpcwaitSetting = common::Setting<
    "-rpcwait", bool, common::SettingOptions{.legacy = true},
    "Wait for RPC server to start">;

using RpcwaittimeoutSetting = common::Setting<
    "-rpcwaittimeout=<n>", int64_t, common::SettingOptions{.legacy = true, .disallow_negation = true},
    "Timeout in seconds to wait for the RPC server to start, or 0 for no timeout. (default: %d)">
    ::Default<DEFAULT_WAIT_CLIENT_TIMEOUT>;

using RpcwalletSetting = common::Setting<
    "-rpcwallet=<walletname>", std::string, common::SettingOptions{.legacy = true},
    "Send RPC for non-default wallet on RPC server (needs to exactly match corresponding -wallet option passed to bitcoind). This changes the RPC endpoint used, e.g. http://127.0.0.1:8332/wallet/<walletname>">;

using StdinSetting = common::Setting<
    "-stdin", bool, common::SettingOptions{.legacy = true},
    "Read extra arguments from standard input, one per line until EOF/Ctrl-D (recommended for sensitive information such as passphrases). When combined with -stdinrpcpass, the first line from standard input is used for the RPC password.">;

using StdinrpcpassSetting = common::Setting<
    "-stdinrpcpass", bool, common::SettingOptions{.legacy = true},
    "Read RPC password from standard input as a single line. When combined with -stdin, the first line from standard input is used for the RPC password. When combined with -stdinwalletpassphrase, -stdinrpcpass consumes the first line, and -stdinwalletpassphrase consumes the second.">;

using StdinwalletpassphraseSetting = common::Setting<
    "-stdinwalletpassphrase", bool, common::SettingOptions{.legacy = true},
    "Read wallet passphrase from standard input as a single line. When combined with -stdin, the first line from standard input is used for the wallet passphrase.">;

#endif // BITCOIN_BITCOIN_CLI_SETTINGS_H
