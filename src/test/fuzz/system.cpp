// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/system.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {
std::string GetArgumentName(const std::string& name)
{
    size_t idx = name.find('=');
    if (idx == std::string::npos) {
        idx = name.size();
    }
    return name.substr(0, idx);
}
} // namespace

FUZZ_TARGET(system)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    ArgsManager args_manager{};

    if (fuzzed_data_provider.ConsumeBool()) {
        SetupHelpOptions(args_manager);
    }

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 3000)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                args_manager.SelectConfigNetwork(fuzzed_data_provider.ConsumeRandomLengthString(16));
            },
            [&] {
                args_manager.SoftSetArg(fuzzed_data_provider.ConsumeRandomLengthString(16), fuzzed_data_provider.ConsumeRandomLengthString(16));
            },
            [&] {
                args_manager.ForceSetArg(fuzzed_data_provider.ConsumeRandomLengthString(16), fuzzed_data_provider.ConsumeRandomLengthString(16));
            },
            [&] {
                args_manager.SoftSetBoolArg(fuzzed_data_provider.ConsumeRandomLengthString(16), fuzzed_data_provider.ConsumeBool());
            },
            [&] {
                const OptionsCategory options_category = fuzzed_data_provider.PickValueInArray<OptionsCategory>({OptionsCategory::OPTIONS, OptionsCategory::CONNECTION, OptionsCategory::WALLET, OptionsCategory::WALLET_DEBUG_TEST, OptionsCategory::ZMQ, OptionsCategory::DEBUG_TEST, OptionsCategory::CHAINPARAMS, OptionsCategory::NODE_RELAY, OptionsCategory::BLOCK_CREATION, OptionsCategory::RPC, OptionsCategory::GUI, OptionsCategory::COMMANDS, OptionsCategory::REGISTER_COMMANDS, OptionsCategory::HIDDEN});
                // Avoid hitting:
                // util/system.cpp:425: void ArgsManager::AddArg(const std::string &, const std::string &, unsigned int, const OptionsCategory &): Assertion `ret.second' failed.
                const std::string argument_name = GetArgumentName(fuzzed_data_provider.ConsumeRandomLengthString(16));
                if (args_manager.GetArgFlags(argument_name) != std::nullopt) {
                    return;
                }
                args_manager.AddArg(argument_name, fuzzed_data_provider.ConsumeRandomLengthString(16), fuzzed_data_provider.ConsumeIntegral<unsigned int>() & ~ArgsManager::COMMAND, options_category);
            },
            [&] {
                // Avoid hitting:
                // util/system.cpp:425: void ArgsManager::AddArg(const std::string &, const std::string &, unsigned int, const OptionsCategory &): Assertion `ret.second' failed.
                const std::vector<std::string> names = ConsumeRandomLengthStringVector(fuzzed_data_provider);
                std::vector<std::string> hidden_arguments;
                for (const std::string& name : names) {
                    const std::string hidden_argument = GetArgumentName(name);
                    if (args_manager.GetArgFlags(hidden_argument) != std::nullopt) {
                        continue;
                    }
                    if (std::find(hidden_arguments.begin(), hidden_arguments.end(), hidden_argument) != hidden_arguments.end()) {
                        continue;
                    }
                    hidden_arguments.push_back(hidden_argument);
                }
                args_manager.AddHiddenArgs(hidden_arguments);
            },
            [&] {
                args_manager.ClearArgs();
            },
            [&] {
                const std::vector<std::string> random_arguments = ConsumeRandomLengthStringVector(fuzzed_data_provider);
                std::vector<const char*> argv;
                argv.reserve(random_arguments.size());
                for (const std::string& random_argument : random_arguments) {
                    argv.push_back(random_argument.c_str());
                }
                try {
                    std::string error;
                    (void)args_manager.ParseParameters(argv.size(), argv.data(), error);
                } catch (const std::logic_error&) {
                }
            });
    }

    const std::string s1 = fuzzed_data_provider.ConsumeRandomLengthString(16);
    const std::string s2 = fuzzed_data_provider.ConsumeRandomLengthString(16);
    const int64_t i64 = fuzzed_data_provider.ConsumeIntegral<int64_t>();
    const bool b = fuzzed_data_provider.ConsumeBool();

    (void)args_manager.GetIntArg(s1, i64);
    (void)args_manager.GetArg(s1, s2);
    (void)args_manager.GetArgFlags(s1);
    (void)args_manager.GetArgs(s1);
    (void)args_manager.GetBoolArg(s1, b);
    try {
        (void)args_manager.GetChainName();
    } catch (const std::runtime_error&) {
    }
    (void)args_manager.GetHelpMessage();
    (void)args_manager.GetUnrecognizedSections();
    (void)args_manager.GetUnsuitableSectionOnlyArgs();
    (void)args_manager.IsArgNegated(s1);
    (void)args_manager.IsArgSet(s1);

    (void)HelpRequested(args_manager);
}
