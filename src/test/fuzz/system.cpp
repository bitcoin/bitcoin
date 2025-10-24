// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {
void initialize_system()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
}

std::string GetArgumentName(const std::string& name)
{
    size_t idx = name.find('=');
    if (idx == std::string::npos) {
        idx = name.size();
    }
    return name.substr(0, idx);
}

FUZZ_TARGET(system, .init = initialize_system)
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
                auto str_arg = fuzzed_data_provider.ConsumeRandomLengthString(16);
                auto str_value = fuzzed_data_provider.ConsumeRandomLengthString(16);
                // Avoid Can't call SoftSetArg on arg registered with flags 0x8d8d8d00 (requires 0x2, disallows 0x10)
                try {
                    args_manager.SoftSetArg(str_arg, str_value);
                } catch (const std::logic_error& e) {
                    if (std::string_view(e.what()).find("Can't call ForceSetArg on arg") == std::string_view::npos) throw;
                }
            },
            [&] {
                auto str_arg = fuzzed_data_provider.ConsumeRandomLengthString(16);
                auto str_value = fuzzed_data_provider.ConsumeRandomLengthString(16);
                args_manager.ForceSetArg(str_arg, str_value);
            },
            [&] {
                auto str_arg = fuzzed_data_provider.ConsumeRandomLengthString(16);
                auto f_value = fuzzed_data_provider.ConsumeBool();
                // Avoid Can't call SoftSetBoolArg on arg registered with flags 0x8d8d8d00 (requires 0x2, disallows 0x10)
                try {
                    args_manager.SoftSetBoolArg(str_arg, f_value);
                } catch (const std::logic_error& e) {
                    if (std::string_view(e.what()).find("Can't call SoftSetBoolArg on arg") == std::string_view::npos) throw;
                }
            },
            [&] {
                const OptionsCategory options_category = fuzzed_data_provider.PickValueInArray<OptionsCategory>({OptionsCategory::OPTIONS, OptionsCategory::CONNECTION, OptionsCategory::WALLET, OptionsCategory::WALLET_DEBUG_TEST, OptionsCategory::ZMQ, OptionsCategory::DEBUG_TEST, OptionsCategory::CHAINPARAMS, OptionsCategory::NODE_RELAY, OptionsCategory::BLOCK_CREATION, OptionsCategory::RPC, OptionsCategory::GUI, OptionsCategory::COMMANDS, OptionsCategory::REGISTER_COMMANDS, OptionsCategory::CLI_COMMANDS, OptionsCategory::IPC, OptionsCategory::HIDDEN});
                // Avoid hitting:
                // common/args.cpp:563: void ArgsManager::AddArg(const std::string &, const std::string &, unsigned int, const OptionsCategory &): Assertion `ret.second' failed.
                const std::string argument_name = GetArgumentName(fuzzed_data_provider.ConsumeRandomLengthString(16));
                if (args_manager.GetArgFlags(argument_name) != std::nullopt) {
                    return;
                }
                auto help = fuzzed_data_provider.ConsumeRandomLengthString(16);
                auto flags = fuzzed_data_provider.ConsumeIntegral<unsigned int>() & ~ArgsManager::COMMAND;
                // Avoid hitting "ALLOW_INT flag is incompatible with ALLOW_STRING", etc exceptions
                if (flags & ArgsManager::ALLOW_ANY) flags &= ~(ArgsManager::ALLOW_BOOL | ArgsManager::ALLOW_INT | ArgsManager::ALLOW_STRING);
                if (flags & ArgsManager::ALLOW_BOOL) flags &= ~ArgsManager::DISALLOW_ELISION;
                if (flags & ArgsManager::ALLOW_STRING) flags &= ~ArgsManager::ALLOW_INT;
                args_manager.AddArg(argument_name, help, flags, options_category);
            },
            [&] {
                // Avoid hitting:
                // common/args.cpp:563: void ArgsManager::AddArg(const std::string &, const std::string &, unsigned int, const OptionsCategory &): Assertion `ret.second' failed.
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

    try {
        (void)args_manager.GetIntArg(s1, i64);
    } catch (const std::logic_error& e) {
        if (std::string_view(e.what()).find("Can't call GetIntArg on arg") == std::string_view::npos) throw;
    }
    try {
        (void)args_manager.GetArg(s1, s2);
    } catch (const std::logic_error& e) {
        if (std::string_view(e.what()).find("Can't call GetArg on arg") == std::string_view::npos) throw;
    }
    (void)args_manager.GetArgFlags(s1);
    try {
        (void)args_manager.GetArgs(s1);
    } catch (const std::logic_error& e) {
        if (std::string_view(e.what()).find("Can't call GetArgs on arg") == std::string_view::npos) throw;
    }
    try {
        (void)args_manager.GetBoolArg(s1, b);
    } catch (const std::logic_error& e) {
        if (std::string_view(e.what()).find("Can't call GetBoolArg on arg") == std::string_view::npos) throw;
    }
    try {
        (void)args_manager.GetChainTypeString();
    } catch (const std::runtime_error&) {
    }
    (void)args_manager.GetHelpMessage();
    (void)args_manager.GetUnrecognizedSections();
    (void)args_manager.GetUnsuitableSectionOnlyArgs();
    (void)args_manager.IsArgNegated(s1);
    (void)args_manager.IsArgSet(s1);

    (void)HelpRequested(args_manager);
}
} // namespace
