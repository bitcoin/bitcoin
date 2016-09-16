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
#include <util.h>
#include <utilstrencodings.h>
#include <wallet/wallettool.h>

#include <stdio.h>

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

static void SetupWalletToolArgs()
{
    gArgs.AddArg("-?", "This help message", false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-name=<wallet-name>", "Specify wallet name", false, OptionsCategory::OPTIONS);

    gArgs.AddArg("info", "Get wallet info", false, OptionsCategory::COMMANDS);
    gArgs.AddArg("create", "Create new wallet file", false, OptionsCategory::COMMANDS);
    gArgs.AddArg("salvage", "Recover readable keypairs", false, OptionsCategory::COMMANDS);
    gArgs.AddArg("zaptxs", "Remove all transactions including metadata (will keep keys)", false, OptionsCategory::COMMANDS);

    // Hidden
    gArgs.AddArg("-h", "", false, OptionsCategory::HIDDEN);
    gArgs.AddArg("-help", "", false, OptionsCategory::HIDDEN);
}

static bool WalletAppInit(int argc, char* argv[])
{
    SetupWalletToolArgs();
    std::string error;
    if (!gArgs.ParseParameters(argc, argv, error)) {
        fprintf(stderr, "Error parsing command line arguments: %s\n", error.c_str());
        return false;
    }
    if (argc < 2 || HelpRequested(gArgs)) {
        std::string usage = strprintf(_("%s wallet-tool version"), PACKAGE_NAME) + " " + FormatFullVersion() + "\n\n" +
                                      _("Usage:") + "\n" +
                                     "  bitcoin-wallet-tool [options] <command>\n\n" +
                                     gArgs.GetHelpMessage();

        fprintf(stdout, "%s", usage.c_str());
        return false;
    }

    // check for printtoconsole, allow -debug
    g_logger->m_print_to_console = gArgs.GetBoolArg("-printtoconsole", gArgs.GetBoolArg("-debug", false));

    // Check for -testnet or -regtest parameter (Params() calls are only valid after this clause)
    try {
        SelectParams(gArgs.GetChainName());
    } catch (const std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        return false;
    }

    return true;
}

int main(int argc, char* argv[])
{
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

    while (argc > 1 && IsSwitchChar(argv[1][0])) {
        argc--;
        argv++;
    }
    std::vector<std::string> args = std::vector<std::string>(&argv[1], &argv[argc]);
    std::string method = args[0];

    // A name must be provided when creating a file
    if (method == "create" && !gArgs.IsArgSet("-name")) {
        fprintf(stderr, "Wallet name must be provided when creating a new wallet.\n");
        return EXIT_FAILURE;
    }

    std::string name = gArgs.GetArg("-name", "");

    ECCVerifyHandle globalVerifyHandle;
    ECC_Start();
    if (!WalletTool::executeWalletToolFunc(method, name))
        return EXIT_FAILURE;
    ECC_Stop();
    return EXIT_SUCCESS;
}
