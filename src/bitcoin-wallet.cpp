// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <chainparams.h>
#include <chainparamsbase.h>
#include <interfaces/init.h>
#include <logging.h>
#include <util/system.h>
#include <util/translation.h>
#include <util/url.h>
#include <wallet/wallettool.h>

#include <functional>

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;
UrlDecodeFn* const URL_DECODE = nullptr;

static void SetupWalletToolArgs(ArgsManager& argsman)
{
    SetupHelpOptions(argsman);
    SetupChainParamsBaseOptions(argsman);

    argsman.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-datadir=<dir>", "Specify data directory", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-wallet=<wallet-name>", "Specify wallet name", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::OPTIONS);
    argsman.AddArg("-dumpfile=<file name>", "When used with 'dump', writes out the records to this file. When used with 'createfromdump', loads the records into a new wallet.", ArgsManager::ALLOW_STRING, OptionsCategory::OPTIONS);
    argsman.AddArg("-debug=<category>", "Output debugging information (default: 0).", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-descriptors", "Create descriptors wallet. Only for 'create'", ArgsManager::ALLOW_BOOL, OptionsCategory::OPTIONS);
    argsman.AddArg("-legacy", "Create legacy wallet. Only for 'create'", ArgsManager::ALLOW_BOOL, OptionsCategory::OPTIONS);
    argsman.AddArg("-format=<format>", "The format of the wallet file to create. Either \"bdb\" or \"sqlite\". Only used with 'createfromdump'", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-printtoconsole", "Send trace/debug info to console (default: 1 when no -debug is true, 0 otherwise).", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);

    argsman.AddCommand("info", "Get wallet info");
    argsman.AddCommand("create", "Create new wallet file");
    argsman.AddCommand("salvage", "Attempt to recover private keys from a corrupt wallet. Warning: 'salvage' is experimental.");
    argsman.AddCommand("dump", "Print out all of the wallet key-value records");
    argsman.AddCommand("createfromdump", "Create new wallet file from dumped records");
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
        std::string strUsage = strprintf("%s bitcoin-wallet version", PACKAGE_NAME) + " " + FormatFullVersion() + "\n";
        if (!args.IsArgSet("-version")) {
            strUsage += "\n"
                        "bitcoin-wallet is an offline tool for creating and interacting with " PACKAGE_NAME " wallet files.\n"
                        "By default bitcoin-wallet will act on wallets in the default mainnet wallet directory in the datadir.\n"
                        "To change the target wallet, use the -datadir, -wallet and -testnet/-regtest arguments.\n\n"
                        "Usage:\n"
                        "  bitcoin-wallet [options] <command>\n";
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
    // Check for chain settings (Params() calls are only valid after this clause)
    SelectParams(args.GetChainName());

    return true;
}

int main(int argc, char* argv[])
{
    ArgsManager& args = gArgs;
#ifdef WIN32
    util::WinCmdLineArgs winArgs;
    std::tie(argc, argv) = winArgs.get();
#endif

    int exit_status;
    std::unique_ptr<interfaces::Init> init = interfaces::MakeWalletInit(argc, argv, exit_status);
    if (!init) {
        return exit_status;
    }

    SetupEnvironment();
    RandomInit();
    try {
        if (!WalletAppInit(args, argc, argv)) return EXIT_FAILURE;
    } catch (const std::exception& e) {
        PrintExceptionContinue(&e, "WalletAppInit()");
        return EXIT_FAILURE;
    } catch (...) {
        PrintExceptionContinue(nullptr, "WalletAppInit()");
        return EXIT_FAILURE;
    }

    const auto command = args.GetCommand();
    if (!command) {
        tfm::format(std::cerr, "No method provided. Run `bitcoin-wallet -help` for valid methods.\n");
        return EXIT_FAILURE;
    }
    if (command->args.size() != 0) {
        tfm::format(std::cerr, "Error: Additional arguments provided (%s). Methods do not take arguments. Please refer to `-help`.\n", Join(command->args, ", "));
        return EXIT_FAILURE;
    }

    ECCVerifyHandle globalVerifyHandle;
    ECC_Start();
    if (!WalletTool::ExecuteWalletToolFunc(args, command->command)) {
        return EXIT_FAILURE;
    }
    ECC_Stop();
    return EXIT_SUCCESS;
}
