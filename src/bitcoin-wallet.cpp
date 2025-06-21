// Copyright (c) 2016-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <chainparams.h>
#include <chainparamsbase.h>
#include <compat/compat.h>
#include <logging.h>
#include <util/strencodings.h>
#include <clientversion.h>
#include <key.h>
#include <pubkey.h>
#include <tinyformat.h>
#include <util/system.h>
#include <util/translation.h>
#include <util/url.h>
#include <wallet/wallettool.h>

#include <exception>
#include <string>
#include <tuple>
const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;
UrlDecodeFn* const URL_DECODE = nullptr;

static void SetupWalletToolArgs(ArgsManager& argsman)
{
    SetupHelpOptions(argsman);
    SetupChainParamsBaseOptions(argsman);

    argsman.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-usehd", strprintf("Create HD (hierarchical deterministic) wallet (default: %d)", true), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-datadir=<dir>", "Specify data directory", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-wallet=<wallet-name>", "Specify wallet name", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-dumpfile=<file name>", "When used with 'dump', writes out the records to this file. When used with 'createfromdump', loads the records into a new wallet.", ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_NEGATION, OptionsCategory::OPTIONS);
    argsman.AddArg("-debug=<category>", "Output debugging information (default: 0).", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-descriptors", "Create descriptors wallet. Only for 'create'", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-format=<format>", "The format of the wallet file to create. Either \"bdb\" or \"sqlite\". Only used with 'createfromdump'", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-printtoconsole", "Send trace/debug info to console (default: 1 when no -debug is true, 0 otherwise).", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);

    argsman.AddCommand("info", "Get wallet info", OptionsCategory::COMMANDS);
    argsman.AddCommand("create", "Create new wallet file", OptionsCategory::COMMANDS);
    argsman.AddCommand("salvage", "Attempt to recover private keys from a corrupt wallet. Warning: 'salvage' is experimental.", OptionsCategory::COMMANDS);
    argsman.AddCommand("wipetxes", "Wipe all transactions from a wallet", OptionsCategory::COMMANDS);
    argsman.AddCommand("dump", "Print out all of the wallet key-value records", OptionsCategory::COMMANDS);
    argsman.AddCommand("createfromdump", "Create new wallet file from dumped records", OptionsCategory::COMMANDS);
}

static bool WalletAppInit(ArgsManager& args, int argc, char* argv[])
{
    SetupWalletToolArgs(args);
    std::string error_message;
    if (!args.ParseParameters(argc, argv, error_message)) {
        tfm::format(std::cerr, "Error parsing command line arguments: %s\n", error_message);
        return false;
    }
    if (argc < 2 || HelpRequested(args) || args.IsArgSet("-version")) {
        std::string strUsage = strprintf("%s dash-wallet version", PACKAGE_NAME) + " " + FormatFullVersion() + "\n";
        if (!args.IsArgSet("-version")) {
            strUsage += "\n"
                    "dash-wallet is an offline tool for creating and interacting with " PACKAGE_NAME " wallet files.\n"
                    "By default dash-wallet will act on wallets in the default mainnet wallet directory in the datadir.\n"
                    "To change the target wallet, use the -datadir, -wallet and -regtest/-testnet arguments.\n\n"
                    "Usage:\n"
                    "  dash-wallet [options] <command>\n";
            strUsage += "\n" + args.GetHelpMessage();
        }
        tfm::format(std::cout, "%s", strUsage);
        return false;
    }

    // check for printtoconsole, allow -debug
    LogInstance().m_print_to_console = args.GetBoolArg("-printtoconsole", args.GetBoolArg("-debug", false));

    if (!CheckDataDirOption()) {
        tfm::format(std::cerr, "Error: Specified data directory \"%s\" does not exist.\n", args.GetArg("-datadir", ""));
        return false;
    }
    // Check for -testnet or -regtest parameter (Params() calls are only valid after this clause)
    SelectParams(args.GetChainName());

    return true;
}

MAIN_FUNCTION
{
    ArgsManager& args = gArgs;
#ifdef WIN32
    util::WinCmdLineArgs winArgs;
    std::tie(argc, argv) = winArgs.get();
#endif
    SetupEnvironment();
    RandomInit();
    try {
        if (!WalletAppInit(args, argc, argv)) return EXIT_FAILURE;
    } catch (...) {
        PrintExceptionContinue(std::current_exception(), "WalletAppInit()");
        return EXIT_FAILURE;
    }

    const auto command = args.GetCommand();
    if (!command) {
        tfm::format(std::cerr, "No method provided. Run `dash-wallet -help` for valid methods.\n");
        return EXIT_FAILURE;
    }
    if (command->args.size() != 0) {
        tfm::format(std::cerr, "Error: Additional arguments provided (%s). Methods do not take arguments. Please refer to `-help`.\n", Join(command->args, ", "));
        return EXIT_FAILURE;
    }

    ECC_Start();
    if (!wallet::WalletTool::ExecuteWalletToolFunc(args, command->command)) {
        return EXIT_FAILURE;
    }
    ECC_Stop();
    return EXIT_SUCCESS;
}
