// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/dash-config.h>
#endif

#include <chainparams.h>
#include <chainparamsbase.h>
#include <logging.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <wallet/wallettool.h>

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

static void SetupWalletToolArgs()
{
    SetupChainParamsBaseOptions();

    gArgs.AddArg("-?", "This help message", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-datadir=<dir>", "Specify data directory", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-wallet=<wallet-name>", "Specify wallet name", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-debug=<category>", "Output debugging information (default: 0).", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-printtoconsole", "Send trace/debug info to console (default: 1 when no -debug is true, 0 otherwise.", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);

    gArgs.AddArg("info", "Get wallet info", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);
    gArgs.AddArg("create", "Create new wallet file", ArgsManager::ALLOW_ANY, OptionsCategory::COMMANDS);

    // Hidden
    gArgs.AddArg("-h", "", ArgsManager::ALLOW_ANY, OptionsCategory::HIDDEN);
    gArgs.AddArg("-help", "", ArgsManager::ALLOW_ANY, OptionsCategory::HIDDEN);
}

static bool WalletAppInit(int argc, char* argv[])
{
    SetupWalletToolArgs();
    std::string error_message;
    if (!gArgs.ParseParameters(argc, argv, error_message)) {
        tfm::format(std::cerr, "Error parsing command line arguments: %s\n", error_message);
        return false;
    }
    if (argc < 2 || HelpRequested(gArgs)) {
        std::string usage = strprintf("%s dash-wallet version", PACKAGE_NAME) + " " + FormatFullVersion() + "\n\n" +
                                      "wallet-tool is an offline tool for creating and interacting with Dash Core wallet files.\n" +
                                      "By default wallet-tool will act on wallets in the default mainnet wallet directory in the datadir.\n" +
                                      "To change the target wallet, use the -datadir, -wallet and -testnet/-regtest arguments.\n\n" +
                                      "Usage:\n" +
                                     "  dash-wallet [options] <command>\n\n" +
                                     gArgs.GetHelpMessage();

        tfm::format(std::cout, "%s", usage);
        return false;
    }

    // check for printtoconsole, allow -debug
    LogInstance().m_print_to_console = gArgs.GetBoolArg("-printtoconsole", gArgs.GetBoolArg("-debug", false));

    if (!fs::is_directory(GetDataDir(false))) {
        tfm::format(std::cerr, "Error: Specified data directory \"%s\" does not exist.\n", gArgs.GetArg("-datadir", ""));
        return false;
    }
    // Check for -testnet or -regtest parameter (Params() calls are only valid after this clause)
    SelectParams(gArgs.GetChainName());

    return true;
}

int main(int argc, char* argv[])
{
#ifdef WIN32
    util::WinCmdLineArgs winArgs;
    std::tie(argc, argv) = winArgs.get();
#endif
    SetupEnvironment();
    RandomInit();
    try {
        if (!WalletAppInit(argc, argv)) return EXIT_FAILURE;
    } catch (...) {
        PrintExceptionContinue(std::current_exception(), "WalletAppInit()");
        return EXIT_FAILURE;
    }

    std::string method {};
    for(int i = 1; i < argc; ++i) {
        if (!IsSwitchChar(argv[i][0])) {
            if (!method.empty()) {
                tfm::format(std::cerr, "Error: two methods provided (%s and %s). Only one method should be provided.\n", method, argv[i]);
                return EXIT_FAILURE;
            }
            method = argv[i];
        }
    }

    if (method.empty()) {
        tfm::format(std::cerr, "No method provided. Run `dash-wallet -help` for valid methods.\n");
        return EXIT_FAILURE;
    }

    // A name must be provided when creating a file
    if (method == "create" && !gArgs.IsArgSet("-wallet")) {
        tfm::format(std::cerr, "Wallet name must be provided when creating a new wallet.\n");
        return EXIT_FAILURE;
    }

    std::string name = gArgs.GetArg("-wallet", "");

    ECCVerifyHandle globalVerifyHandle;
    ECC_Start();
    if (!WalletTool::ExecuteWalletToolFunc(method, name))
        return EXIT_FAILURE;
    ECC_Stop();
    return EXIT_SUCCESS;
}
