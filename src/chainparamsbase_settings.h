#ifndef BITCOIN_CHAINPARAMSBASE_SETTINGS_H
#define BITCOIN_CHAINPARAMSBASE_SETTINGS_H

#include <chainparamsbase.h>
#include <common/setting.h>

#include <string>
#include <vector>

using SignetseednodeSetting = common::Setting<
    "-signetseednode", std::vector<std::string>, common::SettingOptions{.legacy = true, .disallow_negation = true},
    "Specify a seed node for the signet network, in the hostname[:port] format, e.g. sig.net:1234 (may be used multiple times to specify multiple seed nodes; defaults to the global default signet test network seed node(s))">
    ::Category<OptionsCategory::CHAINPARAMS>;

using SignetchallengeSetting = common::Setting<
    "-signetchallenge", std::vector<std::string>, common::SettingOptions{.legacy = true, .disallow_negation = true},
    "Blocks must satisfy the given script to be considered valid (only for signet networks; defaults to the global default signet test network challenge)">
    ::Category<OptionsCategory::CHAINPARAMS>;

using TestactivationheightSetting = common::Setting<
    "-testactivationheight=name@height.", std::vector<std::string>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Set the activation height of 'name' (segwit, bip34, dersig, cltv, csv). (regtest-only)">
    ::Category<OptionsCategory::DEBUG_TEST>;

using VbparamsSetting = common::Setting<
    "-vbparams=deployment:start:end[:min_activation_height]", std::vector<std::string>, common::SettingOptions{.legacy = true, .debug_only = true},
    "Use given start/end times and min_activation_height for specified version bits deployment (regtest-only)">
    ::Category<OptionsCategory::CHAINPARAMS>;

using ChainSetting = common::Setting<
    "-chain=<chain>", std::optional<std::string>, common::SettingOptions{.legacy = true},
    "Use the chain <chain> (default: main). Allowed values: " LIST_CHAIN_NAMES>
    ::Category<OptionsCategory::CHAINPARAMS>;

using RegtestSetting = common::Setting<
    "-regtest", common::Unset, common::SettingOptions{.legacy = true, .debug_only = true},
    "Enter regression test mode, which uses a special chain in which blocks can be solved instantly. "
                 "This is intended for regression testing tools and app development. Equivalent to -chain=regtest.">
    ::Category<OptionsCategory::CHAINPARAMS>;

using TestnetSetting = common::Setting<
    "-testnet", common::Unset, common::SettingOptions{.legacy = true},
    "Use the testnet3 chain. Equivalent to -chain=test. Support for testnet3 is deprecated and will be removed in an upcoming release. Consider moving to testnet4 now by using -testnet4.">
    ::Category<OptionsCategory::CHAINPARAMS>;

using Testnet4Setting = common::Setting<
    "-testnet4", common::Unset, common::SettingOptions{.legacy = true},
    "Use the testnet4 chain. Equivalent to -chain=testnet4.">
    ::Category<OptionsCategory::CHAINPARAMS>;

using SignetSetting = common::Setting<
    "-signet", common::Unset, common::SettingOptions{.legacy = true},
    "Use the signet chain. Equivalent to -chain=signet. Note that the network is defined by the -signetchallenge parameter">
    ::Category<OptionsCategory::CHAINPARAMS>;

#endif // BITCOIN_CHAINPARAMSBASE_SETTINGS_H
