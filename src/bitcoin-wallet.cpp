// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <chainparams.h>
#include <chainparamsbase.h>
#include <consensus/consensus.h>
#include <logging.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <wallet/wallettool.h>

#include <stdio.h>

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

static void SetupWalletToolArgs()
{
    SetupHelpOptions(gArgs);
    SetupChainParamsBaseOptions();

    gArgs.AddArg("-datadir=<dir>", "Specify data directory", false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-wallet=<wallet-name>", "Specify wallet name", false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-debug=<category>", "Output debugging information (default: 0).", false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-printtoconsole", "Send trace/debug info to console (default: 1 when no -debug is true, 0 otherwise.", false, OptionsCategory::DEBUG_TEST);

    gArgs.AddArg("info", "Get wallet info", false, OptionsCategory::COMMANDS);
    gArgs.AddArg("create", "Create new wallet file", false, OptionsCategory::COMMANDS);
}

static bool WalletAppInit(int argc, char* argv[])
{
    SetupWalletToolArgs();
    std::string error_message;
    if (!gArgs.ParseParameters(argc, argv, error_message)) {
        fprintf(stderr, "Error parsing command line arguments: %s\n", error_message.c_str());
        return false;
    }
    if (argc < 2 || HelpRequested(gArgs)) {
        std::string usage = strprintf("%s bitcoin-wallet version", PACKAGE_NAME) + " " + FormatFullVersion() + "\n\n" +
                            "wallet-tool is an offline tool for creating and interacting with Bitcoin Core wallet files.\n" +
                            "By default wallet-tool will act on wallets in the default mainnet wallet directory in the datadir.\n" +
                            "To change the target wallet, use the -datadir, -wallet and -testnet/-regtest arguments.\n\n" +
                            "Usage:\n" +
                            "  bitcoin-wallet [options] <method>\n\n" +
                            gArgs.GetHelpMessage();

        fprintf(stdout, "%s", usage.c_str());
        return false;
    }

    // check for printtoconsole, allow -debug
    LogInstance().m_print_to_console = gArgs.GetBoolArg("-printtoconsole", gArgs.GetBoolArg("-debug", false));

    if (!fs::is_directory(GetDataDir(false))) {
        fprintf(stderr, "Error: Specified data directory \"%s\" does not exist.\n", gArgs.GetArg("-datadir", "").c_str());
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
    } catch (const std::exception& e) {
        PrintExceptionContinue(&e, "WalletAppInit()");
        return EXIT_FAILURE;
    } catch (...) {
        PrintExceptionContinue(nullptr, "WalletAppInit()");
        return EXIT_FAILURE;
    }

    std::string method {};
    std::vector<std::string> arguments{};
    for(int i = 1; i < argc; ++i) {
        if (!IsSwitchChar(argv[i][0])) {
            if (method.empty()) {
                method = argv[i];
            } else {
                arguments.push_back(argv[i]);
            }
        }
    }

    if (method.empty()) {
        fprintf(stderr, "No method provided. Run `bitcoin-wallet -help` for valid methods.\n");
        return EXIT_FAILURE;
    }

    const std::set<std::string> methods = {
        "create",
        "info"};

    if (methods.find(method) == methods.end()) { // Use contains() as of c++20
        fprintf(stderr, "Invalid method: %s\n", method.c_str());
        return EXIT_FAILURE;
    }

    std::string name = gArgs.GetArg("-wallet", "");

    ECCVerifyHandle globalVerifyHandle;
    ECC_Start();
    // A name must be provided when creating a file
    if (method == "create") {
        if (!gArgs.IsArgSet("-wallet")) {
            fprintf(stderr, "Wallet name must be provided when creating a new wallet.\n");
            return EXIT_FAILURE;
        }
        if (!arguments.empty()) {
            fprintf(stderr, "Error: unexpected argument(s)\n");
            return EXIT_FAILURE;
        }
        if (!WalletTool::ExecuteCreateWallet(name)) return EXIT_FAILURE;
    } else if (method == "info") {
        if (!arguments.empty()) {
            fprintf(stderr, "Error: unexpected argument(s)\n");
            return EXIT_FAILURE;
        }
        if (!WalletTool::ExecuteWalletInfo(name)) return EXIT_FAILURE;
    } else {
        assert(false);
    }

    ECC_Stop();
    return EXIT_SUCCESS;
}
